#include "DoorWallSystem.hpp"
#include "constants.hpp"
#include <Level.hpp>
#include <ECS/ECSManager.hpp>
#include <ECS/Components/DoorComponent.hpp>
#include <ECS/Components/TimerComponent.hpp>
#include <ECS/Components/MapCoordComponent.hpp>
#include <ECS/Components/RectangleCollisionComponent.hpp>
#include <ECS/Components/MoveableWallConfComponent.hpp>
#include <ECS/Components/MoveableComponent.hpp>
#include <ECS/Components/TriggerComponent.hpp>
#include <ECS/Components/GeneralCollisionComponent.hpp>
#include <ECS/Components/AudioComponent.hpp>
#include <cassert>

//===================================================================
DoorWallSystem::DoorWallSystem(const ECSManager *memECSManager)
{
    m_ECSManager = memECSManager;
    bAddComponentToSystem(Components_e::DOOR_COMPONENT);
}

//===================================================================
void DoorWallSystem::updateEntities()
{
    System::execSystem();
    std::bitset<TOTAL_COMPONENTS> bitset;
    bitset[MOVEABLE_WALL_CONF_COMPONENT] = true;
    m_vectMoveableWall = m_ECSManager->getEntitiesContainingComponents(bitset);
    bitset[MOVEABLE_WALL_CONF_COMPONENT] = false;
    bitset[TRIGGER_COMPONENT] = true;
    m_vectTrigger = m_ECSManager->getEntitiesContainingComponents(bitset);
}


//===================================================================
void DoorWallSystem::execSystem()
{
    if(mVectNumEntity.empty() && m_vectMoveableWall.empty())
    {
        updateEntities();
    }
    treatTriggers();
    treatDoors();
    treatMoveableWalls();
}

//===================================================================
void DoorWallSystem::treatDoors()
{
    for(uint32_t i = 0; i < mVectNumEntity.size(); ++i)
    {
        DoorComponent *doorComp = stairwayToComponentManager().
                searchComponentByType<DoorComponent>(mVectNumEntity[i],
                                                     Components_e::DOOR_COMPONENT);
        assert(doorComp);
        if(doorComp->m_currentState == DoorState_e::STATIC_CLOSED)
        {
            continue;
        }
        TimerComponent *timerComp = stairwayToComponentManager().
                searchComponentByType<TimerComponent>(mVectNumEntity[i],
                                                     Components_e::TIMER_COMPONENT);
        assert(timerComp);
        std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() -
                timerComp->m_clockA;
        if(doorComp->m_currentState == DoorState_e::STATIC_OPEN)
        {
            if(elapsed_seconds.count() > m_timeDoorClosed)
            {
                doorComp->m_currentState = DoorState_e::MOVE_CLOSE;
                activeDoorSound(mVectNumEntity[i]);
                timerComp->m_clockA = std::chrono::system_clock::now();
            }
            continue;
        }
        if(elapsed_seconds.count() > doorComp->m_speedMove)
        {
            treatDoorMovementSize(doorComp, mVectNumEntity[i]);
            timerComp->m_clockA = std::chrono::system_clock::now();
        }
    }
}

//===================================================================
void DoorWallSystem::activeDoorSound(uint32_t entityNum)
{
    AudioComponent *audioComp = stairwayToComponentManager().
            searchComponentByType<AudioComponent>(entityNum, Components_e::AUDIO_COMPONENT);
    assert(audioComp);
    audioComp->m_soundElements[0]->m_toPlay = true;
}

