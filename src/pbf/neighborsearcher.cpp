#include"pbf/neighborsearcher.h"

#include<iostream>
#include<cmath>
NeighborSearcher::NeighborSearcher(const ParticleGroup& pg,float radius,uint32_t begin,uint32_t end):
                                    particlegroup(pg),begin(begin),end(end),
                                    radius(radius),voxelmap(1000,&VoxelHash)
{
    uint32_t n = pg.size();
    for(uint32_t i=begin;i<end;++i){
        auto& position = pg[i].position;
        glm::ivec3 voxel_indice{std::floor(position.x/radius),std::floor(position.y/radius),std::floor(position.z/radius)};
        uint32_t voxel_idx = -1;
        if(!voxelmap.count(voxel_indice)){
            voxels.push_back(voxel{});  
            voxelmap[voxel_indice] = voxels.size()-1;
            voxel_idx = voxels.size()-1;
        }
        else {
            voxel_idx = voxelmap[voxel_indice];
        }
        voxels[voxel_idx].push_back(i);
    }
    computed_neighbors.resize(n);
    for(uint32_t i=begin;i<end;++i){
        InitNeighbors(i,computed_neighbors[i]);
    }
    
}

std::vector<uint32_t> &NeighborSearcher::GetNeighbors(uint32_t idx)
{
    if(idx <begin || idx>=end){
        throw std::runtime_error("out of range idx in neighbor searching!");
    }
    return computed_neighbors[idx];
}
bool NeighborSearcher::InitNeighbors(uint32_t idx, std::vector<uint32_t> &neighbors)
{
    glm::vec3 position = particlegroup[idx].position;
    glm::ivec3 center_voxel_indice{std::floor(position.x/radius),std::floor(position.y/radius),std::floor(position.z/radius)};
    for(char da=-1;da<=1;++da)
        for(char db=-1;db<=1;++db)
            for(char dc=-1;dc<=1;++dc){
                glm::ivec3 d = {da,db,dc};
                glm::ivec3 voxel_indice = center_voxel_indice + d;
                if(!voxelmap.count(voxel_indice)) continue;
                uint32_t voxel_idx = voxelmap[voxel_indice];
                for(auto neighbor_idx:voxels[voxel_idx]){
                    if(neighbor_idx == idx) continue;
                    glm::vec3 r = position - particlegroup[neighbor_idx].position;
                    float rlength = glm::length(r);
                    if(rlength <= radius){
                        neighbors.push_back(neighbor_idx);
                    }
                }
            }
    return true;
}
