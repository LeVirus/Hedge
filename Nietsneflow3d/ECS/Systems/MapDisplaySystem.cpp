#include "MapDisplaySystem.hpp"
#include "Level.hpp"
#include "PictureData.hpp"
#include "CollisionUtils.hpp"
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/MapCoordComponent.hpp>
#include <ECS/Components/SpriteTextureComponent.hpp>
#include <ECS/Components/ColorVertexComponent.hpp>
#include <ECS/Components/MoveableComponent.hpp>
#include <ECS/Components/GeneralCollisionComponent.hpp>
#include <ECS/Components/CircleCollisionComponent.hpp>
#include <ECS/Components/RectangleCollisionComponent.hpp>
#include <ECS/Components/VisionComponent.hpp>
#include <ECS/Systems/ColorDisplaySystem.hpp>
#include <constants.hpp>

//===================================================================
//WARNING CONSIDER THAT LENGHT AND WEIGHT ARE THE SAME
MapDisplaySystem::MapDisplaySystem()
{
    setUsedComponents();
}

//===================================================================
void MapDisplaySystem::confLevelData()
{
    m_localLevelSizePX = Level::getRangeView() * 2;
    m_sizeLevelPX = {Level::getSize().first * LEVEL_TILE_SIZE_PX,
                    Level::getSize().second * LEVEL_TILE_SIZE_PX};
    m_miniMapTileSizeGL = (LEVEL_TILE_SIZE_PX * MAP_LOCAL_SIZE_GL) / m_localLevelSizePX;
    m_fullMapTileSizePX = {m_sizeLevelPX.first / Level::getSize().first,
                           m_sizeLevelPX.second / Level::getSize().second};
    m_fullMapTileSizeGL = {m_fullMapTileSizePX.first * FULL_MAP_SIZE_GL / m_sizeLevelPX.first,
                          m_fullMapTileSizePX.second * FULL_MAP_SIZE_GL / m_sizeLevelPX.second};
    m_entitiesDetectedData.clear();
}

//===================================================================
void MapDisplaySystem::setUsedComponents()
{
    bAddComponentToSystem(Components_e::POSITION_VERTEX_COMPONENT);
    bAddComponentToSystem(Components_e::SPRITE_TEXTURE_COMPONENT);
    bAddComponentToSystem(Components_e::MAP_COORD_COMPONENT);
}

//===================================================================
void MapDisplaySystem::setShader(Shader &shader)
{
    m_shader = &shader;
}

//===================================================================
void MapDisplaySystem::execSystem()
{
//    drawMiniMap();
    drawFullMap();
}

//===================================================================
void MapDisplaySystem::drawMiniMap()
{
    System::execSystem();
    confMiniMapPositionVertexEntities();
    fillMiniMapVertexFromEntities();
    drawMapVertex(true);
    drawPlayerOnMiniMap();
}

//===================================================================
void MapDisplaySystem::drawFullMap()
{
    confFullMapPositionVertexEntities();
    fillFullMapVertexFromEntities();
    drawMapVertex(false);
    drawPlayerOnFullMap();
}

//===================================================================
void MapDisplaySystem::confFullMapPositionVertexEntities()
{
    PairFloat_t corner, relativePosMapGL;
    for(std::map<uint32_t, PairUI_t>::const_iterator it = m_entitiesDetectedData.begin();
        it != m_entitiesDetectedData.end(); ++it)
    {
        MapCoordComponent *mapComp = stairwayToComponentManager().
                searchComponentByType<MapCoordComponent>(it->first,
                                                         Components_e::MAP_COORD_COMPONENT);
        assert(mapComp);
        //issue with BARREL !!!
        //get absolute position corner
        corner = getUpLeftCorner(mapComp, it->first);
        //convert absolute position to relative
        confFullMapVertexElement(corner, it->first);
    }
}

//===================================================================
void MapDisplaySystem::drawPlayerOnFullMap()
{

}

