#include "InputSystem.hpp"
#include "constants.hpp"
#include <ECS/Components/MoveableComponent.hpp>
#include <ECS/Components/MapCoordComponent.hpp>
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/VisionComponent.hpp>
#include <ECS/Components/PlayerConfComponent.hpp>
#include <ECS/Components/WriteComponent.hpp>
#include "PhysicalEngine.hpp"
#include <MainEngine.hpp>
#include <cassert>


//===================================================================
InputSystem::InputSystem()
{
    setUsedComponents();
}

//===================================================================
void InputSystem::setUsedComponents()
{
    bAddComponentToSystem(Components_e::INPUT_COMPONENT);
}

//===================================================================
void InputSystem::treatPlayerInput()
{
    for(uint32_t i = 0; i < mVectNumEntity.size(); ++i)
    {
        if(m_mainEngine->isGamePaused())
        {
            treatMainMenu(mVectNumEntity[i]);
            continue;
        }
        if (!m_keyEspapePressed && glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(m_window, true);
            return;
        }
        else if(glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_RELEASE)
        {
            m_keyEspapePressed = false;
        }
        MapCoordComponent *mapComp = stairwayToComponentManager().
                searchComponentByType<MapCoordComponent>(mVectNumEntity[i],
                                                         Components_e::MAP_COORD_COMPONENT);
        assert(mapComp);
        MoveableComponent *moveComp = stairwayToComponentManager().
                searchComponentByType<MoveableComponent>(mVectNumEntity[i],
                                                         Components_e::MOVEABLE_COMPONENT);
        assert(moveComp);
        PositionVertexComponent *posComp = stairwayToComponentManager().
                searchComponentByType<PositionVertexComponent>(mVectNumEntity[i],
                                                         Components_e::POSITION_VERTEX_COMPONENT);
        assert(posComp);
        VisionComponent *visionComp = stairwayToComponentManager().
                searchComponentByType<VisionComponent>(mVectNumEntity[i],
                                                         Components_e::VISION_COMPONENT);
        assert(visionComp);
        PlayerConfComponent *playerComp = stairwayToComponentManager().
                searchComponentByType<PlayerConfComponent>(mVectNumEntity[i],
                                                         Components_e::PLAYER_CONF_COMPONENT);
        assert(playerComp);
        treatPlayerMove(playerComp, moveComp, mapComp);
        if(glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        {
            moveComp->m_degreeOrientation -= moveComp->m_rotationAngle;
            if(moveComp->m_degreeOrientation < 0.0f)
            {
                moveComp->m_degreeOrientation += 360.0f;
            }
            updatePlayerOrientation(*moveComp, *posComp, *visionComp);
        }
        else if(glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS)
        {
            moveComp->m_degreeOrientation += moveComp->m_rotationAngle;
            if(moveComp->m_degreeOrientation > 360.0f)
            {
                moveComp->m_degreeOrientation -= 360.0f;
            }
            updatePlayerOrientation(*moveComp, *posComp, *visionComp);
        }
        playerComp->m_playerAction = (glfwGetKey(m_window, GLFW_KEY_SPACE) ==
                                      GLFW_PRESS) ?
                    true : false;
        if(glfwGetKey(m_window, GLFW_KEY_M) == GLFW_PRESS)
        {
            m_mainEngine->setUnsetPaused();
        }
        if(!playerComp->m_weaponChange && !playerComp->m_timerShootActive)
        {
            //Change weapon
            if(glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS)
            {
                changePlayerWeapon(*playerComp, false);
            }
            else if(glfwGetKey(m_window, GLFW_KEY_R) == GLFW_PRESS)
            {
                changePlayerWeapon(*playerComp, true);
            }
            else if(!playerComp->m_timerShootActive &&
                    playerComp->m_ammunationsCount[static_cast<uint32_t>(
                        playerComp->m_currentWeapon)] > 0)
            {
                if(glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                {
                    playerComp->m_playerShoot = true;
                    m_mainEngine->playerShoot(playerComp, mapComp->m_absoluteMapPositionPX,
                                              moveComp->m_degreeOrientation);
                }
            }
        }
    }
}

//===================================================================
void InputSystem::treatPlayerMove(PlayerConfComponent *playerComp, MoveableComponent *moveComp,
                                  MapCoordComponent *mapComp)
{
    playerComp->m_inMovement = false;
    //init value
    MoveOrientation_e currentMoveDirection = MoveOrientation_e::FORWARD;
    //STRAFE
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
    {

        currentMoveDirection = MoveOrientation_e::RIGHT;
        playerComp->m_inMovement = true;
    }
    else if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        currentMoveDirection = MoveOrientation_e::LEFT;
        playerComp->m_inMovement = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS)
    {
        if(currentMoveDirection == MoveOrientation_e::RIGHT)
        {
            currentMoveDirection = MoveOrientation_e::FORWARD_RIGHT;
        }
        else if(currentMoveDirection == MoveOrientation_e::LEFT)
        {
            currentMoveDirection = MoveOrientation_e::FORWARD_LEFT;
        }
        else
        {
            currentMoveDirection = MoveOrientation_e::FORWARD;
        }
        playerComp->m_inMovement = true;
    }
    else if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        if(currentMoveDirection == MoveOrientation_e::RIGHT)
        {
            currentMoveDirection = MoveOrientation_e::BACKWARD_RIGHT;
        }
        else if(currentMoveDirection == MoveOrientation_e::LEFT)
        {
            currentMoveDirection = MoveOrientation_e::BACKWARD_LEFT;
        }
        else
        {
            currentMoveDirection = MoveOrientation_e::BACKWARD;
        }
        playerComp->m_inMovement = true;
    }
    if(playerComp->m_inMovement)
    {
        moveElement(*moveComp, *mapComp, currentMoveDirection);
    }
}

