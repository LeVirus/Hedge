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
#include <alias.hpp>
#include <cmath>

//===================================================================
//WARNING CONSIDER THAT LENGHT AND WEIGHT ARE THE SAME
MapDisplaySystem::MapDisplaySystem()
{
    setUsedComponents();
}

//===================================================================
void MapDisplaySystem::confLevelData()
{
    m_backgroundLock = false;
    m_firstLoop = true;
    m_localLevelSizePX = 350.0f;
    m_visibleTile = (m_localLevelSizePX / LEVEL_TILE_SIZE_PX + 1) * 2;
    m_localLevelSizeCase = m_localLevelSizePX / LEVEL_TILE_SIZE_PX + 1;
    m_sizeLevelPX = {Level::getSize().first * LEVEL_TILE_SIZE_PX,
                    Level::getSize().second * LEVEL_TILE_SIZE_PX};
    m_miniMapTileSizeGL = (LEVEL_TILE_SIZE_PX * MAP_LOCAL_SIZE_GL) / m_localLevelSizePX;
    m_fullMapTileSizePX = {m_sizeLevelPX.first / Level::getSize().first,
                           m_sizeLevelPX.second / Level::getSize().second};
    m_fullMapTileSizeGL = {m_fullMapTileSizePX.first * FULL_MAP_SIZE_GL / m_sizeLevelPX.first,
                          m_fullMapTileSizePX.second * FULL_MAP_SIZE_GL / m_sizeLevelPX.second};
    m_background = std::nullopt;
}

//===================================================================
void MapDisplaySystem::setUsedComponents()
{
    addComponentsToSystem(Components_e::POSITION_VERTEX_COMPONENT, 1);
    addComponentsToSystem(Components_e::SPRITE_TEXTURE_COMPONENT, 1);
    addComponentsToSystem(Components_e::MAP_COORD_COMPONENT, 1);
}

//===================================================================
void MapDisplaySystem::setShader(Shader &shader)
{
    m_shader = &shader;
}

//===================================================================
void MapDisplaySystem::reinitMemLevelLimit()
{
    m_levelMin = 0;
    m_freezeBackGround = false;
}

//===================================================================
void MapDisplaySystem::execSystem()
{
    MapCoordComponent *mapCompPlayer = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerNum);
    PairFloat_t playerPos = mapCompPlayer->m_absoluteMapPositionPX;
    PairUI_t max = Level::getSize(), min = {0, 0};
    if(Level::getScrollingLock())
    {
        // min.first
        if(mapCompPlayer->m_absoluteMapPositionPX.first - m_localLevelSizePX > m_levelMin)
        {
            m_levelMin = mapCompPlayer->m_absoluteMapPositionPX.first - m_localLevelSizePX;
            if(m_levelMin > m_sizeLevelPX.first - m_localLevelSizePX * 2)
            {
                m_levelMin = m_sizeLevelPX.first - m_localLevelSizePX * 2;
            }
            m_freezeBackGround = false;
        }
        else
        {
            m_freezeBackGround = true;
        }
    }
    PairFloat_t centerScreen = getCenterScreen(mapCompPlayer->m_absoluteMapPositionPX, min, max);
    getMapDisplayLimit(centerScreen, min, max);
    confVertexBackground();
    confVertexGround(centerScreen);
    confVertexMiddle(centerScreen);
    drawBackground();
    drawMiddle();
    drawMiniMap(centerScreen, min, max);
    drawGround();
}

//===================================================================
void MapDisplaySystem::drawMiniMap(const PairFloat_t &centerScreenPos, const PairUI_t &min, const PairUI_t &max)
{
    confMiniMapPositionVertexEntities(centerScreenPos, min, max);
    fillMiniMapVertexFromEntities();
    drawMapVertex();
}

