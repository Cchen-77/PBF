#include"renderer.h"
#include"renderer_types.h"
#include"glm/gtc/matrix_transform.hpp"

#include<iostream>
#include<exception>
#include<chrono>
#include<string>
#include<algorithm>

#undef APIENTRY
#define NOMINMAX
#include"tbb/tbb.h"

#define PI 3.1415926
float W_Poly6(const glm::vec3 &r,float h)
{
    float ret = 0.0;
	float rl = glm::length(r);
	float q = rl / h;
	float h3 = h * h * h;
	if (q <= 0.5)
	{
		float q2 = q * q;
		float q3 = q2 * q;
		ret = 8.0 / (PI * h3) * (6.0 * q3 - 6.0 * q2 + 1.0);
	}
	else
	{
		ret = 16.0 / (PI * h3) * pow(1 - q, 3.0);
	}
	return ret;
}
float W_Spiky(const glm::vec3 &r,float h)
{
    float coeff = 15.0f/(PI*std::pow(h,6));
    float rlength = glm::length(r);
    if(rlength<0||rlength>h) return 0.0f;
    float diff = h-rlength;
    float diff3 = diff*diff*diff;
    return coeff*diff3;
}
glm::vec3 Grad_W_Spiky(const glm::vec3 &r,float h)
{
    glm::vec3 ret(0.0f);
	float rl = glm::length(r);
	float q = rl / h;
	float h3 = h * h * h;
	if (rl > 1.0e-6)
	{
		const glm::vec3 gradq = static_cast<float>(1.0 / (rl * h)) * r;
		if (q <= 0.5)
		{
			ret = static_cast<float>(48.0 / (PI * h3) * q * (3.0 * q - 2.0)) * gradq;
		}
		else
		{
			float factor = 1.0 - q;
			ret = static_cast<float>(48.0 / (PI * h3) * (-factor * factor)) * gradq;
		}
	}
	return ret;
}
void GetNeighbors(uint32_t idx,std::vector<Particle>& particles,float h,std::vector<uint32_t>& ngbrs){
    ngbrs.resize(0);
    int vx0 = std::floor(particles[idx].Location.x/h);
    int vy0 = std::floor(particles[idx].Location.y/h);
    int vz0 = std::floor(particles[idx].Location.z/h);
    for(int dx =-1;dx<=1;++dx){
        for(int dy=-1;dy<=1;++dy){
            for(int dz=-1;dz<=1;++dz){
                int vx = dx+vx0,vy = dy+vy0,vz = dz+vz0;
                
                int low = 0,high = particles.size()-1;
                int find = -1;
                while(low<=high){
                    int mid = (low+high)/2;
                    int vx1 = std::floor(particles[mid].Location.x/h);
                    int vy1 = std::floor(particles[mid].Location.y/h);
                    int vz1 = std::floor(particles[mid].Location.z/h);

                    bool less = false;
                    if(vx1<vx){
                        less = true;
                    }
                    else if(vx1>vx){
                        less = false;
                    }
                    else if(vy1<vy){
                        less = true;
                    }
                    else if(vy1>vy){
                        less = false;
                    }
                    else if(vz1<vz){
                        less = true;
                    }

                    if(less){
                        low = mid+1;
                    }
                    else{
                        find = mid;
                        high = mid-1;
                    }
                }
                if(find!=-1){
                    while(find < particles.size()){
                        int vx1 = std::floor(particles[find].Location.x/h);
                        int vy1 = std::floor(particles[find].Location.y/h);
                        int vz1 = std::floor(particles[find].Location.z/h);
                        if(vx1 != vx || vy1 != vy || vz1 != vz){
                            break;
                        }
                        if(glm::length(particles[idx].Location - particles[find].Location)<=h&&idx!=find){
                            ngbrs.push_back(find);
                        }
                        find++;
                    }
                }
            }
        }
    } 
}
float dt = 0.005;
void UpdateTimeStepSizeCFL(const float &minTimestep, const float &maxTimestep,float particle_radius,std::vector<Particle>& particles)
{
    const float cflFactor = 1.0f;
    float h = dt;
    float max_velocity = 0.1;
    const float diam = 2.0*particle_radius;
    for(uint32_t i=0;i<particles.size();++i){
        if(!particles[i].IsFluid) continue;
        const glm::vec3 &velocity = particles[i].Velocity;
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
int main(int argc,char** argv){
    
    try{
        Renderer renderer = Renderer(800,800,RF_PARTICLE,true);
        //Set the mvp 
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(glm::vec3(0.5,1.5,2.5),glm::vec3(0,0,0),glm::vec3(0,1,0));
        glm::mat4 projection = glm::perspective(glm::radians(90.0f),1.0f,0.1f,100.0f);
        projection[1][1]*=-1;
        renderer.SetMVP(model,view,projection,true);

        
        //PBF SETTINGS
        float radius = 0.04;
        float diam = 2*radius;
        float sph_radius = 4*radius;
        float rest_density = 1000;
        UniformComputeObject cubo;
        cubo.SphRadius = sph_radius;
        cubo.RestDensity=rest_density;

        //INIT FLUIDS PARTICLES
        std::vector<Particle> particles;
        //a water cube
        for(float i=-0.5;i<=0.5;i+=diam){
            for(float j=-0.5;j<=0.5;j+=diam){
                for(float k=-0.5;k<=0.5;k+=diam){
                    Particle particle{};
                    particle.Location = glm::vec3(i,j,k);
                    particle.Velocity = glm::vec3(0,0,0);
                    particle.IsFluid = 1;
                    particle.Mass = diam*diam*diam*rest_density;
                    particles.push_back(particle);
                }
            }
        }
        uint32_t num_fluids = particles.size();
        cubo.NumFluids = num_fluids;
        //add the walls
        for(float i=-1;i<=1;i+=diam)
            for(float j=-1;j<=1;j+=diam)
                for(float k =-0.1;k<=0.1;k+=diam){
                Particle particle{};
                particle.IsFluid = 0;
                //up
                particle.Location = glm::vec3(i,1+k,j);
                particles.push_back(particle);
                //floor
                particle.Location = glm::vec3(i,-1+k,j);
                particles.push_back(particle);
                //left
                particle.Location = glm::vec3(-1+k,i,j);
                
                particles.push_back(particle);
                //right
                particle.Location = glm::vec3(1+k,i,j);
                
                particles.push_back(particle);
                //front 
                particle.Location = glm::vec3(i,j,1+k);
                
                particles.push_back(particle);
                //back
                particle.Location = glm::vec3(i,j,-1+k);
                particles.push_back(particle);
            }
        cubo.NumParticles = particles.size();
        auto particlecmp = [&](const Particle& lhs,const Particle& rhs)->bool {
            int lx = std::floor(lhs.Location.x/sph_radius);
            int ly = std::floor(lhs.Location.y/sph_radius);
            int lz = std::floor(lhs.Location.z/sph_radius);
            
            int rx = std::floor(rhs.Location.x/sph_radius);
            int ry = std::floor(rhs.Location.y/sph_radius);
            int rz = std::floor(rhs.Location.z/sph_radius);

            if(lx<rx) return true;
            if(lx>rx) return false;
            if(ly<ry) return true;
            if(ly>ry) return false;
            if(lz<rz) return true;
            
            return false;
        };
        
        std::sort(particles.begin(),particles.end(),particlecmp);
        tbb::parallel_for(tbb::blocked_range<uint32_t>(0,particles.size(),5000),
            [&](tbb::blocked_range<uint32_t>& r){
                for(uint32_t i=r.begin();i<r.end();++i){
                    if(!particles[i].IsFluid){
                        float denominator = W_Poly6(glm::vec3(0.0f),sph_radius);
                        std::vector<uint32_t> ngbrs;
                        GetNeighbors(i,particles,sph_radius,ngbrs);
                        for(auto ngbr:ngbrs){
                            if(!particles[ngbr].IsFluid){
                                denominator += W_Poly6(particles[i].Location - particles[ngbr].Location,sph_radius);
                            }
                        }
                        particles[i].Mass = 2*rest_density/denominator;
                    }
                }
            }
        );
        renderer.SetParticles(particles,true);

        renderer.Init();
        auto now = std::chrono::high_resolution_clock::now();
        for(;;){
            auto last = now;
            now = std::chrono::high_resolution_clock::now();
            float deltatime = std::chrono::duration<float,std::chrono::seconds::period>(now-last).count();
            //UpdateTimeStepSizeCFL(0.0001,0.005,radius,particles);
            if(deltatime > 1/60.0f) deltatime = 1/60.0f;
            cubo.DeltaTime = deltatime;
            renderer.SetComputeUbo(cubo,false);
            //UPDATE NEIGHBOR SEARCHER WITH LAST FRAME
            renderer.GetParticles(particles);
            std::sort(particles.begin(),particles.end(),particlecmp);
            renderer.SetParticles(particles,false);

            renderer.Simulate();
            TickResult result = renderer.Tick(deltatime);
            switch (result)
            {
            case TickResult::EXIT:
                return EXIT_SUCCESS;
            default:
                break;
            }
        }
        renderer.Cleanup();
    }
    catch(std::runtime_error err){
        std::cerr<<err.what()<<std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}