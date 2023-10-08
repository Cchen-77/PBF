#version 450
layout(location=0) in vec3 inlocation;

layout(location=0) flat out float outdepth;

layout(binding=0) uniform UniformRenderingObject{
    float zNear;
    float zFar;
    float fovy;
    float aspect;

    mat4 model;
    mat4 view;
    mat4 projection;

    float particleRadius;
};

void main(){
    vec4 viewlocation = view*model*vec4(inlocation,1); 
    vec4 location = projection*viewlocation;  
    
    outdepth = -viewlocation.z;

    gl_Position = location;

    gl_PointSize = 1;

  
} 