//===================================================================
void MapDisplaySystem::confFullMapPositionVertexEntities()
{
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
void MapDisplaySystem::confMiniMapPositionVertexEntities(const PairFloat_t &centerScreenPos, const PairUI_t &min, const PairUI_t &max)
{
    PairFloat_t corner, diffPosPX, relativePosMapGL;
    m_entitiesToDisplay.clear();
    m_entitiesToDisplay.reserve(m_usedEntities.size());
    for(std::set<uint32_t>::const_iterator it = m_usedEntities.begin(); it != m_usedEntities.end(); ++it)
    {
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(*it);
        if(!mapComp)
        {
            it = m_usedEntities.erase(it);
            continue;
        }
        if(m_playerNum == *it)
        {
            PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerNum);
            assert(playerComp);
            std::optional<PairUI_t> coord = getLevelCoord(mapComp->m_absoluteMapPositionPX);
            assert(coord);
            mapComp->m_coord = *coord;
            if(playerComp->m_associatedVehicle)
            {
                continue;
            }
        }
        GeneralCollisionComponent *genComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(*it);
        assert(genComp);
        if(!genComp->m_active)
        {
            continue;
        }
        if(checkBoundEntityMap(*getLevelCoord(mapComp->m_absoluteMapPositionPX), min, max))
        {
            if(genComp->m_tagA == CollisionTag_e::VEHICULE_CT)
            {
                RectangleCollisionComponent *rectComp = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(*it);
                assert(rectComp);
                if(rectComp->m_size.first > 50)
                {
                    VehicleComponent *vehicleComp = Ecsm_t::instance().getComponent<VehicleComponent, Components_e::VEHICLE_COMPONENT>(*it);
                    assert(vehicleComp);
                    //If on stair
                    if(vehicleComp->m_currentSpritesType != VehicleSpriteType_e::MOVE_RIGHT && vehicleComp->m_currentSpritesType != VehicleSpriteType_e::MOVE_LEFT &&
                        vehicleComp->m_currentSpritesType != VehicleSpriteType_e::SHOOT_MOVE_RIGHT && vehicleComp->m_currentSpritesType != VehicleSpriteType_e::SHOOT_MOVE_LEFT)
                    {
                        MapCoordComponent *mapCompB = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(*it, 1);
                        assert(mapCompB);
                        mapCompB->m_absoluteMapPositionPX = {mapComp->m_absoluteMapPositionPX.first, mapComp->m_absoluteMapPositionPX.second + rectComp->m_size.second / 2};
                        mapComp = mapCompB;
                        vehicleComp->m_collDownActive = true;
                        // diffPosPX.second -= rectComp->m_size.second / 2;
                    }
                    else
                    {
                        vehicleComp->m_collDownActive = false;
                    }
                }
            }
            //get absolute position corner
            m_entitiesToDisplay.emplace_back(*it);
            corner = getUpLeftCorner(*mapComp, *it);
            diffPosPX = corner - centerScreenPos;
            //convert absolute position to relative
            relativePosMapGL = {diffPosPX.first * MAP_LOCAL_SIZE_GL / m_localLevelSizePX,
                                diffPosPX.second * MAP_LOCAL_SIZE_GL / m_localLevelSizePX};

            EnemyConfComponent *enemyComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(*it);
            if(enemyComp && enemyComp->m_life > 0)
            {
                enemyComp->m_behaviourMode = EnemyBehaviourMode_e::ATTACK;
            }
            confMiniMapVertexElement(relativePosMapGL, *it);
        }
        else
        {
            EnemyConfComponent *enemyComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(*it);
            if(enemyComp && enemyComp->m_life > 0)
            {
                enemyComp->m_behaviourMode = EnemyBehaviourMode_e::PASSIVE;
            }
        }
    }
}

//===================================================================
PairFloat_t MapDisplaySystem::getCenterScreen(const PairFloat_t &playerMap, const PairUI_t &min, const PairUI_t &max)
{
    PairFloat_t finalPos = playerMap;
    m_freezeBackGround = false;
    if(Level::getScrollingLock())
    {
        finalPos.first = m_levelMin + m_localLevelSizePX;
    }
    else if(min.first == 0 && finalPos.first - m_localLevelSizePX < 0.0f)
    {
        finalPos.first = m_localLevelSizePX;
        m_freezeBackGround = true;
    }
    else if(max.first >= Level::getSize().first && finalPos.first + m_localLevelSizePX > m_sizeLevelPX.first)
    {
        m_freezeBackGround = true;
        finalPos.first = m_sizeLevelPX.first - m_localLevelSizePX;
    }
    //if Scrolling lock active
    if(min.second == 0 && finalPos.second - m_localLevelSizePX < 0.0f)
    {
        finalPos.second = m_localLevelSizePX;
    }
    else if(max.second >= Level::getSize().second && finalPos.second + m_localLevelSizePX > m_sizeLevelPX.second)
    {
        finalPos.second = m_sizeLevelPX.second - m_localLevelSizePX;
    }
    return finalPos;
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
        GeneralCollisionComponent *genComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(m_entitiesToDisplay[i]);
        assert(genComp);
        if(!genComp->m_active)
        {
            continue;
        }
        PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(m_entitiesToDisplay[i]);
        SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(m_entitiesToDisplay[i]);
        assert(posComp);
        assert(spriteComp);
        assert(spriteComp->m_spriteData->m_textureNum < m_vectMapVerticesData.size());
        m_vectMapVerticesData[spriteComp->m_spriteData->m_textureNum].
                loadVertexStandartTextureComponent(*posComp, *spriteComp);
    }
}

