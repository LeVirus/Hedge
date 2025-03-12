#include "PlatformSystem.hpp"

#include "constants.hpp"
#include <Level.hpp>
#include <CollisionUtils.hpp>
#include <MainEngine.hpp>
#include <ECS/Components/TimerComponent.hpp>
#include <ECS/Components/MapCoordComponent.hpp>
#include <ECS/Components/RectangleCollisionComponent.hpp>
#include <ECS/Components/MoveableWallComponent.hpp>
#include <ECS/Components/MoveableComponent.hpp>
#include <ECS/Components/GeneralCollisionComponent.hpp>
#include <ECS/Components/AudioComponent.hpp>
#include <cassert>
#include <alias.hpp>

//===================================================================
PlatformSystem::PlatformSystem()
{
    addComponentsToSystem(Components_e::MOVEABLE_WALL_CONF_COMPONENT, 1);
}

void PlatformSystem::execSystem()
{
    if(m_usedEntities.empty())
    {
        Ecsm_t::instance().updateEntitiesFromSystem(static_cast<uint32_t>(Systems_e::PLATFORM_SYSTEM));
    }
    // treatTriggers();
    treatMoveableWalls();
}

//===================================================================
void PlatformSystem::activeDoorSound(uint32_t entityNum)
{
    AudioComponent *audioComp = Ecsm_t::instance().getComponent<AudioComponent, Components_e::AUDIO_COMPONENT>(entityNum);
    assert(audioComp);
    audioComp->m_soundElements[0]->m_toPlay = true;
}

