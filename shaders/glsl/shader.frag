#version 450
layout(location=0) out vec4 outcolor;

layout(binding=1) uniform sampler2D texsampler;
layout(depth_greater) out float gl_FragDepth;
void main(){
    vec2 pos = gl_PointCoord - vec2(0.5);
    if(length(pos) > 0.5){
        discard;
    }
    else{
        outcolor = vec4(1,0,0,1);
    }
}