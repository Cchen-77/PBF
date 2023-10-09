#version 450
layout(location=0) out float outdepth;
layout(location=1) out float outthickness;

layout(location=0) flat in float indepth;

layout(binding=0) uniform UniformRenderingObject{
    float zNear;
    float zFar;
    float fovy;
    float aspect;

    mat4 model;
    mat4 view;
    mat4 projection;

    float particleRadius;
} ;

void main(){
    float radius = particleRadius;
    vec2 pos = gl_PointCoord - vec2(0.5);
    if(length(pos) > 0.5){
        discard;
        return;
    }
    float l = radius*2*length(pos);
    float depth = indepth - sqrt(radius*radius - l*l);
    
    outdepth = depth;

    outthickness = 0;

    //outthickness = 2*sqrt(radius*radius - l*l);
    
}