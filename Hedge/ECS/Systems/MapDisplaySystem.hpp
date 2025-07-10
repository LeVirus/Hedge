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
    void drawMiniMap(const PairFloat_t &centerScreenPos, const PairUI_t &min, const PairUI_t &max);
    void setShader(Shader &shader);
    void reinitMemLevelLimit();
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
    inline void memBackgroundEntity(uint32_t entity)
    {
        m_background = entity;
    }
    inline void memGroundEntity(uint32_t entity)
    {
        m_ground = entity;
    }
    inline void memMiddleEntity(uint32_t entity)
    {
        m_middle = entity;
    }
    inline uint32_t getMinLevelLock()const
    {
        return m_levelMin;
    }
private:
    void confFullMapPositionVertexEntities();
    void setUsedComponents();
    void fillMiniMapVertexFromEntities();
    void drawMapVertex();
    void confMiniMapPositionVertexEntities(const PairFloat_t &centerScreenPos, const PairUI_t &min, const PairUI_t &max);
    PairFloat_t getCenterScreen(const PairFloat_t &playerMap, const PairUI_t &min, const PairUI_t &max);
    void confMiniMapVertexElement(const PairFloat_t &glPosition, uint32_t entityNum);
    void confFullMapVertexElement(const PairFloat_t &absolutePositionPX, uint32_t entityNum);
    void setVertexStaticElementPosition(uint32_t entityNum);
    bool checkBoundEntityMap(const PairUI_t &centerScreen, const PairUI_t &minBound, const PairUI_t &maxBound);
    void getMapDisplayLimit(const PairFloat_t &playerPos, PairUI_t &min, PairUI_t &max);
    PairFloat_t getUpLeftCorner(const MapCoordComponent &mapCoordComp, uint32_t entityNum);
    void confVertexBackground();
    void confVertexGround(const PairFloat_t &centerScreenPos);
    void confVertexMiddle(const PairFloat_t &centerScreenPos);
    void updateBackgroundLateralPos();
    void drawBackground();
    void drawMiddle();
    void drawGround();
private:
    uint32_t m_playerNum;
    bool m_firstLoop = true, m_backgroundLock = false, m_freezeBackGround = false;
    float m_groundPosLateral = 0.0f, m_backgroundPosLateral = 0.0f, m_middlePosLateral = 0.0f, m_memPreviousPos;
    std::map<uint32_t, PairUI_t> m_entitiesDetectedData;
    std::vector<uint32_t> m_entitiesToDisplay;
    PairFloat_t m_sizeLevelPX, m_fullMapTileSizePX, m_fullMapTileSizeGL;
    uint32_t m_visibleTile;
    Shader *m_shader;
    std::vector<VerticesData> m_vectMapVerticesData;
    float m_localLevelSizePX;
    uint32_t m_localLevelSizeCase;
    float m_miniMapTileSizeGL;
    std::vector<Texture> *m_ptrVectTexture = nullptr;
    std::optional<uint32_t> m_background, m_ground, m_middle;
    VerticesData m_backgroundTextVertice, m_groundTextVertice, m_middleTextVertice;
    uint32_t m_levelMin;
};

//Adapt to GL context
template <typename T>
std::pair<T,T> operator-(const std::pair<T,T> & l,const std::pair<T,T> & r) {
    return {l.first - r.first,r.second - l.second};
}