//===================================================================
void DoorWallSystem::treatMoveableWalls()
{
    bool next;
    PairUI_t memPreviousPos;
    for(uint32_t i = 0; i < m_vectMoveableWall.size(); ++i)
    {
        MoveableWallConfComponent *moveWallComp = stairwayToComponentManager().
                searchComponentByType<MoveableWallConfComponent>(m_vectMoveableWall[i],
                                                     Components_e::MOVEABLE_WALL_CONF_COMPONENT);
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
                triggerMoveableWall(m_vectMoveableWall[i]);
            }
        }
        next = false;
        MapCoordComponent *mapComp = stairwayToComponentManager().
                searchComponentByType<MapCoordComponent>(m_vectMoveableWall[i],
                                                         Components_e::MAP_COORD_COMPONENT);
        assert(mapComp);
        Direction_e currentDir = moveWallComp->m_directionMove[moveWallComp->m_currentPhase].first;
        if((currentDir == Direction_e::WEST && mapComp->m_coord.first == 0) ||
                (currentDir == Direction_e::NORTH && mapComp->m_coord.second == 0))
        {
            stopMoveWallLevelLimitCase(mapComp, moveWallComp);
            continue;
        }
        if(moveWallComp->m_initPos)
        {
            setInitPhaseMoveWall(mapComp, moveWallComp, currentDir, m_vectMoveableWall[i]);
        }
        MoveableComponent *moveComp = stairwayToComponentManager().
                searchComponentByType<MoveableComponent>(m_vectMoveableWall[i],
                                                         Components_e::MOVEABLE_COMPONENT);
        assert(moveComp);
        memPreviousPos = mapComp->m_coord;
        switch(currentDir)
        {
        case Direction_e::EAST:
            mapComp->m_absoluteMapPositionPX.first += moveComp->m_velocity;
            if(mapComp->m_absoluteMapPositionPX.first >= moveWallComp->m_nextPhasePos.first)
            {
                ++mapComp->m_coord.first;
                next = true;
            }
            break;
        case Direction_e::WEST:
            mapComp->m_absoluteMapPositionPX.first -= moveComp->m_velocity;
            if(mapComp->m_absoluteMapPositionPX.first <= moveWallComp->m_nextPhasePos.first)
            {
                --mapComp->m_coord.first;
                next = true;
            }
            break;
        case Direction_e::NORTH:
            mapComp->m_absoluteMapPositionPX.second -= moveComp->m_velocity;
            if(mapComp->m_absoluteMapPositionPX.second <= moveWallComp->m_nextPhasePos.second)
            {
                --mapComp->m_coord.second;
                next = true;
            }
            break;
        case Direction_e::SOUTH:
            mapComp->m_absoluteMapPositionPX.second += moveComp->m_velocity;
            if(mapComp->m_absoluteMapPositionPX.second >= moveWallComp->m_nextPhasePos.second)
            {
                ++mapComp->m_coord.second;
                next = true;
            }
            break;
        }
        if(next)
        {
            switchToNextPhaseMoveWall(m_vectMoveableWall[i], mapComp, moveWallComp, memPreviousPos);
        }
    }
}

//===================================================================
void DoorWallSystem::treatTriggers()
{
    for(uint32_t i = 0; i < m_vectTrigger.size(); ++i)
    {
        TriggerComponent *triggerComp = stairwayToComponentManager().
                searchComponentByType<TriggerComponent>(m_vectTrigger[i],
                                                        Components_e::TRIGGER_COMPONENT);
        assert(triggerComp);
        if(!triggerComp->m_actionned)
        {
            continue;
        }
        triggerComp->m_actionned = false;
        for(uint32_t j = 0; j < triggerComp->m_vectElementEntities.size();)
        {
            if(triggerMoveableWall(triggerComp->m_vectElementEntities[j]))
            {
                triggerComp->m_vectElementEntities.erase(triggerComp->m_vectElementEntities.begin() + j);
            }
            else
            {
                ++j;
            }
        }
    }
}

//===================================================================
bool DoorWallSystem::triggerMoveableWall(uint32_t wallEntity)
{
    MoveableWallConfComponent *moveableWallComp = stairwayToComponentManager().
            searchComponentByType<MoveableWallConfComponent>(wallEntity, Components_e::MOVEABLE_WALL_CONF_COMPONENT);
    MapCoordComponent *mapComp = stairwayToComponentManager().
            searchComponentByType<MapCoordComponent>(wallEntity, Components_e::MAP_COORD_COMPONENT);
    assert(moveableWallComp);
    assert(mapComp);
    bool once = (moveableWallComp->m_triggerBehaviour == TriggerBehaviourType_e::ONCE);
    if(moveableWallComp->m_inMovement || (once && moveableWallComp->m_actionned))
    {
       return once;
    }
    moveableWallComp->m_actionned = true;
    std::optional<ElementRaycast> element = Level::getElementCase(mapComp->m_coord);
    //init move wall case
    if(Level::getElementCase(mapComp->m_coord)->m_typeStd != LevelCaseType_e::WALL_LC)
    {
        Level::memMoveWallEntity(mapComp->m_coord, wallEntity);
    }
    moveableWallComp->m_inMovement = true;
    moveableWallComp->m_initPos = true;
    moveableWallComp->m_currentPhase = 0;
    moveableWallComp->m_currentMove = 0;
    return once;
}


