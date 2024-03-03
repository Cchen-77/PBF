#version 450
layout(binding=2) uniform sampler2D defaultTexture;

layout(location=0) in vec2 inTexcoord;

layout(location=0) out vec4 outcolor;
void main(){
    outcolor = texture(defaultTexture,inTexcoord);
}