#include"pbf/pbf.h"
#include"pbf/neighborsearcher.h"
#include"pbf/sphkernel.h"
#define NOMINMAX

#include"tbb/tbb.h"

PBFSimulator::PBFSimulator(ParticleGroup& pg,uint32_t n_fluids,float density0,float particle_radius,float vc,int iter_T):
                            particlegroup(pg),n_fluids(n_fluids),
                            rest_density(density0),particle_radius(particle_radius),sph_radius(4*particle_radius),vicosity_coeff(vc),
                            sphkernels(new SPHKernels(sph_radius)),
                            iter_T(iter_T)
{
    neighborsearcher = new NeighborSearcher(particlegroup,sph_radius,n_fluids,particlegroup.size());
    tbb::parallel_for(tbb::blocked_range<uint32_t>(n_fluids,particlegroup.size(),1000),
            [&](const tbb::blocked_range<uint32_t>& r){
                for(uint32_t i=r.begin();i!=r.end();++i){
                    auto& centerp = particlegroup[i];
                    float denominator = sphkernels->W_Poly6(glm::vec3(0.0f));
                    for(auto neighbor_idx:neighborsearcher->GetNeighbors(i)){
                        if(neighbor_idx >= n_fluids){
                            auto& neighbor = particlegroup[neighbor_idx];
                            denominator += sphkernels->W_Poly6(centerp.position - neighbor.position);
                        }
                    }
                    centerp.mass = rest_density/denominator;
                }
            }
        );

}
PBFSimulator::~PBFSimulator()
{
    delete neighborsearcher;
    delete sphkernels;
}


float PBFSimulator::CalcDensity(uint32_t idx) const
{
    std::vector<uint32_t> neighbors;
    auto& centerp = particlegroup[idx];
    float density = centerp.mass*sphkernels->W_Poly6(glm::vec3(0.0f));
    for(auto neighbor_idx:neighborsearcher->GetNeighbors(idx)){
        auto& neighbor = particlegroup[neighbor_idx];
        density += neighbor.mass * (sphkernels->W_Poly6(centerp.position - neighbor.position));
    }
    return density;
}

float PBFSimulator::CalcConstraint(uint32_t idx) const
{
    return CalcDensity(idx)/rest_density - 1.0f;
}

glm::vec3 PBFSimulator::CalcGradConstraint(uint32_t idx, uint32_t t_idx) const
{
    auto& centerp = particlegroup[idx];
    if(idx == t_idx){
        glm::vec3 grad(0.0f,0.0f,0.0f);
        std::vector<uint32_t> neighbors;
        for(auto neighbor_idx:neighbors){
            auto& neighbor = particlegroup[neighbor_idx];
            grad += neighbor.mass*(sphkernels->Grad_W_Spiky(centerp.position-neighbor.position));
        }
        grad/=rest_density;
        return grad;
    }
    else{
        auto& tp = particlegroup[t_idx];
        return -tp.mass*sphkernels->Grad_W_Spiky(centerp.position-tp.position)/rest_density;
    }
}

float PBFSimulator::CalcLagrangeMultiplier(uint32_t idx) const
{
    float eps = 1e-6;
    float  numerator = std::max(0.0f,CalcConstraint(idx));
    if(numerator == 0) {
        return 0.0f;
    }
    float denominator = 0.0f;
    for(auto neighbor_idx:neighborsearcher->GetNeighbors(idx)){
        auto grad = CalcGradConstraint(idx,neighbor_idx);
        denominator += glm::dot(grad,grad);
    }
    auto igrad = CalcGradConstraint(idx,idx);
    denominator += glm::dot(igrad,igrad);
    denominator += eps;
    return -numerator / denominator;
}

