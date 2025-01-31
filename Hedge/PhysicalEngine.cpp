#include "PhysicalEngine.hpp"
#include "Level.hpp"
#include <math.h>
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/MapCoordComponent.hpp>
#include <ECS/Components/MoveableComponent.hpp>
#include <ECS/Components/WeaponComponent.hpp>
#include <ECS/Systems/StaticDisplaySystem.hpp>
#include <ECS/Systems/IASystem.hpp>
#include <ECS/Systems/GravitySystem.hpp>
#include <ECS/Systems/InputSystem.hpp>
#include <cassert>

//===================================================================
PhysicalEngine::PhysicalEngine()
{

}

//===================================================================
void PhysicalEngine::runIteration(bool gamePaused)
{
    if(!gamePaused)
    {
        m_gravSystem->execSystem();
        m_collisionSystem->execSystem();
        m_iaSystem->execSystem();
    }
    m_inputSystem->execSystem();
}

//===================================================================
void PhysicalEngine::linkSystems(InputSystem *inputSystem, CollisionSystem *collisionSystem,
                                 IASystem *iaSystem, GravitySystem *gravSystem)
{
    m_inputSystem = inputSystem;
    m_collisionSystem = collisionSystem;
    m_iaSystem = iaSystem;
    m_gravSystem = gravSystem;
}

//===================================================================
void PhysicalEngine::memPlayerEntity(uint32_t playerEntity)
{
    assert(m_iaSystem);
    m_iaSystem->memPlayerDatas(playerEntity);
    m_collisionSystem->memPlayerDatas(playerEntity);
    m_inputSystem->memPlayerEntity(playerEntity);
}

//===================================================================
void PhysicalEngine::confPlayerVisibleShoot(std::vector<uint32_t> &visibleShots,
                                            const PairFloat_t &point, float degreeAngle)
{
    m_iaSystem->confVisibleShoot(visibleShots, point, degreeAngle, CollisionTag_e::BULLET_PLAYER_CT);
}

//===================================================================
void PhysicalEngine::setModeTransitionMenu(bool transition)
{
    m_inputSystem->setModeTransitionMenu(transition);
}

//===================================================================
void PhysicalEngine::clearSystems()
{
}

//===================================================================
void PhysicalEngine::setKeyboardKey(const std::array<MouseKeyboardInputState, static_cast<uint32_t>(ControlKey_e::TOTAL)> &keyboardArray)
{
    for(uint32_t i = 0; i < keyboardArray.size(); ++i)
    {
        m_inputSystem->updateNewInputKeyKeyboard(static_cast<ControlKey_e>(i), keyboardArray[i]);
    }
}

//===================================================================
void PhysicalEngine::setGamepadKey(const std::array<GamepadInputState, static_cast<uint32_t>(ControlKey_e::TOTAL)> &gamepadArray)
{
    InputType_e inputType;
    bool axisSense = false;
    for(uint32_t i = 0; i < gamepadArray.size(); ++i)
    {
        if(gamepadArray[i].m_standardButton)
        {
            inputType = InputType_e::GAMEPAD_BUTTONS;
        }
        else
        {
            axisSense = *gamepadArray[i].m_axisPos;
            inputType = InputType_e::GAMEPAD_AXIS;
        }
        m_inputSystem->updateNewInputKeyGamepad(static_cast<ControlKey_e>(i), gamepadArray[i].m_keyID, inputType, axisSense);
    }
}

//===================================================================
void moveElementFromAngle(float distanceMove, float radiantAngle, PairFloat_t &point, bool backgroundRaycastMode)
{
    PairUI_t size = Level::getSize();
    float maxLimitX = size.first * LEVEL_TILE_SIZE_PX - distanceMove,
            maxLimitY = size.second * LEVEL_TILE_SIZE_PX - distanceMove;
    point.first += std::cos(radiantAngle) * distanceMove;
    //limit case
    if(!backgroundRaycastMode && point.first < distanceMove)
    {
        point.first = distanceMove;
    }
    else if(!backgroundRaycastMode && point.first > maxLimitX)
    {
        point.first = maxLimitX;
    }
    point.second -= std::sin(radiantAngle) * distanceMove;
    //limit case
    if(!backgroundRaycastMode && point.second < distanceMove)
    {
        point.second = distanceMove;
    }
    else if(!backgroundRaycastMode && point.second > maxLimitY)
    {
        point.second = maxLimitY;
    }
}

//===================================================================
void updatePlayerArrow(const MoveableComponent &moveComp, PositionVertexComponent &posComp)
{
    if(posComp.m_vertex.empty())
    {
        posComp.m_vertex.resize(3);
    }
    float angle = moveComp.m_degreeOrientation;
    float radiantAngle = getRadiantAngle(angle);
    posComp.m_vertex[0].first = MAP_LOCAL_CENTER_X_GL +
            cos(radiantAngle) * PLAYER_RAY_DISPLAY;
    posComp.m_vertex[0].second = MAP_LOCAL_CENTER_Y_GL +
            sin(radiantAngle) * PLAYER_RAY_DISPLAY;
    angle += 150.0f;
    radiantAngle = getRadiantAngle(angle);

    posComp.m_vertex[1].first = MAP_LOCAL_CENTER_X_GL +
            cos(radiantAngle) * PLAYER_RAY_DISPLAY;
    posComp.m_vertex[1].second = MAP_LOCAL_CENTER_Y_GL +
            sin(radiantAngle) * PLAYER_RAY_DISPLAY;
    angle += 60.0f;
    radiantAngle = getRadiantAngle(angle);

    posComp.m_vertex[2].first = MAP_LOCAL_CENTER_X_GL +
            cos(radiantAngle) * PLAYER_RAY_DISPLAY;
    posComp.m_vertex[2].second = MAP_LOCAL_CENTER_Y_GL +
            sin(radiantAngle) * PLAYER_RAY_DISPLAY;
}

//===================================================================
float getRadiantAngle(float angle)
{
    return angle * PI / 180;
}

//===================================================================
float getDegreeAngle(float angle)
{
    return angle / PI * 180;
}
