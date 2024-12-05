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
#include <ECS/Components/PlayerConfComponent.hpp>
#include <ECS/Components/CircleCollisionComponent.hpp>
#include <ECS/Components/RectangleCollisionComponent.hpp>
#include <ECS/Systems/ColorDisplaySystem.hpp>
#include <constants.hpp>
#include <PhysicalEngine.hpp>

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
    PlayerConfComponent *playerConfComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerNum);
    switch(playerConfComp->m_mapMode)
    {
    case MapMode_e::NONE:
        return;
    case MapMode_e::MINI_MAP:
        drawMiniMap();
        break;
    case MapMode_e::FULL_MAP:
        drawFullMap();
        break;
    }
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(m_playerNum);
    MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(m_playerNum);
    updatePlayerArrow(*moveComp, *posComp);
}

//===================================================================
void MapDisplaySystem::drawMiniMap()
{
    confMiniMapPositionVertexEntities();
    fillMiniMapVertexFromEntities();
    drawMapVertex();
    drawPlayerOnMap();
}

//===================================================================
void MapDisplaySystem::drawFullMap()
{
    confFullMapPositionVertexEntities();
    fillFullMapVertexFromEntities();
    drawMapVertex();
    confVertexPlayerOnFullMap();
    drawPlayerOnMap();
}

//===================================================================
void MapDisplaySystem::confFullMapPositionVertexEntities()
{
    OptUint_t compNum;
    PairFloat_t corner;
    for(std::map<uint32_t, PairUI_t>::const_iterator it = m_entitiesDetectedData.begin();
        it != m_entitiesDetectedData.end();)
    {
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(it->first);
        if(!mapComp)
        {
            it = m_entitiesDetectedData.erase(it);
            continue;
        }
        //get absolute position corner
        corner = getUpLeftCorner(*mapComp, it->first);
        //convert absolute position to relative
        confFullMapVertexElement(corner, it->first);
        ++it;
    }
}

//===================================================================
void MapDisplaySystem::confVertexPlayerOnFullMap()
{
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(m_playerNum);
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerNum);
    MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(m_playerNum);
    if(posComp->m_vertex.empty())
    {
        posComp->m_vertex.resize(3);
    }
    float angle = moveComp->m_degreeOrientation;
    float radiantAngle = getRadiantAngle(angle);
    PairFloat_t GLPos = {mapComp->m_absoluteMapPositionPX.first / m_sizeLevelPX.first * FULL_MAP_SIZE_GL,
                        mapComp->m_absoluteMapPositionPX.second / m_sizeLevelPX.second * FULL_MAP_SIZE_GL};
    // ((absolutePositionPX.first / m_sizeLevelPX.first) * FULL_MAP_SIZE_GL)
    posComp->m_vertex[0].first = MAP_FULL_TOP_LEFT_X_GL + GLPos.first +
            cos(radiantAngle) * m_fullMapTileSizeGL.first;
    posComp->m_vertex[0].second = MAP_FULL_TOP_LEFT_Y_GL - GLPos.second +
            sin(radiantAngle) * m_fullMapTileSizeGL.second;
    angle += 150.0f;
    radiantAngle = getRadiantAngle(angle);

    posComp->m_vertex[1].first = MAP_FULL_TOP_LEFT_X_GL + GLPos.first +
            cos(radiantAngle) * m_fullMapTileSizeGL.first;
    posComp->m_vertex[1].second = MAP_FULL_TOP_LEFT_Y_GL - GLPos.second +
            sin(radiantAngle) * m_fullMapTileSizeGL.second;
    angle += 60.0f;
    radiantAngle = getRadiantAngle(angle);

    posComp->m_vertex[2].first = MAP_FULL_TOP_LEFT_X_GL + GLPos.first +
            cos(radiantAngle) * m_fullMapTileSizeGL.first;
    posComp->m_vertex[2].second = MAP_FULL_TOP_LEFT_Y_GL - GLPos.second +
            sin(radiantAngle) * m_fullMapTileSizeGL.second;
}

//===================================================================
void MapDisplaySystem::confMiniMapPositionVertexEntities()
{
    MapCoordComponent *mapCompPlayer = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerNum);
    PairFloat_t playerPos = mapCompPlayer->m_absoluteMapPositionPX;
    PairFloat_t corner, diffPosPX, relativePosMapGL;
    PairUI_t max, min;
    getMapDisplayLimit(playerPos, min, max);
    m_entitiesToDisplay.clear();
    m_entitiesToDisplay.reserve(mVectNumEntity.size());
    for(std::map<uint32_t, PairUI_t>::const_iterator it = m_entitiesDetectedData.begin();
         it != m_entitiesDetectedData.end(); ++it)
    {
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(it->first);
        if(!mapComp)
        {
            it = m_entitiesDetectedData.erase(it);
            continue;
        }
        if(checkBoundEntityMap(*mapComp, min, max))
        {
            //get absolute position corner
            corner = getUpLeftCorner(*mapComp, it->first);
            m_entitiesToDisplay.emplace_back(it->first);
            diffPosPX = corner - mapCompPlayer->m_absoluteMapPositionPX;
            //convert absolute position to relative
            relativePosMapGL = {diffPosPX.first * MAP_LOCAL_SIZE_GL / m_localLevelSizePX,
                                diffPosPX.second * MAP_LOCAL_SIZE_GL / m_localLevelSizePX};
            confMiniMapVertexElement(relativePosMapGL, it->first);
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
        PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(m_entitiesToDisplay[i]);
        SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(m_entitiesToDisplay[i]);
        assert(spriteComp->m_spriteData->m_textureNum < m_vectMapVerticesData.size());
        m_vectMapVerticesData[spriteComp->m_spriteData->m_textureNum].
                loadVertexStandartTextureComponent(*posComp, *spriteComp);
    }
}

//===================================================================
PairFloat_t MapDisplaySystem::getUpLeftCorner(const MapCoordComponent &mapCoordComp, uint32_t entityNum)
{
    GeneralCollisionComponent *genCollComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(entityNum);
    if(genCollComp.m_shape == CollisionShape_e::CIRCLE_C)
    {
        CircleCollisionComponent *circleCollComp = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(entityNum);
        return getCircleUpLeftCorner(mapCoordComp.m_absoluteMapPositionPX, circleCollComp->m_ray);
    }
    else
    {
        return mapCoordComp.m_absoluteMapPositionPX;
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
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(entityNum);
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
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(entityNum);
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
void MapDisplaySystem::drawMapVertex()
{
    m_shader->use();
    for(uint32_t h = 0; h < m_vectMapVerticesData.size(); ++h)
    {
        m_ptrVectTexture->operator[](h).bind();
        m_vectMapVerticesData[h].confVertexBuffer();
        m_vectMapVerticesData[h].drawElement();
    }
}

//===================================================================
void MapDisplaySystem::drawPlayerOnMap()
{
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(m_playerNum);
    ColorVertexComponent *colorComp = Ecsm_t::instance().getComponent<ColorVertexComponent, Components_e::COLOR_VERTEX_COMPONENT>(m_playerNum);
    mptrSystemManager->searchSystemByType<ColorDisplaySystem>(
                static_cast<uint32_t>(Systems_e::COLOR_DISPLAY_SYSTEM))->drawEntity(*posComp, *colorComp);
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
