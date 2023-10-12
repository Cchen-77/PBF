#version 450
layout(location=0) in vec3 inlocation;

layout(location=0) flat out float outviewdepth;

layout(binding=0) uniform UniformRenderingObject{
    float zNear;
    float zFar;
    float fovy;
    float aspect;

    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 inv_projection;

    float particleRadius;
};

void main(){
    vec4 viewlocation = view*model*vec4(inlocation,1); 
    
    outviewdepth = viewlocation.z;

    gl_Position = projection*viewlocation;

    float nearHeight = 2*zNear*tan(fovy/2);

    float scale = 800/nearHeight;

    float nearSize = particleRadius*zNear/(-outviewdepth);

    gl_PointSize = 2*scale*nearSize;
  
} 