//===================================================================
void InputSystem::treatMainMenu(uint32_t playerEntity)
{
    if(glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        m_keyEspapePressed = true;
        m_mainEngine->setUnsetPaused();
        return;
    }
    if(m_keyUpPressed && glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_RELEASE)
    {
        m_keyUpPressed = false;
        return;
    }
    if(m_keyDownPressed && glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_RELEASE)
    {
        m_keyDownPressed = false;
        return;
    }
    PlayerConfComponent *playerComp = stairwayToComponentManager().
            searchComponentByType<PlayerConfComponent>(playerEntity,
                                                       Components_e::PLAYER_CONF_COMPONENT);
    assert(playerComp);
    if(!m_modeTransition && !m_keyUpPressed && glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS)
    {
        m_keyUpPressed = true;
        uint32_t index = static_cast<uint32_t>(playerComp->m_currentCursorPos);
        if(index == 0)
        {
            playerComp->m_currentCursorPos = static_cast<CurrentMenuCursorPos_e>(m_maxMenuCursorIndex);
        }
        else
        {
            playerComp->m_currentCursorPos = static_cast<CurrentMenuCursorPos_e>(index - 1);
        }
    }
    else if(!m_modeTransition && !m_keyDownPressed && glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        m_keyDownPressed = true;
        uint32_t index = static_cast<uint32_t>(playerComp->m_currentCursorPos);
        if(index == m_maxMenuCursorIndex)
        {
            playerComp->m_currentCursorPos = static_cast<CurrentMenuCursorPos_e>(0);
        }
        else
        {
            playerComp->m_currentCursorPos = static_cast<CurrentMenuCursorPos_e>(index + 1);
        }
    }
    else if(glfwGetKey(m_window, GLFW_KEY_ENTER) == GLFW_PRESS)
    {
        switch(playerComp->m_currentCursorPos)
        {
        case CurrentMenuCursorPos_e::NEW_GAME:
            break;
        case CurrentMenuCursorPos_e::QUIT_GAME:
            glfwSetWindowShouldClose(m_window, true);
            break;
        case CurrentMenuCursorPos_e::RETURN_TO_GAME:
            m_mainEngine->setUnsetPaused();
            break;
        case CurrentMenuCursorPos_e::TOTAL:
            break;
        }
    }
}

//===================================================================
void changePlayerWeapon(PlayerConfComponent &playerComp, bool next)
{
    WeaponsType_e weapon = playerComp.m_currentWeapon;
    if(!next)
    {
        do
        {
            //first weapon
            if(weapon == WeaponsType_e::GUN)
            {
                weapon = WeaponsType_e::SHOTGUN;
            }
            else
            {
                weapon = static_cast<WeaponsType_e>(
                            static_cast<uint32_t>(weapon) - 1);
            }
            if(playerComp.m_weapons[static_cast<uint32_t>(weapon)])
            {
                break;
            }
        }while(true);
    }
    else if(next)
    {
        do
        {
            //last weapon
            if(weapon == WeaponsType_e::SHOTGUN)
            {
                weapon = WeaponsType_e::GUN;
            }
            else
            {
                weapon = static_cast<WeaponsType_e>(
                            static_cast<uint8_t>(weapon) + 1);
            }
            if(playerComp.m_weapons[static_cast<uint32_t>(weapon)])
            {
                break;
            }
        }while(true);
    }
    setPlayerWeapon(playerComp, weapon);
}


//===================================================================
void setPlayerWeapon(PlayerConfComponent &playerComp, WeaponsType_e weapon)
{
    if(playerComp.m_currentWeapon != weapon)
    {
        playerComp.m_currentWeapon = weapon;
        playerComp.m_weaponChange = true;
    }
}

//===================================================================
void InputSystem::execSystem()
{
    System::execSystem();
    treatPlayerInput();
}