//===================================================================
void MapDisplaySystem::confMiniMapPositionVertexEntities()
{
    assert(m_playerComp.m_mapCoordComp);
    PairFloat_t playerPos = m_playerComp.m_mapCoordComp->m_absoluteMapPositionPX;
    PairFloat_t corner, diffPosPX, relativePosMapGL;
    PairUI_t max, min;
    getMapDisplayLimit(playerPos, min, max);
    m_entitiesToDisplay.clear();
    m_entitiesToDisplay.reserve(mVectNumEntity.size());
    for(uint32_t i = 0; i < mVectNumEntity.size(); ++i)
    {
        MapCoordComponent *mapComp = stairwayToComponentManager().
                searchComponentByType<MapCoordComponent>(mVectNumEntity[i],
                                                         Components_e::MAP_COORD_COMPONENT);
        assert(mapComp);
        GeneralCollisionComponent *genCollComp = stairwayToComponentManager().
                searchComponentByType<GeneralCollisionComponent>(mVectNumEntity[i],
                                                                 Components_e::GENERAL_COLLISION_COMPONENT);
        if(genCollComp && (genCollComp->m_tagA == CollisionTag_e::BULLET_ENEMY_CT ||
                           genCollComp->m_tagA == CollisionTag_e::BULLET_PLAYER_CT) &&
                !genCollComp->m_active)
        {
            continue;
        }
        //get absolute position corner
        if(checkBoundEntityMap(*mapComp, min, max))
        {
            corner = getUpLeftCorner(mapComp, mVectNumEntity[i]);
            m_entitiesToDisplay.emplace_back(mVectNumEntity[i]);
            diffPosPX = corner - m_playerComp.m_mapCoordComp->m_absoluteMapPositionPX;
            //convert absolute position to relative
            relativePosMapGL = {diffPosPX.first * MAP_LOCAL_SIZE_GL / m_localLevelSizePX,
                                diffPosPX.second * MAP_LOCAL_SIZE_GL / m_localLevelSizePX};
            confMiniMapVertexElement(relativePosMapGL, mVectNumEntity[i]);
        }
    }
}

//===================================================================
void MapDisplaySystem::fillMiniMapVertexFromEntities()
{
    for(uint32_t h = 0; h < m_vectMapVerticesData.size(); ++h)
    {
        m_vectMapVerticesData[h].clear();
    }
    for(uint32_t i = 0; i < m_entitiesToDisplay.size(); ++i)
    {
        PositionVertexComponent *posComp = stairwayToComponentManager().
                searchComponentByType<PositionVertexComponent>(m_entitiesToDisplay[i],
                                                               Components_e::POSITION_VERTEX_COMPONENT);
        SpriteTextureComponent *spriteComp = stairwayToComponentManager().
                searchComponentByType<SpriteTextureComponent>(m_entitiesToDisplay[i],
                                                            Components_e::SPRITE_TEXTURE_COMPONENT);
        assert(posComp);
        assert(spriteComp);
        assert(spriteComp->m_spriteData->m_textureNum < m_vectMapVerticesData.size());
        m_vectMapVerticesData[spriteComp->m_spriteData->m_textureNum].
                loadVertexStandartTextureComponent(*posComp, *spriteComp);
    }
}

//===================================================================
void MapDisplaySystem::fillFullMapVertexFromEntities()
{
    for(uint32_t h = 0; h < m_vectMapVerticesData.size(); ++h)
    {
        m_vectMapVerticesData[h].clear();
    }
    for(std::map<uint32_t, PairUI_t>::const_iterator it = m_entitiesDetectedData.begin();
        it != m_entitiesDetectedData.end(); ++it)
    {
        PositionVertexComponent *posComp = stairwayToComponentManager().
                searchComponentByType<PositionVertexComponent>(it->first,
                                                               Components_e::POSITION_VERTEX_COMPONENT);
        SpriteTextureComponent *spriteComp = stairwayToComponentManager().
                searchComponentByType<SpriteTextureComponent>(it->first,
                                                            Components_e::SPRITE_TEXTURE_COMPONENT);
        assert(posComp);
        assert(spriteComp);
        assert(spriteComp->m_spriteData->m_textureNum < m_vectMapVerticesData.size());
        m_vectMapVerticesData[spriteComp->m_spriteData->m_textureNum].
                loadVertexStandartTextureComponent(*posComp, *spriteComp);
    }
}

