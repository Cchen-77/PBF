#version 450
layout(location=0) in vec3 inlocation;
layout(location=1) in uint inisfluid;
layout(binding=0) uniform UniformMVPObject{
    mat4 model;
    mat4 view;
    mat4 projection;
}mvp;

void main(){
    vec4 location = mvp.projection*mvp.view*mvp.model*vec4(inlocation,1);  
    if(inisfluid!=1){
        location.z = -1;
    }
    gl_Position = location;
    gl_PointSize = 10.0f;

  
} 