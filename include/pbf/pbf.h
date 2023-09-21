#ifndef PBF_H
#define PBF_H
#include"glm/glm.hpp"
#include"pbf/sphkernel.h"
#include"pbf/pbf_types.h"


#include<vector>
#include<unordered_map>

class PBFSimulator{
public:
    typedef std::vector<FluidParticle> ParticleGroup;
    PBFSimulator(ParticleGroup& pg,uint32_t n_fluids,
    float density0=1000.0f,float particle_radius=0.1f,float vc = 0.02,
    int iter_T = 2);

    ~PBFSimulator();
    
    void Simulate();

    float CalcDensity(uint32_t idx) const;
    float CalcConstraint(uint32_t idx) const;
    glm::vec3 CalcGradConstraint(uint32_t idx,uint32_t t_idx) const;
    float CalcLagrangeMultiplier(uint32_t idx) const;
    glm::vec3 CalcDeltaPosition(uint32_t idx) const;
    glm::vec3 CalcVorticityConfinementVelocity(uint32_t idx) const;
    glm::vec3 CalcVicosityCorrectVelocity(uint32_t idx) const;

    void UpdateTimeStepSizeCFL(const float& minTimestep,const float& maxTimestep);
    ParticleGroup& particlegroup;
    uint32_t n_fluids;

    float particle_radius;
    float sph_radius;
    float rest_density;
    float vicosity_coeff;


    class NeighborSearcher* neighborsearcher = nullptr;
    class SPHKernels* sphkernels = nullptr;

    const int iter_T;

    float dt = 0.005;
};
#endif