//===================================================================
PairFloat_t MapDisplaySystem::getUpLeftCorner(const MapCoordComponent &mapCoordComp, uint32_t entityNum)
{
    GeneralCollisionComponent *genCollComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(entityNum);
    if(genCollComp->m_shape == CollisionShape_e::CIRCLE_C)
    {
        CircleCollisionComponent *circleCollComp = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(entityNum);
        assert(circleCollComp);
        return getCircleUpLeftCorner(mapCoordComp.m_absoluteMapPositionPX, circleCollComp->m_ray);
    }
    else
    {
        return mapCoordComp.m_absoluteMapPositionPX;
    }
}

//===================================================================
void MapDisplaySystem::confVertexBackground()
{
    if(!m_firstLoop && m_backgroundLock)
    {
        m_backgroundLock = false;
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerNum);
        assert(mapComp);
        m_memPreviousPos = mapComp->m_absoluteMapPositionPX.first;
        return;
    }
    updateBackgroundLateralPos();
    float leftPos = m_backgroundPosLateral - 2.0f, rightPos = m_backgroundPosLateral + 2.0f;
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(*m_background);
    assert(posComp);
    //BACKGROUND
    posComp->m_vertex[0].first = leftPos;
    posComp->m_vertex[3].first = leftPos;
    posComp->m_vertex[1].first = m_backgroundPosLateral;
    posComp->m_vertex[2].first = m_backgroundPosLateral;
    posComp->m_vertex[4].first = rightPos;
    posComp->m_vertex[5].first = rightPos;

    leftPos = m_middlePosLateral - 2.0f;
    rightPos = m_middlePosLateral + 2.0f;
    //MIDDLE
    posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(*m_middle);
    assert(posComp);
    posComp->m_vertex[0].first = leftPos;
    posComp->m_vertex[3].first = leftPos;
    posComp->m_vertex[1].first = m_middlePosLateral;
    posComp->m_vertex[2].first = m_middlePosLateral;
    posComp->m_vertex[4].first = rightPos;
    posComp->m_vertex[5].first = rightPos;
}

//===================================================================
void MapDisplaySystem::confVertexGround(const PairFloat_t &centerScreenPos)
{
    //GROUND
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(*m_ground);
    assert(posComp);
    uint32_t levelSizeY = Level::getSize().second;
    float posDownScreen = centerScreenPos.second + m_localLevelSizePX;
    float posGround = (levelSizeY - 5) * LEVEL_TILE_SIZE_PX;
    float diffPosPX = posDownScreen - posGround;
    float leftPos = m_groundPosLateral - 2.0f, rightPos = m_groundPosLateral + 2.0f;
    float groundGLy = -1.0f + (diffPosPX * MAP_LOCAL_SIZE_GL / m_localLevelSizePX), groundDownPos = groundGLy - (LEVEL_TILE_SIZE_PX * 5 * MAP_LOCAL_SIZE_GL) / m_localLevelSizePX;
    posComp->m_vertex[0].first = leftPos;
    posComp->m_vertex[3].first = leftPos;
    posComp->m_vertex[1].first = m_groundPosLateral;
    posComp->m_vertex[2].first = m_groundPosLateral;
    posComp->m_vertex[4].first = rightPos;
    posComp->m_vertex[5].first = rightPos;

    posComp->m_vertex[0].second = groundGLy;
    posComp->m_vertex[1].second = groundGLy;
    posComp->m_vertex[4].second = groundGLy;

    posComp->m_vertex[2].second = groundDownPos;
    posComp->m_vertex[3].second = groundDownPos;
    posComp->m_vertex[5].second = groundDownPos;
}

