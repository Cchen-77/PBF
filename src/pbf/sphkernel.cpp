#include"pbf/sphkernel.h"
#define PI 3.1415926f
SPHKernels::SPHKernels(float h):h(h){
}
float SPHKernels::W_Poly6(const glm::vec3 &r)
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

float SPHKernels::W_Spiky(const glm::vec3 &r)
{
    static float coeff = 15.0f/(PI*std::pow(h,6));
    float rlength = glm::length(r);
    if(rlength<0||rlength>h) return 0.0f;
    float diff = h-rlength;
    float diff3 = diff*diff*diff;
    return coeff*diff3;
}

glm::vec3 SPHKernels::Grad_W_Spiky(const glm::vec3 &r)
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