//===================================================================
void setInitPhaseMoveWall(MapCoordComponent *mapComp, MoveableWallConfComponent *moveWallComp,
                          Direction_e currentDir, uint32_t wallEntity)
{
    PairUI_t nextCase;
    if(Level::getElementCase(mapComp->m_coord)->m_type != LevelCaseType_e::WALL_LC)
    {
        Level::setElementTypeCase(mapComp->m_coord, LevelCaseType_e::WALL_MOVE_LC);
    }
    moveWallComp->m_initPos = false;
    switch(currentDir)
    {
    case Direction_e::EAST:
        moveWallComp->m_nextPhasePos = {mapComp->m_absoluteMapPositionPX.first + LEVEL_TILE_SIZE_PX,
                                        mapComp->m_absoluteMapPositionPX.second};
        nextCase = {mapComp->m_coord.first + 1, mapComp->m_coord.second};
        break;
    case Direction_e::WEST:
        moveWallComp->m_nextPhasePos = {mapComp->m_absoluteMapPositionPX.first - LEVEL_TILE_SIZE_PX,
                                        mapComp->m_absoluteMapPositionPX.second};
        nextCase = {mapComp->m_coord.first - 1, mapComp->m_coord.second};
        break;
    case Direction_e::NORTH:
        moveWallComp->m_nextPhasePos = {mapComp->m_absoluteMapPositionPX.first,
                                        mapComp->m_absoluteMapPositionPX.second - LEVEL_TILE_SIZE_PX};
        nextCase = {mapComp->m_coord.first, mapComp->m_coord.second - 1};
        break;
    case Direction_e::SOUTH:
        moveWallComp->m_nextPhasePos = {mapComp->m_absoluteMapPositionPX.first,
                                        mapComp->m_absoluteMapPositionPX.second + LEVEL_TILE_SIZE_PX};
        nextCase = {mapComp->m_coord.first, mapComp->m_coord.second + 1};
        break;
    }
    if(Level::getElementCase(nextCase)->m_type != LevelCaseType_e::WALL_LC)
    {
        Level::memMoveWallEntity(nextCase, wallEntity);
        Level::setElementTypeCase(nextCase, LevelCaseType_e::WALL_MOVE_LC);
    }
}

//===================================================================
void stopMoveWallLevelLimitCase(MapCoordComponent *mapComp, MoveableWallConfComponent *moveWallComp)
{
    Level::resetMoveWallElementCase(mapComp->m_coord, moveWallComp->muiGetIdEntityAssociated());
    Level::setElementTypeCase(mapComp->m_coord, LevelCaseType_e::WALL_LC);
    Level::setStandardElementTypeCase(mapComp->m_coord, LevelCaseType_e::WALL_LC);
    Level::setElementEntityCase(mapComp->m_coord, moveWallComp->muiGetIdEntityAssociated());
    moveWallComp->m_actionned = true;
    moveWallComp->m_inMovement = false;
    moveWallComp->m_triggerBehaviour = TriggerBehaviourType_e::ONCE;
}