//===================================================================
void MapDisplaySystem::confVertexMiddle(const PairFloat_t &centerScreenPos)
{
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(*m_middle);
    assert(posComp);
    float groundGLy = (centerScreenPos.second * MAP_LOCAL_SIZE_GL / (Level::getSize().second * LEVEL_TILE_SIZE_PX)), groundDownPos = groundGLy - 1.8f;

    posComp->m_vertex[0].second = groundGLy;
    posComp->m_vertex[1].second = groundGLy;
    posComp->m_vertex[4].second = groundGLy;

    posComp->m_vertex[2].second = groundDownPos;
    posComp->m_vertex[3].second = groundDownPos;
    posComp->m_vertex[5].second = groundDownPos;
}

//===================================================================
void MapDisplaySystem::updateBackgroundLateralPos()
{
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerNum);
    assert(mapComp);
    if(!m_firstLoop && !m_freezeBackGround)
    {
        //BACKGROUND
        m_backgroundPosLateral += (m_memPreviousPos - mapComp->m_absoluteMapPositionPX.first) / (m_localLevelSizePX * 1.5f);
        if(m_backgroundPosLateral <= -1.00f)
        {
            m_backgroundPosLateral = 1.0f + std::fmod(m_backgroundPosLateral, 1.00f);
        }
        else if(m_backgroundPosLateral >= 1.00f)
        {
            m_backgroundPosLateral = -1.0f + std::fmod(m_backgroundPosLateral, 1.00f);
        }

        //MIDDLE
        m_middlePosLateral += (m_memPreviousPos - mapComp->m_absoluteMapPositionPX.first) / (m_localLevelSizePX);
        if(m_middlePosLateral <= -1.00f)
        {
            m_middlePosLateral = 1.0f + std::fmod(m_middlePosLateral, 1.00f);
        }
        else if(m_middlePosLateral >= 1.00f)
        {
            m_middlePosLateral = -1.0f + std::fmod(m_middlePosLateral, 1.00f);
        }

        //GROUND
        m_groundPosLateral += (m_memPreviousPos - mapComp->m_absoluteMapPositionPX.first) / m_localLevelSizePX;
        if(m_groundPosLateral <= -1.00f)
        {
            m_groundPosLateral = 1.0f + std::fmod(m_groundPosLateral, 1.00f);
        }
        else if(m_groundPosLateral >= 1.00f)
        {
            m_groundPosLateral = -1.0f + std::fmod(m_groundPosLateral, 1.00f);
        }
    }
    else
    {
        m_firstLoop = false;
    }
    m_memPreviousPos = mapComp->m_absoluteMapPositionPX.first;
}

//===================================================================
void MapDisplaySystem::drawBackground()
{
    m_shader->use();
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(*m_background);
    assert(posComp);
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(*m_background);
    assert(spriteComp);
    m_ptrVectTexture->operator[](static_cast<uint32_t>(spriteComp->m_spriteData->m_textureNum)).bind();
    m_backgroundTextVertice.clear();
    m_backgroundTextVertice.loadVertexStandartTextureComponent(*posComp, *spriteComp);
    m_backgroundTextVertice.confVertexBuffer();
    m_backgroundTextVertice.drawElement();

}

//===================================================================
void MapDisplaySystem::drawMiddle()
{
    m_shader->use();
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(*m_middle);
    assert(posComp);
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(*m_middle);
    assert(spriteComp);
    m_ptrVectTexture->operator[](static_cast<uint32_t>(spriteComp->m_spriteData->m_textureNum)).bind();
    m_backgroundTextVertice.clear();
    m_backgroundTextVertice.loadVertexStandartTextureComponent(*posComp, *spriteComp);
    m_backgroundTextVertice.confVertexBuffer();
    m_backgroundTextVertice.drawElement();
}

//===================================================================
void MapDisplaySystem::drawGround()
{
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(*m_ground);
    assert(posComp);
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(*m_ground);
    assert(spriteComp);
    m_ptrVectTexture->operator[](static_cast<uint32_t>(spriteComp->m_spriteData->m_textureNum)).bind();
    m_groundTextVertice.clear();
    m_groundTextVertice.loadVertexStandartTextureComponent(*posComp, *spriteComp);
    m_groundTextVertice.confVertexBuffer();
    m_groundTextVertice.drawElement();
}

