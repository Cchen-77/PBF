#version 450
layout(location=3) in vec2 intexcoord;
layout(location=0) out vec4 outcolor;

layout(binding=1) uniform sampler2D texsampler;
void main(){
    outcolor = texture(texsampler,intexcoord);
}