//===================================================================
void DoorWallSystem::switchToNextPhaseMoveWall(uint32_t wallEntity, MapCoordComponent *mapComp, MoveableWallConfComponent *moveWallComp,
                                               const PairUI_t &previousPos)
{
    Level::resetMoveWallElementCase(previousPos, moveWallComp->muiGetIdEntityAssociated());
    moveWallComp->m_initPos = true;
    if(++moveWallComp->m_currentMove == moveWallComp->m_directionMove[moveWallComp->m_currentPhase].second)
    {
        moveWallComp->m_currentMove = 0;
        //IF CYCLE END
        if(++moveWallComp->m_currentPhase == moveWallComp->m_directionMove.size())
        {
            bool autoMode = (moveWallComp->m_triggerBehaviour == TriggerBehaviourType_e::AUTO);
            std::optional<ElementRaycast> element = Level::getElementCase(mapComp->m_coord);
            //put element case to static wall
            if(element->m_type != LevelCaseType_e::WALL_LC)
            {
                Level::setElementEntityCase(mapComp->m_coord, moveWallComp->muiGetIdEntityAssociated());
                Level::resetMoveWallElementCase(mapComp->m_coord, moveWallComp->muiGetIdEntityAssociated());
                Level::setElementTypeCase(mapComp->m_coord, LevelCaseType_e::WALL_LC);
                Level::setMoveableWallStopped(mapComp->m_coord, true);
            }
            if(moveWallComp->m_triggerBehaviour == TriggerBehaviourType_e::ONCE)
            {
                GeneralCollisionComponent *genComp = stairwayToComponentManager().
                        searchComponentByType<GeneralCollisionComponent>(wallEntity,
                                                                         Components_e::GENERAL_COLLISION_COMPONENT);
                assert(genComp);
                genComp->m_tagB = CollisionTag_e::WALL_CT;
            }
            else
            {
                if(moveWallComp->m_triggerBehaviour == TriggerBehaviourType_e::REVERSABLE ||
                        autoMode)
                {
                    reverseDirection(moveWallComp);
                }
            }
            moveWallComp->m_manualTrigger = autoMode;
            moveWallComp->m_inMovement = false;
            moveWallComp->m_actionned = false;
        }
    }
}

//===================================================================
void reverseDirection(MoveableWallConfComponent *moveWallComp)
{
    assert(!moveWallComp->m_directionMove.empty());
    if(moveWallComp->m_directionMove.size() == 1)
    {
        moveWallComp->m_directionMove[0].first =
                getReverseDirection(moveWallComp->m_directionMove[0].first);
    }
    else
    {
        uint32_t size = moveWallComp->m_directionMove.size(), mirrorCase;
        for(uint32_t i = 0; i < size; ++i)
        {
            mirrorCase = size - (i + 1);
            if(i < mirrorCase)
            {
                moveWallComp->m_directionMove[i].first =
                        getReverseDirection(moveWallComp->m_directionMove[i].first);
                moveWallComp->m_directionMove[mirrorCase].first =
                        getReverseDirection(moveWallComp->m_directionMove[mirrorCase].first);
                std::swap(moveWallComp->m_directionMove[i], moveWallComp->m_directionMove[mirrorCase]);
            }
            else if(i == mirrorCase)
            {
                moveWallComp->m_directionMove[i].first =
                        getReverseDirection(moveWallComp->m_directionMove[i].first);
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
void DoorWallSystem::clearSystem()
{
    mVectNumEntity.clear();
    m_vectMoveableWall.clear();
    m_vectTrigger.clear();
}

//===================================================================
void DoorWallSystem::treatDoorMovementSize(DoorComponent *doorComp, uint32_t entityNum)
{
    RectangleCollisionComponent *rectComp = stairwayToComponentManager().
            searchComponentByType<RectangleCollisionComponent>(entityNum,
                                                               Components_e::RECTANGLE_COLLISION_COMPONENT);
    assert(rectComp);
    if(doorComp->m_currentState == DoorState_e::MOVE_CLOSE)
    {
        if(doorComp->m_vertical)
        {
            rectComp->m_size.second += 1.0f;
            if(rectComp->m_size.second >= LEVEL_TILE_SIZE_PX)
            {
                doorComp->m_currentState = DoorState_e::STATIC_CLOSED;
                rectComp->m_size.second = LEVEL_TILE_SIZE_PX;
            }
        }
        else
        {
            rectComp->m_size.first += 1.0f;
            if(rectComp->m_size.first >= LEVEL_TILE_SIZE_PX)
            {
                doorComp->m_currentState = DoorState_e::STATIC_CLOSED;
                rectComp->m_size.first = LEVEL_TILE_SIZE_PX;
            }
        }
    }
    else if(doorComp->m_currentState == DoorState_e::MOVE_OPEN)
    {
        if(doorComp->m_vertical)
        {
            rectComp->m_size.second -= 1.0f;
            if(rectComp->m_size.second <= 0.0f)
            {
                doorComp->m_currentState = DoorState_e::STATIC_OPEN;
                rectComp->m_size.second = 0.0f;
            }
        }
        else
        {
            rectComp->m_size.first -= 1.0f;
            if(rectComp->m_size.first <= 0.0f)
            {
                doorComp->m_currentState = DoorState_e::STATIC_OPEN;
                rectComp->m_size.first = 0.0f;
            }
        }
    }
}
