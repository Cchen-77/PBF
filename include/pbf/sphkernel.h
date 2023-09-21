#ifndef SPHKERNEL_H
#define SPHKERNEL_H
#include"glm/glm.hpp"
class SPHKernels{
public:
    SPHKernels(float h);

    float W_Poly6(const glm::vec3& r);
    float W_Spiky(const glm::vec3& r);
    glm::vec3 Grad_W_Spiky(const glm::vec3& r);
private:
    float h;
};
#endif