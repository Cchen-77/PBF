#include<iostream>
#include<exception>
#include<chrono>
#include"renderer.h"
#include"preset_renderers.h"
int main(){
    try{
#ifndef DEBUG
        Renderer renderer = Renderer(800,800);
#else
        Renderer renderer = Renderer(800,800,true,"textures/justagirl.png");
#endif
        auto now = std::chrono::high_resolution_clock::now();
        PresetRenderers::Init_JustAGirl(&renderer);
        for(;;){
            auto last = now;
            now = std::chrono::high_resolution_clock::now();
            float deltatime = std::chrono::duration<float,std::chrono::seconds::period>(now-last).count();

            PresetRenderers::Tick_JustAGirl(&renderer,deltatime);
            TickResult result = renderer.Tick(deltatime);
            switch (result)
            {
            case TickResult::EXIT:
                return EXIT_SUCCESS;
            default:
                break;
            }
            
        }
    }

    catch(std::runtime_error err){
        std::cerr<<err.what()<<std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}