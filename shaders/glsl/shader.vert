#version 450
layout(location=0) in vec3 inlocation;
layout(location=3) in vec2 intexcoord;
layout(location=3) out vec2 outtexcoord; 

layout(binding=0) uniform UniformMVPObject{
    mat4 model;
    mat4 view;
    mat4 projection;
}mvp;
void main(){
  gl_Position = mvp.projection*mvp.view*mvp.model*vec4(inlocation,1);
  outtexcoord = intexcoord;
} 