//===================================================================
void PlatformSystem::treatMoveableWalls()
{
    bool next, playerMovedLateral = false;
    PairUI_t memPreviousPos;
    for(std::set<uint32_t>::const_iterator it = m_usedEntities.begin(); it != m_usedEntities.end(); ++it)
    {
        MoveableWallConfComponent *moveWallComp = Ecsm_t::instance().getComponent<MoveableWallConfComponent, Components_e::MOVEABLE_WALL_CONF_COMPONENT>(*it);
        assert(moveWallComp);
        if(!moveWallComp->m_inMovement)
        {
            if(!moveWallComp->m_manualTrigger)
            {
                continue;
            }
            else
            {
                moveWallComp->m_manualTrigger = false;
                triggerMoveableWall(*it);
            }
        }
        next = false;

        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(*it);
        assert(mapComp);
        Direction_e currentDir = moveWallComp->m_directionMove[moveWallComp->m_currentPhase].first;
        if((currentDir == Direction_e::WEST && mapComp->m_coord.first == 0) ||
            (currentDir == Direction_e::NORTH && mapComp->m_coord.second == 0) ||
            (currentDir == Direction_e::SOUTH && mapComp->m_coord.second == Level::getSize().second - 1) ||
            (currentDir == Direction_e::EAST && mapComp->m_coord.first == Level::getSize().first - 1))
        {
            stopMoveWallLevelLimitCase(*mapComp, *moveWallComp, *it);
            continue;
        }

        MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(*it);
        assert(moveComp);
        if(moveWallComp->m_initPos)
        {
            setInitPhaseMoveWall(*mapComp, *moveWallComp, currentDir, *it);
        }
        memPreviousPos = mapComp->m_coord;
        switch(currentDir)
        {
        case Direction_e::EAST:
        {
            mapComp->m_absoluteMapPositionPX.first += moveComp->m_velocity;
            if(mapComp->m_absoluteMapPositionPX.first >= moveWallComp->m_nextPhasePos.first)
            {
                ++mapComp->m_coord.first;
                next = true;
            }
            if(!playerMovedLateral && m_currentWallPlayerOnGround && *m_currentWallPlayerOnGround == *it)
            {
                assert(m_playerEntity != 10000);
                MapCoordComponent *mapPlayerComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerEntity);
                assert(mapPlayerComp);
                mapPlayerComp->m_absoluteMapPositionPX.first += moveComp->m_velocity;
                ++mapPlayerComp->m_absoluteMapPositionPX.second;
                GravityComponent *gravityComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(m_playerEntity);
                assert(gravityComp);
                gravityComp->m_fall = false;
                playerMovedLateral = true;
            }
            break;
        }
        case Direction_e::WEST:
        {
            mapComp->m_absoluteMapPositionPX.first -= moveComp->m_velocity;
            if(mapComp->m_absoluteMapPositionPX.first <= moveWallComp->m_nextPhasePos.first)
            {
                --mapComp->m_coord.first;
                next = true;
            }
            if(!playerMovedLateral && m_currentWallPlayerOnGround && *m_currentWallPlayerOnGround == *it)
            {
                assert(m_playerEntity != 10000);
                MapCoordComponent *mapPlayerComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerEntity);
                assert(mapPlayerComp);
                mapPlayerComp->m_absoluteMapPositionPX.first -= moveComp->m_velocity;
                ++mapPlayerComp->m_absoluteMapPositionPX.second;
                GravityComponent *gravityComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(m_playerEntity);
                assert(gravityComp);
                gravityComp->m_fall = false;
                playerMovedLateral = true;
            }
            break;
        }
        case Direction_e::NORTH:
            mapComp->m_absoluteMapPositionPX.second -= moveComp->m_velocity;
            if(mapComp->m_absoluteMapPositionPX.second <= moveWallComp->m_nextPhasePos.second)
            {
                --mapComp->m_coord.second;
                next = true;
            }
            break;
        case Direction_e::SOUTH:
        {
            mapComp->m_absoluteMapPositionPX.second += moveComp->m_velocity;
            if(mapComp->m_absoluteMapPositionPX.second >= moveWallComp->m_nextPhasePos.second)
            {
                ++mapComp->m_coord.second;
                next = true;
            }
            if(m_currentWallPlayerOnGround && *m_currentWallPlayerOnGround == *it)
            {
                assert(m_playerEntity != 10000);
                MapCoordComponent *mapPlayerComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerEntity);
                assert(mapPlayerComp);
                mapPlayerComp->m_absoluteMapPositionPX.second += moveComp->m_velocity + 1.0f;
                GravityComponent *gravityComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(m_playerEntity);
                assert(gravityComp);
                gravityComp->m_fall = false;
            }
            break;
        }
        }
        if(next)
        {
            if(m_currentWallPlayerOnGround && *m_currentWallPlayerOnGround == *it && (currentDir == Direction_e::EAST || currentDir == Direction_e::WEST))
            {
                PairFloat_t correctedPos = getAbsolutePosition(mapComp->m_coord);
                MapCoordComponent *mapPlayerComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerEntity);
                assert(mapPlayerComp);
                if(currentDir == Direction_e::WEST)
                {
                    mapPlayerComp->m_absoluteMapPositionPX.first += /*std::abs*/(correctedPos.first - mapComp->m_absoluteMapPositionPX.first);
                }
                else
                {
                    mapPlayerComp->m_absoluteMapPositionPX.first -= std::abs(mapComp->m_absoluteMapPositionPX.first - correctedPos.first);
                }
            }
            mapComp->m_absoluteMapPositionPX = getAbsolutePosition(mapComp->m_coord);
            switchToNextPhaseMoveWall(*it, *mapComp, *moveWallComp, memPreviousPos, *it);
            if(!moveWallComp->m_initPos && moveWallComp->m_triggerBehaviour == TriggerBehaviourType_e::AUTO)
            {
                setInitPhaseMoveWall(*mapComp, *moveWallComp, currentDir, *it);
            }
        }
    }
}


