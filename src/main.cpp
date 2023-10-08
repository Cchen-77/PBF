#include"renderer.h"
#include"renderer_types.h"
#include"glm/gtc/matrix_transform.hpp"

#include<iostream>
#include<exception>
#include<chrono>
#include<string>
#include<algorithm>

#undef APIENTRY
#define NOMINMAX
#include"tbb/tbb.h"

#define PI 3.1415926

int main(int argc,char** argv){
    
    try{
        float radius = 0.01;
        float restDesity = 1000.0f;
        float diam = 2*radius;

        Renderer renderer = Renderer(800,800,true);

        UniformRenderingObject renderingobj{};
        renderingobj.model = glm::mat4(1.0f);
        renderingobj.view = glm::lookAt(glm::vec3(1,1,1),glm::vec3(0,0.5,0),glm::vec3(0,1,0));
        renderingobj.projection = glm::perspective(glm::radians(90.0f),1.0f,0.1f,100.0f);
        renderingobj.projection[1][1]*=-1;
        renderingobj.zNear = 0.1f;
        renderingobj.zFar = 10.0f;
        renderingobj.aspect = 1;
        renderingobj.fovy = glm::radians(90.0f);
        renderingobj.particleRadius = radius; 
        renderer.SetRenderingObj(renderingobj);
        

        UniformSimulatingObject simulatingobj{};
        simulatingobj.dt = 0.0001;
        simulatingobj.restDensity = 1000.0f;
        simulatingobj.sphRadius = 4*radius;
        renderer.SetSimulatingObj(simulatingobj);
        
        UniformNSObject nsobj{};
        nsobj.sphRadius = 4*radius;
        renderer.SetNSObj(nsobj);

        std::vector<Particle> particles;
        for(float x=0;x<=0.5;x+=diam){
            for(float z=0;z<=0.5;z+=diam){
                for(float y=0.25;y<=0.75;y+=diam){
                    Particle particle{};
                    particle.Location = glm::vec3(x,y,z);

                    particle.Mass = restDesity*diam*diam*diam;
                    particle.NumNgbrs = 0;
                    particles.push_back(particle);
                }
            }
        }
        renderer.SetParticles(particles);

    

        renderer.Init();
        auto now = std::chrono::high_resolution_clock::now();
        for(int i=0;;++i){
            auto last = now;
            now = std::chrono::high_resolution_clock::now();
            float deltatime = std::chrono::duration<float,std::chrono::seconds::period>(now-last).count();

            renderer.Simulate();

            auto result = renderer.TickWindow(deltatime);
            
            if(result == TickWindowResult::EXIT){
                break;
            }
            if(result != TickWindowResult::HIDE){
                renderer.Draw();
            }
            std::vector<int> indexs;

            printf("%f\n",1/deltatime);
        }

        renderer.Cleanup();
    }
    catch(std::runtime_error err){
        std::cerr<<err.what()<<std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}