#include"preset_renderers.h"
#include"renderer.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include"tiny_obj_loader.h"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include"glm/gtc/matrix_transform.hpp"

#include<vector>
#include<iostream>
#include<exception>
RendererCaches PresetRenderers::cache = {};
void PresetRenderers::Init_JustAGirl(Renderer *renderer)
{
    cache = RendererCaches{};
    tinyobj::attrib_t attrb;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err,warn;
    bool result = tinyobj::LoadObj(&attrb,&shapes,&materials,&warn,&err,"models/justagirl.obj","models");
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indexs;

    for(auto& shape:shapes){
        for(uint32_t i=0;i<shape.mesh.indices.size();++i){
            const auto& indice = shape.mesh.indices[i];
            Vertex vertex;
            vertex.Location = {
                attrb.vertices[3*indice.vertex_index+0],
                attrb.vertices[3*indice.vertex_index+1],
                attrb.vertices[3*indice.vertex_index+2],
            };
            vertex.TexCoord = {
                attrb.texcoords[2*indice.texcoord_index+0],
                attrb.texcoords[2*indice.texcoord_index+1],
            };
            switch (shape.mesh.material_ids[i/3])
			{
			case 0:
				vertex.TexCoord /= 2;
				vertex.TexCoord += glm::vec2(0.5, 0.5);
				break;
			case 1:
				vertex.TexCoord /= 2;
				vertex.TexCoord += glm::vec2(0, 0.5);
				break;
			}
			vertices.push_back(vertex);
			indexs.push_back(indexs.size());
        }
    }
    renderer->SetVertices(vertices,indexs);
    cache.model = glm::mat4(1.0f);
	cache.view = glm::lookAt(glm::vec3(-60, 50 ,200), glm::vec3(0, 50, 0), glm::vec3(0.0 ,1.0f,0.0f));
	cache.projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 1000.0f);
	cache.projection[1][1] *= -1;
    renderer->SetMVP(cache.model,cache.view,cache.projection);
}

void PresetRenderers::Tick_JustAGirl(Renderer *renderer,float DeltaTime)
{
    cache.model = glm::rotate(cache.model,DeltaTime*glm::radians(90.0f),{0,1,0});
    renderer->SetMVP(cache.model,cache.view,cache.projection);
}