//===================================================================
PairFloat_t MapDisplaySystem::getUpLeftCorner(const MapCoordComponent *mapCoordComp, uint32_t entityNum)
{
    GeneralCollisionComponent *genCollComp = stairwayToComponentManager().
            searchComponentByType<GeneralCollisionComponent>(entityNum, Components_e::GENERAL_COLLISION_COMPONENT);
    assert(genCollComp);
    assert(mapCoordComp);
    if(genCollComp->m_shape == CollisionShape_e::CIRCLE_C)
    {
        CircleCollisionComponent *circleCollComp = stairwayToComponentManager().
                searchComponentByType<CircleCollisionComponent>(entityNum, Components_e::CIRCLE_COLLISION_COMPONENT);
        assert(circleCollComp);
        return getCircleUpLeftCorner(mapCoordComp->m_absoluteMapPositionPX, circleCollComp->m_ray);
    }
    else
    {
        return mapCoordComp->m_absoluteMapPositionPX;//tmp
    }
}

//===================================================================
void MapDisplaySystem::getMapDisplayLimit(PairFloat_t &playerPos,
                                          PairUI_t &min, PairUI_t &max)
{
    assert(playerPos.first >= 0.0f || playerPos.second >= 0.0f);
    //getBound
    float rangeView = Level::getRangeView();
    playerPos.first += rangeView;
    playerPos.second += rangeView;
    max = *getLevelCoord(playerPos);
    playerPos.first -= rangeView * 2;
    if(playerPos.first < LEVEL_TILE_SIZE_PX)
    {
        min.first = 0;
    }
    else
    {
        min.first = static_cast<uint32_t>(playerPos.first / LEVEL_TILE_SIZE_PX);
    }
    playerPos.second -= rangeView * 2;
    if(playerPos.second < LEVEL_TILE_SIZE_PX)
    {
        min.second = 0;
    }
    else
    {
        min.second = static_cast<uint32_t>(playerPos.second / LEVEL_TILE_SIZE_PX);
    }
}


//===================================================================
void MapDisplaySystem::confMiniMapVertexElement(const PairFloat_t &glPosition,
                                         uint32_t entityNum)
{
    PositionVertexComponent *posComp = stairwayToComponentManager().
            searchComponentByType<PositionVertexComponent>(entityNum,
                                                           Components_e::POSITION_VERTEX_COMPONENT);
    assert(posComp);
    posComp->m_vertex.resize(4);
    //CONSIDER THAT MAP X AND Y ARE THE SAME
    if(posComp->m_vertex.empty())
    {
        posComp->m_vertex.resize(4);
    }
    posComp->m_vertex[0] = {MAP_LOCAL_CENTER_X_GL + glPosition.first,
                            MAP_LOCAL_CENTER_Y_GL + glPosition.second};
    posComp->m_vertex[1] = {MAP_LOCAL_CENTER_X_GL + glPosition.first + m_miniMapTileSizeGL,
                            MAP_LOCAL_CENTER_Y_GL + glPosition.second};
    posComp->m_vertex[2] = {MAP_LOCAL_CENTER_X_GL + glPosition.first + m_miniMapTileSizeGL,
                            MAP_LOCAL_CENTER_Y_GL + glPosition.second - m_miniMapTileSizeGL};
    posComp->m_vertex[3] = {MAP_LOCAL_CENTER_X_GL + glPosition.first,
                            MAP_LOCAL_CENTER_Y_GL + glPosition.second - m_miniMapTileSizeGL};
}

//===================================================================
void MapDisplaySystem::confFullMapVertexElement(const PairFloat_t &absolutePositionPX, uint32_t entityNum)
{
    PositionVertexComponent *posComp = stairwayToComponentManager().
            searchComponentByType<PositionVertexComponent>(entityNum,
                                                           Components_e::POSITION_VERTEX_COMPONENT);
    assert(posComp);
    posComp->m_vertex.resize(4);
    //CONSIDER THAT MAP X AND Y ARE THE SAME
    if(posComp->m_vertex.empty())
    {
        posComp->m_vertex.resize(4);
    }
    double leftPos = MAP_FULL_TOP_LEFT_X_GL +
            ((absolutePositionPX.first / m_sizeLevelPX.first) * FULL_MAP_SIZE_GL),
            rightPos = leftPos + m_fullMapTileSizeGL.first,
            topPos = MAP_FULL_TOP_LEFT_Y_GL -
            ((absolutePositionPX.second  / m_sizeLevelPX.second) * FULL_MAP_SIZE_GL),
            downPos = topPos - m_fullMapTileSizeGL.second;
    posComp->m_vertex[0] = {leftPos, topPos};
    posComp->m_vertex[1] = {rightPos, topPos};
    posComp->m_vertex[2] = {rightPos, downPos};
    posComp->m_vertex[3] = {leftPos, downPos};
}

