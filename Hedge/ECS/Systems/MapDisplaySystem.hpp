#pragma once

#include <ECS_Headers/System.hpp>
#include <functional>
#include <set>
#include <OpenGLUtils/Shader.hpp>
#include <OpenGLUtils/glheaders.hpp>
#include <OpenGLUtils/VerticesData.hpp>
#include <OpenGLUtils/Texture.hpp>
#include <constants.hpp>

struct MapCoordComponent;
struct VisionComponent;
struct MoveableComponent;
struct PlayerConfComponent;

class MapDisplaySystem : public ECS::System<Components_e::TOTAL_COMPONENTS>
{
public:
    MapDisplaySystem();
    void confLevelData();
    void setVectTextures(std::vector<Texture> &vectTexture);
    void execSystem()override;
    void drawMiniMap();
    void drawFullMap();
    void setShader(Shader &shader);
    inline bool entityAlreadyDiscovered(uint32_t entityNum)const
    {
        return m_entitiesDetectedData.find(entityNum) != m_entitiesDetectedData.end();
    }
    inline void addDiscoveredEntity(uint32_t entityNum, const PairUI_t &pos)
    {
        m_entitiesDetectedData.insert({entityNum, pos});
    }
    inline void memPlayerEntity(uint32_t entityNum)
    {
        m_playerNum = entityNum;
    }
    inline const std::map<uint32_t, PairUI_t> &getDetectedMapData()const
    {
        return m_entitiesDetectedData;
    }
    inline void clearRevealedMap()
    {
        m_entitiesDetectedData.clear();
    }
    inline const std::map<uint32_t, PairUI_t> &getRevealedMap()const
    {
        return m_entitiesDetectedData;
    }
private:
    void confFullMapPositionVertexEntities();
    void confVertexPlayerOnFullMap();
    void setUsedComponents();
    void fillMiniMapVertexFromEntities();
    void drawMapVertex();
    void drawPlayerOnMap();
    void confMiniMapPositionVertexEntities();
    void confMiniMapVertexElement(const PairFloat_t &glPosition, uint32_t entityNum);
    void confFullMapVertexElement(const PairFloat_t &absolutePositionPX, uint32_t entityNum);
    void setVertexStaticElementPosition(uint32_t entityNum);
    bool checkBoundEntityMap(const MapCoordComponent &mapCoordComp, const PairUI_t &minBound, const PairUI_t &maxBound);
    void getMapDisplayLimit(PairFloat_t &playerPos, PairUI_t &min, PairUI_t &max);
    PairFloat_t getUpLeftCorner(const MapCoordComponent &mapCoordComp, uint32_t entityNum);
private:
    uint32_t m_playerNum;
    std::map<uint32_t, PairUI_t> m_entitiesDetectedData;
    std::vector<uint32_t> m_entitiesToDisplay;
    PairFloat_t m_sizeLevelPX, m_fullMapTileSizePX, m_fullMapTileSizeGL;
    Shader *m_shader;
    std::vector<VerticesData> m_vectMapVerticesData;
    float m_localLevelSizePX;
    float m_miniMapTileSizeGL;
    std::vector<Texture> *m_ptrVectTexture = nullptr;
};

//Adapt to GL context
template <typename T>
std::pair<T,T> operator-(const std::pair<T,T> & l,const std::pair<T,T> & r) {
    return {l.first - r.first,r.second - l.second};
}
