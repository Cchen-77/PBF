#version 450
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
layout(binding=1) uniform UniformBoxInfoObject{
    vec2 clampX;
    vec2 clampY;
    vec2 clampZ; 

    vec2 clampX_still;
    vec2 clampY_still;
    vec2 clampZ_still; 
};

layout(location=0) out vec2 outTexcoord;

ivec3 boxCornerIndice[8] = {
    {0,0,0},
    {1,0,0},
    {0,0,1},
    {1,0,1},
    {0,1,0},
    {1,1,0},
    {0,1,1},
    {1,1,1},
};
uint hardCoded_indexbuffer[18] = {
    0,1,3,
    0,2,3,

    0,4,6,
    0,2,6,

    0,1,5,
    0,4,5,
};
vec2 hardCoded_UVs[18] = {
    {0,0},{1,0},{1,1},
    {0,0},{0,1},{1,1},

    {1,1},{1,0},{0,0},
    {1,1},{0,1},{0,0},

    {0,1},{1,1},{1,0},
    {0,1},{0,0},{1,0},
};
void main(){
    uint index = hardCoded_indexbuffer[gl_VertexIndex];
    ivec3 cornerIndice = boxCornerIndice[index];
    vec4 Location = vec4(clampX_still[cornerIndice[0]],clampY_still[cornerIndice[1]],clampZ_still[cornerIndice[2]],1);

    gl_Position = projection*view*model*Location;

    outTexcoord = hardCoded_UVs[gl_VertexIndex];

}