//===================================================================
void MapDisplaySystem::getMapDisplayLimit(const PairFloat_t &playerPos, PairUI_t &min, PairUI_t &max)
{
    assert(playerPos.first >= 0.0f || playerPos.second >= 0.0f);
    float correctedLevelSize = m_localLevelSizePX + LEVEL_TILE_SIZE_PX;
    //getBound
    PairFloat_t posMax = {playerPos.first + correctedLevelSize, playerPos.second + correctedLevelSize},
        posMin = {playerPos.first - correctedLevelSize, playerPos.second - correctedLevelSize};
    max = *getLevelCoord(posMax);
    if(posMin.first < 0.0f)
    {
        min.first = 0;
        max.first = m_visibleTile;
        m_backgroundLock = true;
    }
    else if(posMax.first >= m_sizeLevelPX.first)
    {
        max.first = Level::getSize().first;
        min.first = max.first - m_visibleTile;
        m_backgroundLock = true;
    }
    else
    {
        min.first = static_cast<uint32_t>(posMin.first / LEVEL_TILE_SIZE_PX);
    }
    if(posMin.second < 0.0f)
    {
        min.second = 0;
        max.second = m_visibleTile;
    }
    else if(posMax.second >= m_sizeLevelPX.second)
    {
        max.second = Level::getSize().second;
        min.second = max.second - m_visibleTile;
    }
    else
    {
        min.second = static_cast<uint32_t>(posMin.second / LEVEL_TILE_SIZE_PX);
    }
}

//===================================================================
void MapDisplaySystem::confMiniMapVertexElement(const PairFloat_t &glPosition, uint32_t entityNum)
{
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(entityNum);
    assert(posComp);
    posComp->m_vertex.resize(4);
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(entityNum);
    assert(spriteComp);
    if(!spriteComp->m_displaySize)
    {
        //CONSIDER THAT MAP X AND Y ARE THE SAME
        if(posComp->m_vertex.empty())
        {
            posComp->m_vertex.resize(4);
        }
        posComp->m_vertex[0] = {glPosition.first, glPosition.second};
        posComp->m_vertex[1] = {glPosition.first + m_miniMapTileSizeGL, glPosition.second};
        posComp->m_vertex[2] = {glPosition.first + m_miniMapTileSizeGL, glPosition.second - m_miniMapTileSizeGL};
        posComp->m_vertex[3] = {glPosition.first, glPosition.second - m_miniMapTileSizeGL};
    }
    else
    {
        GeneralCollisionComponent *Comp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(entityNum);
        assert(Comp);
        float sizeX = spriteComp->m_displaySize->first * (LEVEL_TILE_SIZE_PX * MAP_LOCAL_SIZE_GL) / m_localLevelSizePX,
            sizeY = spriteComp->m_displaySize->second * (LEVEL_TILE_SIZE_PX * MAP_LOCAL_SIZE_GL) / m_localLevelSizePX;
        posComp->m_vertex[0] = {glPosition.first, glPosition.second};
        posComp->m_vertex[1] = {glPosition.first + sizeX, glPosition.second};
        posComp->m_vertex[2] = {glPosition.first + sizeX, glPosition.second - sizeY};
        posComp->m_vertex[3] = {glPosition.first, glPosition.second - sizeY};
    }
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
bool MapDisplaySystem::checkBoundEntityMap(const PairUI_t &centerScreen,
                                           const PairUI_t &minBound,
                                           const PairUI_t &maxBound)
{
    if(centerScreen.first < minBound.first ||
            centerScreen.second < minBound.second)
    {
        return false;
    }
    if(centerScreen.first > maxBound.first ||
            centerScreen.second > maxBound.second)
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
void MapDisplaySystem::setVectTextures(std::vector<Texture> &vectTexture)
{
    m_ptrVectTexture = &vectTexture;
    m_vectMapVerticesData.reserve(vectTexture.size());
    for(uint32_t h = 0; h < vectTexture.size(); ++h)
    {
        m_vectMapVerticesData.emplace_back(VerticesData(Shader_e::TEXTURE_S));
    }
}
