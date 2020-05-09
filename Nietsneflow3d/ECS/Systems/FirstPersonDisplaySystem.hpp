#pragma once

#include <vector>
#include <set>
#include <BaseECS/system.hpp>
#include <OpenGLUtils/Shader.hpp>
#include <OpenGLUtils/glheaders.hpp>
#include <OpenGLUtils/VerticesData.hpp>
#include <OpenGLUtils/Texture.hpp>
#include <ECS/Systems/MapDisplaySystem.hpp>

struct GeneralCollisionComponent;

using vectUI_t = std::vector<uint32_t>;

struct EntityData
{
    float m_distance;
    Texture_t m_textureNum;
    uint32_t m_entityNum;
    EntityData(float distance, Texture_t textureNum, uint32_t entityNum) : m_distance(distance),
        m_textureNum(textureNum), m_entityNum(entityNum)
    {}

    bool operator<(const EntityData& rhs)const
    {
        return m_distance > rhs.m_distance;
    }
};

class FirstPersonDisplaySystem : public ecs::System
{
public:
    FirstPersonDisplaySystem();
    void execSystem()override;
    void setVectTextures(std::vector<Texture> &vectTexture);
    void setShader(Shader &shader);
private:
    void setUsedComponents();
    void confCompVertexMemEntities();
    void confVertex(uint32_t numEntity, GeneralCollisionComponent *genCollComp,
                    VisionComponent *visionComp, float lateralPosDegree, float distance);
    void drawVertex();
    pairFloat_t getCenterPosition(MapCoordComponent const *mapComp, GeneralCollisionComponent *genCollComp, float numEntity);
    void fillVertexFromEntitie(uint32_t numEntity, uint32_t numIteration, float distance);
private:
    Shader *m_shader;
    std::set<EntityData> m_entitiesNumMem;
    std::vector<VerticesData> m_vectVerticesData;
    std::vector<Texture> *m_ptrVectTexture = nullptr;
    //number of entity to draw per player
    vectUI_t m_numVertexToDraw;
};
