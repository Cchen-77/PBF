#ifndef NEIGHBORSEARCHER_H
#define NEIGHBORSEARCHER_H
#include"glm/glm.hpp"
#include"pbf/pbf_types.h"
#include<vector>
#include<unordered_map>
inline uint32_t VoxelHash(const glm::ivec3& v){
    return 123123123*v.x + 32154323*v.y + 45902323*v.z;
}
class NeighborSearcher{
public:
    typedef std::vector<uint32_t> voxel;
    typedef std::vector<FluidParticle> ParticleGroup;
    NeighborSearcher(const ParticleGroup& pg,float radius,uint32_t begin,uint32_t end);
    std::vector<uint32_t>& GetNeighbors(uint32_t idx);
private:
    bool InitNeighbors(uint32_t idx,std::vector<uint32_t>& neighbors);
private:
    const ParticleGroup& particlegroup;
    uint32_t begin;
    uint32_t end;
    std::unordered_map<glm::ivec3,uint32_t,uint32_t(*)(const glm::ivec3&)> voxelmap;
    std::vector<voxel> voxels; 
    float radius; 

    std::vector<std::vector<uint32_t>> computed_neighbors;
};
#endif