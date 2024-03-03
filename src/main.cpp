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

#define PI 3.1415926f

int main(int argc,char** argv){
    
    try{
        float radius = 0.008;
        float restDesity = 1000.0f;
        float diam = 2*radius;

        Renderer renderer = Renderer(800,800,true);

        UniformRenderingObject renderingobj{};
        renderingobj.model = glm::mat4(1.0f);
        renderingobj.view = glm::lookAt(glm::vec3(1.5,1.3,1.5),glm::vec3(0,0.3,0),glm::vec3(0,1,0));
        renderingobj.projection = glm::perspective(glm::radians(90.0f),1.0f,0.1f,10.0f);
        renderingobj.projection[1][1]*=-1;
        renderingobj.inv_projection = glm::inverse(renderingobj.projection);
        renderingobj.zNear = 0.1f;
        renderingobj.zFar = 10.0f;
        renderingobj.aspect = 1;
        renderingobj.fovy = glm::radians(90.0f);
        renderingobj.particleRadius = radius; 
        renderer.SetRenderingObj(renderingobj);
        

        UniformSimulatingObject simulatingobj{};
        simulatingobj.dt = 1/240.0f;
        simulatingobj.restDensity = 1.0f/(diam*diam*diam);
        simulatingobj.sphRadius = 4*radius;

        simulatingobj.coffPoly6 = 315.0f/(64*PI*pow(simulatingobj.sphRadius,3));
        simulatingobj.coffGradSpiky = -45/(PI*pow(simulatingobj.sphRadius,4));
        simulatingobj.coffSpiky = 15/(PI*pow(simulatingobj.sphRadius,3));

        simulatingobj.scorrK = 0.0001;
        simulatingobj.scorrQ = 0.1;
        simulatingobj.scorrN = 4;
        renderer.SetSimulatingObj(simulatingobj);
        
        UniformNSObject nsobj{};
        nsobj.sphRadius = 4*radius;
        renderer.SetNSObj(nsobj);

        UniformBoxInfoObject boxinfoobj{};
        boxinfoobj.clampX = glm::vec2{0,1.5};
        boxinfoobj.clampY = glm::vec2{0,1};
        boxinfoobj.clampZ = glm::vec2{0,1};
        boxinfoobj.clampX_still = glm::vec2{0,1.5};
        boxinfoobj.clampY_still = glm::vec2{0,1};
        boxinfoobj.clampZ_still = glm::vec2{0,1};
        renderer.SetBoxinfoObj(boxinfoobj);

        std::vector<Particle> particles;
        for(float x=0.25;x<=0.75;x+=diam){
            for(float z=0.25;z<=0.75;z+=diam){
                for(float y=0.25;y<=0.75;y+=diam){
                    Particle particle{};
                    particle.Location = glm::vec3(x,y,z);

                    particle.Mass = 1;
                    particle.NumNgbrs = 0;
                    particles.push_back(particle);
                }
            }
        }
        renderer.SetParticles(particles);

        float accumulated_time = 0.0f;
        renderer.Init();
        auto now = std::chrono::high_resolution_clock::now();
        for(;;){
            auto last = now;
            now = std::chrono::high_resolution_clock::now();
            float deltatime = std::chrono::duration<float,std::chrono::seconds::period>(now-last).count();

            float dt = std::clamp(deltatime,1/360.0f,1/60.0f);
            accumulated_time += dt;

            simulatingobj.dt = dt;
            renderer.SetSimulatingObj(simulatingobj);

            boxinfoobj.clampX.y = 1+0.25*(1-glm::cos(5*accumulated_time));
            renderer.SetBoxinfoObj(boxinfoobj);
            
            renderer.Simulate();

            auto result = renderer.TickWindow(deltatime);
            
            if(result == TickWindowResult::EXIT){
                break;
            }
            if(result != TickWindowResult::HIDE){
                renderer.Draw();
            }

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