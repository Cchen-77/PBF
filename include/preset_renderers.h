#ifndef PRESET_RENDERERS_H
#define PRESET_RENDERERS_H
#include"glm/glm.hpp"
struct RendererCaches{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;

    class PBF* pbfsimulator;
    float accumulated_time = 0.0f;
    ~RendererCaches(){
        delete pbfsimulator;
    }
};
class PresetRenderers{
public:
    static void Init_JustAGirl(class Renderer* renderer);
    static void Tick_JustAGirl(class Renderer* renderer,float Deltatime);

private:
    static RendererCaches cache;
};
#endif