//===================================================================
void PlatformSystem::triggerMoveableWall(uint32_t wallEntity)
{
    MoveableWallConfComponent *moveableWallComp = Ecsm_t::instance().getComponent<MoveableWallConfComponent, Components_e::MOVEABLE_WALL_CONF_COMPONENT>(wallEntity);
    assert(moveableWallComp);
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(wallEntity);
    assert(mapComp);
    moveableWallComp->m_actionned = true;
    std::optional<ElementRaycast> element = Level::getElementCase(mapComp->m_coord);
    //init move wall case
    if(Level::getElementCase(mapComp->m_coord)->m_typeStd != LevelCaseType_e::WALL_LC)
    {
        Level::memMoveWallEntity(mapComp->m_coord, wallEntity);
    }
    if(!Level::removeStaticMoveWallElementCase(mapComp->m_coord, wallEntity))
    {
        if(element && element->m_typeStd != LevelCaseType_e::WALL_LC)
        {
            Level::setElementTypeCase(mapComp->m_coord, LevelCaseType_e::EMPTY_LC);
        }
    }
    else
    {
        Level::setElementTypeCase(mapComp->m_coord, LevelCaseType_e::WALL_LC);
    }
    moveableWallComp->m_cycleInMovement = true;
    moveableWallComp->m_inMovement = true;
    moveableWallComp->m_initPos = true;
    moveableWallComp->m_currentPhase = 0;
    moveableWallComp->m_currentMove = 0;
}


//===================================================================
void setInitPhaseMoveWall(MapCoordComponent &mapComp, MoveableWallConfComponent &moveWallComp,
                          Direction_e currentDir, uint32_t wallEntity)
{
    PairUI_t nextCase;
    if(Level::getElementCase(mapComp.m_coord)->m_type != LevelCaseType_e::WALL_LC)
    {
        Level::setElementTypeCase(mapComp.m_coord, LevelCaseType_e::WALL_MOVE_LC);
    }
    moveWallComp.m_initPos = false;
    switch(currentDir)
    {
    case Direction_e::EAST:
        nextCase = {mapComp.m_coord.first + 1, mapComp.m_coord.second};
        break;
    case Direction_e::WEST:
        nextCase = {mapComp.m_coord.first - 1, mapComp.m_coord.second};
        break;
    case Direction_e::NORTH:
        nextCase = {mapComp.m_coord.first, mapComp.m_coord.second - 1};
        break;
    case Direction_e::SOUTH:
        nextCase = {mapComp.m_coord.first, mapComp.m_coord.second + 1};
        break;
    }
    moveWallComp.m_nextPhasePos = getAbsolutePosition(nextCase);
    if(Level::getElementCase(nextCase)->m_typeStd == LevelCaseType_e::EMPTY_LC &&
        !(Level::getElementCase(nextCase)->m_type == LevelCaseType_e::WALL_LC &&
          Level::getElementCase(nextCase)->m_typeStd == LevelCaseType_e::EMPTY_LC &&
          (Level::getElementCase(nextCase)->m_memStaticMoveableWall &&
           !Level::getElementCase(nextCase)->m_memStaticMoveableWall->empty())))
    {
        Level::memMoveWallEntity(nextCase, wallEntity);
        Level::setElementTypeCase(nextCase, LevelCaseType_e::WALL_MOVE_LC);
    }
}

//===================================================================
void stopMoveWallLevelLimitCase(MapCoordComponent &mapComp, MoveableWallConfComponent &moveWallComp, uint32_t entityNum)
{
    Level::resetMoveWallElementCase(mapComp.m_coord, entityNum);
    Level::setElementTypeCase(mapComp.m_coord, LevelCaseType_e::WALL_LC);
    Level::setStandardElementTypeCase(mapComp.m_coord, LevelCaseType_e::WALL_LC);
    Level::setElementEntityCase(mapComp.m_coord, entityNum);
    moveWallComp.m_cycleInMovement = false;
    moveWallComp.m_actionned = true;
    moveWallComp.m_inMovement = false;
    moveWallComp.m_triggerBehaviour = TriggerBehaviourType_e::ONCE;
}