//===================================================================
bool MapDisplaySystem::checkBoundEntityMap(const MapCoordComponent &mapCoordComp,
                                           const PairUI_t &minBound,
                                           const PairUI_t &maxBound)
{
    if(mapCoordComp.m_coord.first < minBound.first ||
            mapCoordComp.m_coord.second < minBound.second)
    {
        return false;
    }
    if(mapCoordComp.m_coord.first > maxBound.first ||
            mapCoordComp.m_coord.second > maxBound.second)
    {
        return false;
    }
    return true;
}

//===================================================================
void MapDisplaySystem::drawMapVertex(bool miniMap)
{
//    drawPlayerVision();
    m_shader->use();
    for(uint32_t h = 0; h < m_vectMapVerticesData.size(); ++h)
    {
        m_ptrVectTexture->operator[](h).bind();
        m_vectMapVerticesData[h].confVertexBuffer();
        m_vectMapVerticesData[h].drawElement();
    }
}

//===================================================================
void MapDisplaySystem::drawPlayerVision()
{
    assert(m_playerComp.m_visionComp);
    mptrSystemManager->searchSystemByType<ColorDisplaySystem>(
                static_cast<uint32_t>(Systems_e::COLOR_DISPLAY_SYSTEM))->drawEntity(&m_playerComp.m_visionComp->m_positionVertexComp,
                                                             &m_playerComp.m_visionComp->m_colorVertexComp);
}

//===================================================================
void MapDisplaySystem::drawPlayerOnMiniMap()
{
    assert(m_playerComp.m_posComp);
    assert(m_playerComp.m_colorComp);
    mptrSystemManager->searchSystemByType<ColorDisplaySystem>(
                static_cast<uint32_t>(Systems_e::COLOR_DISPLAY_SYSTEM))->drawEntity(m_playerComp.m_posComp,
                                                             m_playerComp.m_colorComp);
}

//===================================================================
void MapDisplaySystem::confPlayerComp(uint32_t playerNum)
{
    m_playerComp.m_mapCoordComp = stairwayToComponentManager().
            searchComponentByType<MapCoordComponent>(playerNum,
                                                     Components_e::MAP_COORD_COMPONENT);
    m_playerComp.m_posComp = stairwayToComponentManager().
            searchComponentByType<PositionVertexComponent>(playerNum,
                                                           Components_e::POSITION_VERTEX_COMPONENT);
    m_playerComp.m_colorComp = stairwayToComponentManager().
            searchComponentByType<ColorVertexComponent>(playerNum,
                                                        Components_e::COLOR_VERTEX_COMPONENT);
    VisionComponent *visionComp = stairwayToComponentManager().
            searchComponentByType<VisionComponent>(playerNum,
                                                   Components_e::VISION_COMPONENT);
    assert(visionComp);
    visionComp->m_colorVertexComp.m_vertex.reserve(3);
    visionComp->m_colorVertexComp.m_vertex.emplace_back(0.00f, 100.00f, 0.00f, 1.0f);
    visionComp->m_colorVertexComp.m_vertex.emplace_back(0.00f, 10.00f, 0.00f, 1.0f);
    visionComp->m_colorVertexComp.m_vertex.emplace_back(0.00f, 10.00f, 0.00f, 1.0f);
    m_playerComp.m_visionComp = visionComp;
    assert(m_playerComp.m_posComp);
    assert(m_playerComp.m_colorComp);
    assert(m_playerComp.m_mapCoordComp);
}

//===================================================================
void MapDisplaySystem::setVectTextures(std::vector<Texture> &vectTexture)
{
    m_ptrVectTexture = &vectTexture;
    m_vectMapVerticesData.reserve(vectTexture.size());
    for(uint32_t h = 0; h < vectTexture.size(); ++h)
    {
        m_vectMapVerticesData.emplace_back(VerticesData(Shader_e::TEXTURE_S));
    }
}
