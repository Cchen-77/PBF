#ifndef PBF_TYPES_H
#define PBF_TYPES_H
#include<glm/glm.hpp>

#include<vector>
struct FluidParticle{
    float mass;
    glm::vec3 position;
    glm::vec3 velocity;

    
    glm::vec3 last_postion;
    glm::vec3 tmp_velocity;

    float lambda;
    float density;
    glm::vec3 delta_postion;

    glm::vec4 color;
};
#endif