glm::vec3 PBFSimulator::CalcDeltaPosition(uint32_t idx) const
{
    auto& centerp = particlegroup[idx];
    std::vector<uint32_t> neighbors;
    glm::vec3 dp(0.0f,0.0f,0.0f);
    float corr_k = centerp.mass*0.1;
    float corr_n = 4;
    glm::vec3 corr_dq = {0.2*sph_radius,0.0f,0.0f};
    for(auto neighbor_idx:neighborsearcher->GetNeighbors(idx)){
        auto& neighbor = particlegroup[neighbor_idx];
        float scorr = sphkernels->W_Poly6(centerp.position - neighbor.position)/sphkernels->W_Poly6(corr_dq);
        scorr = -corr_k*std::pow(scorr,corr_n);

        dp += (neighbor.lambda + centerp.lambda)*neighbor.mass*(sphkernels->Grad_W_Spiky(centerp.position - neighbor.position));
    }
    dp/=rest_density;
    return dp;
}
glm::vec3 PBFSimulator::CalcVorticityConfinementVelocity(uint32_t idx) const
{
    auto& centerp = particlegroup[idx];
    glm::vec3 corrv = centerp.tmp_velocity;
    glm::vec3 omega = glm::vec3(0.0f);
    glm::vec3 omega_x = glm::vec3(0.0f);
    glm::vec3 omega_y = glm::vec3(0.0f);
    glm::vec3 omega_z = glm::vec3(0.0f);
    for(auto neighbor_idx:neighborsearcher->GetNeighbors(idx)){
        if(neighbor_idx < n_fluids){
            auto& neighbor = particlegroup[neighbor_idx];
            glm::vec3 vgap = neighbor.tmp_velocity - centerp.tmp_velocity;
            omega += glm::cross(vgap,sphkernels->Grad_W_Spiky(centerp.position - neighbor.position));
            omega_x += glm::cross(vgap,sphkernels->Grad_W_Spiky(centerp.position + glm::vec3(0.01,0,0) - neighbor.position));
            omega_y += glm::cross(vgap,sphkernels->Grad_W_Spiky(centerp.position + glm::vec3(0,0.01,0) - neighbor.position));
            omega_z += glm::cross(vgap,sphkernels->Grad_W_Spiky(centerp.position + glm::vec3(0,0,0.01) - neighbor.position));
        }
    }
    float omega_length = glm::length(omega);
    glm::vec3 N = {glm::length(omega_x) - omega_length,glm::length(omega_y) - omega_length,glm::length(omega_z) - omega_length};
    N = glm::normalize(N);
    if (glm::isnan(N).x || glm::isnan(N).y || glm::isnan(N).z)
		return corrv;
    glm::vec3 force = 0.000010f*glm::cross(N,omega);
    corrv += force*dt;
    return corrv;
}
glm::vec3 PBFSimulator::CalcVicosityCorrectVelocity(uint32_t idx) const
{
    auto& centerp = particlegroup[idx];
    glm::vec3 corrv = particlegroup[idx].velocity;
    for(auto neighbor_idx:neighborsearcher->GetNeighbors(idx)){
        if(neighbor_idx < n_fluids){
            auto& neighbor = particlegroup[neighbor_idx];
            corrv -= (neighbor.mass/neighbor.density)*vicosity_coeff*sphkernels->W_Poly6(centerp.position - neighbor.position)*(centerp.velocity-neighbor.velocity);
        }
    }
    return corrv;
}
void PBFSimulator::UpdateTimeStepSizeCFL(const float &minTimestep, const float &maxTimestep)
{
    const float cflFactor =1.0f;
    float h = dt;
    float max_velocity = 0.1;
    const float diam = 2.0*particle_radius;
    for(uint32_t i=0;i<n_fluids;++i){
        const glm::vec3 &velocity = particlegroup[i].velocity;
		const glm::vec3 &acceleration = glm::vec3{0,-9.8,0};
		const float velMag = pow(glm::length(velocity + static_cast<float>(h) * acceleration), 2.0);
		if (velMag > max_velocity)
			max_velocity = velMag;
    }
    h = cflFactor*0.4*(diam/sqrt(max_velocity));
    h = std::min(h,maxTimestep);
    h = std::max(h,minTimestep);
    dt = h;
}
void PBFSimulator::Simulate()
{

    uint32_t n = particlegroup.size();
    
    UpdateTimeStepSizeCFL(0.0001,0.005);
    tbb::parallel_for(tbb::blocked_range<uint32_t>(0,n_fluids,1000),
        [&](const tbb::blocked_range<uint32_t>& r){
            for(uint32_t i=r.begin();i!=r.end();++i){
                particlegroup[i].last_postion = particlegroup[i].position;
                particlegroup[i].velocity += glm::vec3(0.0f,-9.81f,0.0f)*dt;
                particlegroup[i].position += particlegroup[i].velocity*dt;
            }
        }
    );
    //rebuild neighbor searching structure:
    delete neighborsearcher;
    neighborsearcher = new NeighborSearcher(particlegroup,sph_radius,0,n);
    int T = 5;
    while(T--){    
        
        tbb::parallel_for(tbb::blocked_range<uint32_t>(0,n_fluids,1000),
            [&](const tbb::blocked_range<uint32_t>& r){
                for(uint32_t i=r.begin();i!=r.end();++i){
                    particlegroup[i].lambda = CalcLagrangeMultiplier(i);
                }
            }
        );
        tbb::parallel_for(tbb::blocked_range<uint32_t>(0,n_fluids,1000),
            [&](const tbb::blocked_range<uint32_t>& r){
                for(uint32_t i=r.begin();i!=r.end();++i){
                    particlegroup[i].delta_postion = CalcDeltaPosition(i);
                }
            }
        );
        tbb::parallel_for(tbb::blocked_range<uint32_t>(0,n_fluids,1000),
            [&](const tbb::blocked_range<uint32_t>& r){
                for(uint32_t i=r.begin();i!=r.end();++i){
                    particlegroup[i].position += particlegroup[i].delta_postion;
                }
            }
        );
    }
    tbb::parallel_for(tbb::blocked_range<uint32_t>(0,n_fluids,1000),
            [&](const tbb::blocked_range<uint32_t>& r){
                for(uint32_t i=r.begin();i!=r.end();++i){
                    particlegroup[i].density = CalcDensity(i);
                }
            }
    );
    tbb::parallel_for(tbb::blocked_range<uint32_t>(0,n_fluids,5000),
            [&](const tbb::blocked_range<uint32_t>& r){
                for(uint32_t i=r.begin();i!=r.end();++i){
                    particlegroup[i].velocity = (particlegroup[i].position - particlegroup[i].last_postion)/dt;
                }
            }
    );
    for(uint32_t i=0;i<n_fluids;++i){
        particlegroup[i].velocity = CalcVicosityCorrectVelocity(i);
        particlegroup[i].tmp_velocity = particlegroup[i].velocity;
    }
    tbb::parallel_for(tbb::blocked_range<uint32_t>(0,n_fluids,5000),
            [&](const tbb::blocked_range<uint32_t>& r){
                for(uint32_t i=r.begin();i!=r.end();++i){
                    particlegroup[i].velocity = CalcVorticityConfinementVelocity(i);
                }
            }
    );
    
  
}