//===================================================================
void PlatformSystem::switchToNextPhaseMoveWall(uint32_t wallEntity, MapCoordComponent &mapComp, MoveableWallConfComponent &moveWallComp,
                                               const PairUI_t &previousPos, uint32_t entityNum)
{
    bool autoMode = (moveWallComp.m_triggerBehaviour == TriggerBehaviourType_e::AUTO);
    Level::resetMoveWallElementCase(previousPos, entityNum);
    moveWallComp.m_initPos = true;
    if(++moveWallComp.m_currentMove == moveWallComp.m_directionMove[moveWallComp.m_currentPhase].second)
    {
        moveWallComp.m_currentMove = 0;
        //IF CYCLE END
        if(++moveWallComp.m_currentPhase == moveWallComp.m_directionMove.size())
        {
            std::optional<ElementRaycast> element = Level::getElementCase(mapComp.m_coord);
            if(!autoMode && element && element->m_typeStd != LevelCaseType_e::WALL_LC)
            {
                Level::memStaticMoveWallEntity(mapComp.m_coord, wallEntity);
            }
            //put element case to static wall
            if(!element || element->m_typeStd != LevelCaseType_e::WALL_LC)
            {
                Level::setElementEntityCase(mapComp.m_coord, entityNum);
            }
            Level::resetMoveWallElementCase(mapComp.m_coord, entityNum);
            Level::setElementTypeCase(mapComp.m_coord, LevelCaseType_e::WALL_LC);
            Level::setMoveableWallStopped(mapComp.m_coord, true);
            if(moveWallComp.m_triggerBehaviour == TriggerBehaviourType_e::ONCE)
            {
                GeneralCollisionComponent *genComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(wallEntity);
                assert(genComp);
                genComp->m_tagB = CollisionTag_e::WALL_CT;
            }
            else
            {
                if(moveWallComp.m_triggerBehaviour == TriggerBehaviourType_e::REVERSABLE)
                {
                    reverseDirection(moveWallComp);
                }
            }
            moveWallComp.m_manualTrigger = autoMode;
            moveWallComp.m_inMovement = false;
            moveWallComp.m_actionned = false;
            moveWallComp.m_cycleInMovement = (moveWallComp.m_triggerBehaviour == TriggerBehaviourType_e::AUTO);
        }
    }
    assert(m_refMainEngine);
    m_refMainEngine->addEntityToZone(wallEntity, *getLevelCoord(mapComp.m_absoluteMapPositionPX));
}

//===================================================================
void reverseDirection(MoveableWallConfComponent &moveWallComp)
{
    assert(!moveWallComp.m_directionMove.empty());
    if(moveWallComp.m_directionMove.size() == 1)
    {
        moveWallComp.m_directionMove[0].first =
            getReverseDirection(moveWallComp.m_directionMove[0].first);
    }
    else
    {
        uint32_t size = moveWallComp.m_directionMove.size(), mirrorCase;
        for(uint32_t i = 0; i < size; ++i)
        {
            mirrorCase = size - (i + 1);
            if(i < mirrorCase)
            {
                moveWallComp.m_directionMove[i].first =
                    getReverseDirection(moveWallComp.m_directionMove[i].first);
                moveWallComp.m_directionMove[mirrorCase].first =
                    getReverseDirection(moveWallComp.m_directionMove[mirrorCase].first);
                std::swap(moveWallComp.m_directionMove[i], moveWallComp.m_directionMove[mirrorCase]);
            }
            else if(i == mirrorCase)
            {
                moveWallComp.m_directionMove[i].first =
                    getReverseDirection(moveWallComp.m_directionMove[i].first);
            }
            else
            {
                break;
            }
        }
    }
}

//===================================================================
Direction_e getReverseDirection(Direction_e dir)
{
    switch(dir)
    {
    case Direction_e::EAST:
        return Direction_e::WEST;
    case Direction_e::WEST:
        return Direction_e::EAST;
    case Direction_e::NORTH:
        return Direction_e::SOUTH;
    case Direction_e::SOUTH:
        return Direction_e::NORTH;
    default:
        assert(false);//avoid warning
    }
}

//===================================================================
void PlatformSystem::clearSystem()
{
    m_usedEntities.clear();
    m_cacheUsedComponent.clear();
    m_usedEntities.clear();
    m_vectTrigger.clear();
}

//===================================================================
void PlatformSystem::memRefMainEngine(MainEngine *mainEngine)
{
    m_refMainEngine = mainEngine;
}
