#include "StaticDisplaySystem.hpp"
#include "MainEngine.hpp"
#include <ECS/Components/MemPositionsVertexComponents.hpp>
#include <ECS/Components/MemSpriteDataComponent.hpp>
#include <ECS/Components/SpriteTextureComponent.hpp>
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/PlayerConfComponent.hpp>
#include <ECS/Components/TimerComponent.hpp>
#include <ECS/Components/WriteComponent.hpp>
#include <ECS/Components/WeaponComponent.hpp>
#include <ECS/Components/AudioComponent.hpp>
#include <ECS/Systems/ColorDisplaySystem.hpp>
#include <cassert>


//===================================================================
StaticDisplaySystem::StaticDisplaySystem()
{
    bAddComponentToSystem(Components_e::VISION_COMPONENT);
}

//===================================================================
void StaticDisplaySystem::fillCursorMenuVertex(PlayerConfComponent *playerComp)
{
    uint32_t index = static_cast<uint32_t>(VertexID_e::MENU_CURSOR);
    SpriteTextureComponent *spriteComp = stairwayToComponentManager().
            searchComponentByType<SpriteTextureComponent>(playerComp->m_menuCursorEntity,
                                                          Components_e::SPRITE_TEXTURE_COMPONENT);
    PositionVertexComponent *posComp = stairwayToComponentManager().
            searchComponentByType<PositionVertexComponent>(playerComp->m_menuCursorEntity,
                                                          Components_e::POSITION_VERTEX_COMPONENT);
    assert(spriteComp);
    assert(posComp);
    m_vertices[index].clear();
    m_vertices[index].loadVertexStandartTextureComponent(*posComp, *spriteComp);
    m_cursorInit = true;
}

//===================================================================
void StaticDisplaySystem::updateMenuCursorPosition(PlayerConfComponent *playerComp)
{
    PositionVertexComponent *posComp = stairwayToComponentManager().
                searchComponentByType<PositionVertexComponent>(playerComp->m_menuCursorEntity,
                                                               Components_e::POSITION_VERTEX_COMPONENT);
    assert(posComp);
    WriteComponent *writeMenuComp = stairwayToComponentManager().
                searchComponentByType<WriteComponent>(playerComp->m_menuEntriesEntity,
                                                      Components_e::WRITE_COMPONENT);
    assert(writeMenuComp);
    float upPos = writeMenuComp->m_upLeftPositionGL.second -
            static_cast<float>(playerComp->m_currentCursorPos) * writeMenuComp->m_fontSize,
            downPos = upPos - writeMenuComp->m_fontSize * 2.0f, rightPos =
            writeMenuComp->m_upLeftPositionGL.first - 0.05f, leftPos =
            rightPos - CURSOR_GL_SIZE.first;
    posComp->m_vertex[0] = {leftPos, upPos};
    posComp->m_vertex[1] = {rightPos, upPos};
    posComp->m_vertex[2] = {rightPos, downPos};
    posComp->m_vertex[3] = {leftPos, downPos};
    m_currentCursorPos = playerComp->m_currentCursorPos;
    m_cursorInit = false;
}

//===================================================================
void StaticDisplaySystem::execSystem()
{
    double time;
    System::execSystem();
    m_shader->use();
    for(uint32_t i = 0; i < mVectNumEntity.size(); ++i)
    {
        PlayerConfComponent *playerComp = stairwayToComponentManager().
                    searchComponentByType<PlayerConfComponent>(mVectNumEntity[i],
                                                               Components_e::PLAYER_CONF_COMPONENT);
        assert(playerComp);
        WeaponComponent *weaponComp = stairwayToComponentManager().
                searchComponentByType<WeaponComponent>(playerComp->m_weaponEntity,
                                                       Components_e::WEAPON_COMPONENT);
        assert(weaponComp);
        //DRAW WEAPON
        SpriteTextureComponent *spriteComp = stairwayToComponentManager().
                searchComponentByType<SpriteTextureComponent>(playerComp->m_weaponEntity,
                                                              Components_e::SPRITE_TEXTURE_COMPONENT);
        assert(spriteComp);
        confWeaponsVertexFromComponent(playerComp, spriteComp);
        drawVertex(spriteComp->m_spriteData->m_textureNum, VertexID_e::WEAPON);
        std::string strAmmoDisplay = STR_PLAYER_AMMO +
                std::to_string(weaponComp->m_weaponsData[weaponComp->m_currentWeapon].m_ammunationsCount);
        drawWriteVertex(playerComp->m_ammoWriteEntity, VertexID_e::AMMO_WRITE, strAmmoDisplay);
        drawWriteVertex(playerComp->m_lifeWriteEntity, VertexID_e::LIFE_WRITE, STR_PLAYER_LIFE +
                         std::to_string(playerComp->m_life));
        if(playerComp->m_infoWriteData.first)
        {
            TimerComponent *timerComp = stairwayToComponentManager().
                    searchComponentByType<TimerComponent>(mVectNumEntity[i], Components_e::TIMER_COMPONENT);
            assert(timerComp);
            std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - timerComp->m_clockA;
            drawWriteVertex(playerComp->m_numInfoWriteEntity, VertexID_e::INFO, playerComp->m_infoWriteData.second);
            time = (!timerComp->m_time) ? 1.5 : *timerComp->m_time;
            if(elapsed_seconds.count() > time)
            {
                playerComp->m_infoWriteData.first = false;
                timerComp->m_time = {};
            }
        }
    }
}

//===================================================================
void StaticDisplaySystem::displayMenu()
{
    System::execSystem();
    m_shader->use();
    for(uint32_t i = 0; i < mVectNumEntity.size(); ++i)
    {
        PlayerConfComponent *playerComp = stairwayToComponentManager().
                    searchComponentByType<PlayerConfComponent>(mVectNumEntity[i], Components_e::PLAYER_CONF_COMPONENT);
        assert(playerComp);
        drawWriteVertex(playerComp->m_menuEntriesEntity, VertexID_e::MENU_WRITE);
        drawWriteVertex(playerComp->m_titleMenuEntity, VertexID_e::MENU_WRITE);
        SpriteTextureComponent *spriteComp = stairwayToComponentManager().
                searchComponentByType<SpriteTextureComponent>(playerComp->m_menuCursorEntity, Components_e::SPRITE_TEXTURE_COMPONENT);
        assert(spriteComp);
        if(playerComp->m_menuMode != MenuMode_e::NEW_KEY)
        {
            updateMenuCursorPosition(playerComp);
            if(!m_cursorInit)
            {
                fillCursorMenuVertex(playerComp);
            }
            drawVertex(spriteComp->m_spriteData->m_textureNum, VertexID_e::MENU_CURSOR);
        }
        if(playerComp->m_menuMode == MenuMode_e::SOUND)
        {
            mptrSystemManager->searchSystemByType<ColorDisplaySystem>(
                        static_cast<uint32_t>(Systems_e::COLOR_DISPLAY_SYSTEM))->drawSoundMenuBars();
            //reset shader
            m_shader->use();
        }
        else if(playerComp->m_menuMode == MenuMode_e::DISPLAY)
        {
            drawWriteVertex(m_resolutionDisplayMenuEntity, VertexID_e::RESOLUTION_DISPLAY_MENU);
            drawWriteVertex(m_fullscreenMenuEntity, VertexID_e::FULLSCREEN);
        }
        else if(playerComp->m_menuMode == MenuMode_e::INPUT)
        {
            drawWriteVertex(playerComp->m_menuInfoWriteEntity, VertexID_e::INPUT);
            mptrSystemManager->searchSystemByType<ColorDisplaySystem>(
                        static_cast<uint32_t>(Systems_e::COLOR_DISPLAY_SYSTEM))->drawInputMenuBar();
            //reset shader
            m_shader->use();
            if(playerComp->m_keyboardInputMenuMode)
            {
                for(uint32_t j = 0; j < m_inputMenuKeyboardWriteKeysEntities.size(); ++j)
                {
                    drawWriteVertex(m_inputMenuKeyboardWriteKeysEntities[j], VertexID_e::INPUT);
                }
            }
            else
            {
                for(uint32_t j = 0; j < m_inputMenuGamepadWriteKeysEntities.size(); ++j)
                {
                    drawWriteVertex(m_inputMenuGamepadWriteKeysEntities[j], VertexID_e::INPUT);
                }
            }
        }
        else if(playerComp->m_menuMode == MenuMode_e::CONFIRM_QUIT_INPUT_FORM ||
                playerComp->m_menuMode == MenuMode_e::CONFIRM_LOADING_GAME_FORM ||
                playerComp->m_menuMode == MenuMode_e::CONFIRM_RESTART_LEVEL ||
                playerComp->m_menuMode == MenuMode_e::CONFIRM_RESTART_FROM_LAST_CHECKPOINT)
        {
            drawWriteVertex(playerComp->m_menuInfoWriteEntity, VertexID_e::INPUT);
        }
    }
}

//===================================================================
void StaticDisplaySystem::updateStringWriteEntitiesInputMenu(bool keyboardInputMenuMode, bool defaultInput)
{
    WriteComponent *writeConf;
    //MOUSE KEYBOARD
    if(keyboardInputMenuMode)
    {
        const std::map<ControlKey_e, MouseKeyboardInputState> &map = defaultInput ? MAP_KEYBOARD_DEFAULT_KEY :
                    mptrSystemManager->searchSystemByType<InputSystem>(
                        static_cast<uint32_t>(Systems_e::INPUT_SYSTEM))->getMapTmpKeyboardAssociatedKey();
        for(uint32_t i = 0; i < m_inputMenuKeyboardWriteKeysEntities.size(); ++i)
        {
            writeConf = stairwayToComponentManager().
                    searchComponentByType<WriteComponent>(m_inputMenuKeyboardWriteKeysEntities[i], Components_e::WRITE_COMPONENT);
            assert(writeConf);
            writeConf->m_str = getMouseKeyboardStringKeyAssociated(map.at(static_cast<ControlKey_e>(i)));
            m_mainEngine->updateWriteComp(writeConf);
        }
    }
    //GAMEPAD
    else
    {
        const std::map<ControlKey_e, GamepadInputState> &map = defaultInput ?
                    MAP_GAMEPAD_DEFAULT_KEY :
                    mptrSystemManager->searchSystemByType<InputSystem>(
                        static_cast<uint32_t>(Systems_e::INPUT_SYSTEM))->getMapTmpGamepadAssociatedKey();
        for(uint32_t i = 0; i < m_inputMenuGamepadWriteKeysEntities.size(); ++i)
        {
            writeConf = stairwayToComponentManager().
                    searchComponentByType<WriteComponent>(m_inputMenuGamepadWriteKeysEntities[i], Components_e::WRITE_COMPONENT);
            assert(writeConf);
            if(map.at(static_cast<ControlKey_e>(i)).m_standardButton)
            {
                writeConf->m_str = getGamepadStringKeyButtonAssociated(map.at(static_cast<ControlKey_e>(i)).m_keyID);
            }
            else
            {
                writeConf->m_str = getGamepadStringKeyAxisAssociated(map.at(static_cast<ControlKey_e>(i)).m_keyID,
                                                                     (*map.at(static_cast<ControlKey_e>(i)).m_axisPos));
            }
            m_mainEngine->updateWriteComp(writeConf);
        }
    }
}

//===================================================================
void StaticDisplaySystem::updateDisplayMenuResolution(const std::string &str)
{
    WriteComponent *writeComp = stairwayToComponentManager().
                searchComponentByType<WriteComponent>(m_resolutionDisplayMenuEntity, Components_e::WRITE_COMPONENT);
    assert(writeComp);
    writeComp->m_str = str;
    PositionVertexComponent *posComp = stairwayToComponentManager().
                searchComponentByType<PositionVertexComponent>(m_resolutionDisplayMenuEntity, Components_e::POSITION_VERTEX_COMPONENT);
    assert(posComp);
    writeComp->m_fontSpriteData = m_fontDataPtr->getWriteData(str, writeComp->m_numTexture);
    confWriteVertex(writeComp, posComp, VertexID_e::RESOLUTION_DISPLAY_MENU);
}

//===================================================================
void StaticDisplaySystem::updateMenuEntryFullscreen(bool displayMenufullscreenMode)
{
    WriteComponent *writeComp = stairwayToComponentManager().
                searchComponentByType<WriteComponent>(m_fullscreenMenuEntity, Components_e::WRITE_COMPONENT);
    assert(writeComp);
    writeComp->m_str = displayMenufullscreenMode ? "X" : "";
    PositionVertexComponent *posComp = stairwayToComponentManager().
                searchComponentByType<PositionVertexComponent>(m_fullscreenMenuEntity, Components_e::POSITION_VERTEX_COMPONENT);
    assert(posComp);
    writeComp->m_fontSpriteData = m_fontDataPtr->getWriteData(writeComp->m_str, writeComp->m_numTexture);
    confWriteVertex(writeComp, posComp, VertexID_e::FULLSCREEN);
}

//===================================================================
std::string StaticDisplaySystem::getMouseKeyboardStringKeyAssociated(const MouseKeyboardInputState &state)const
{
    std::map<uint32_t, std::string>::const_iterator it;
    if(state.m_keyboard)
    {
        it = INPUT_KEYBOARD_KEY_STRING.find(state.m_key);
        if(it == INPUT_KEYBOARD_KEY_STRING.end())
        {
            return "";
        }
        return it->second;
    }
    else
    {
        it = INPUT_MOUSE_KEY_STRING.find(state.m_key);
        if(it == INPUT_MOUSE_KEY_STRING.end())
        {
            return "";
        }
        return it->second;
    }
}

//===================================================================
std::string StaticDisplaySystem::getGamepadStringKeyAxisAssociated(uint32_t key, bool axisSense)const
{
    std::map<uint32_t, std::string>::const_iterator it = INPUT_GAMEPAD_AXIS_STRING.find(key);
    if(it == INPUT_GAMEPAD_AXIS_STRING.end())
    {
        return "";
    }
    if(axisSense)
    {
        return it->second + " +";
    }
    else
    {
        return it->second + " -";
    }
}

//===================================================================
std::string StaticDisplaySystem::getGamepadStringKeyButtonAssociated(uint32_t key) const
{
    std::map<uint32_t, std::string>::const_iterator it = INPUT_GAMEPAD_SIMPLE_BUTTONS_STRING.find(key);
    if(it == INPUT_GAMEPAD_SIMPLE_BUTTONS_STRING.end())
    {
        return "";
    }
    return it->second;
}

//===================================================================
void StaticDisplaySystem::updateNewInputKeyGamepad(ControlKey_e currentSelectedKey, uint32_t glKey,
                                                   InputType_e inputType, bool axisSense)
{
    uint32_t entityWrite = m_inputMenuGamepadWriteKeysEntities[static_cast<uint32_t>(currentSelectedKey)];
    WriteComponent *writeComp = stairwayToComponentManager().searchComponentByType<WriteComponent>(
                entityWrite, Components_e::WRITE_COMPONENT);
    assert(writeComp);
    if(inputType == InputType_e::GAMEPAD_BUTTONS)
    {
        writeComp->m_str = getGamepadStringKeyButtonAssociated(glKey);
    }
    else if(inputType == InputType_e::GAMEPAD_AXIS)
    {
        writeComp->m_str = getGamepadStringKeyAxisAssociated(glKey, axisSense);
    }
    m_mainEngine->updateWriteComp(writeComp);
}

//===================================================================
void StaticDisplaySystem::updateNewInputKeyKeyboard(ControlKey_e currentSelectedKey, const MouseKeyboardInputState &state)
{
    uint32_t entityWrite = m_inputMenuKeyboardWriteKeysEntities[static_cast<uint32_t>(currentSelectedKey)];
    WriteComponent *writeComp = stairwayToComponentManager().searchComponentByType<WriteComponent>(
                entityWrite, Components_e::WRITE_COMPONENT);
    assert(writeComp);
    writeComp->m_str = getMouseKeyboardStringKeyAssociated(state);
    m_mainEngine->updateWriteComp(writeComp);
}

//===================================================================
void StaticDisplaySystem::confWriteVertex(WriteComponent *writeComp,
                                          PositionVertexComponent *posComp,
                                          VertexID_e type)
{
    uint32_t index = static_cast<uint32_t>(type);
    m_vertices[index].clear();
    drawLineWriteVertex(posComp, writeComp);
    m_vertices[index].loadVertexWriteTextureComponent(*posComp, *writeComp);
}

//===================================================================
void StaticDisplaySystem::drawVertex(uint32_t numTexture, VertexID_e type)
{
    if(static_cast<size_t>(numTexture) > m_ptrVectTexture->size())
    {
        return;
    }
    m_ptrVectTexture->operator[](numTexture).bind();
    uint32_t index = static_cast<uint32_t>(type);
    m_vertices[index].confVertexBuffer();
    m_vertices[index].drawElement();
}

//===================================================================
void StaticDisplaySystem::drawWriteVertex(uint32_t numEntity, VertexID_e type, const std::string &value)
{
    WriteComponent *writeComp = stairwayToComponentManager().
                searchComponentByType<WriteComponent>(numEntity, Components_e::WRITE_COMPONENT);
    PositionVertexComponent *posComp = stairwayToComponentManager().
                searchComponentByType<PositionVertexComponent>(numEntity, Components_e::POSITION_VERTEX_COMPONENT);
    assert(writeComp);
    assert(posComp);
    if(!value.empty())
    {
        if(value != writeComp->m_str)
        {
            writeComp->m_fontSpriteData = m_fontDataPtr->getWriteData(value, writeComp->m_numTexture);
        }
        writeComp->m_str = value;
    }
    if(type == VertexID_e::INFO)
    {
        writeComp->m_upLeftPositionGL.first = getLeftTextPosition(writeComp->m_str);
    }
    confWriteVertex(writeComp, posComp, type);
    drawVertex(writeComp->m_numTexture, type);
}

//===================================================================
void StaticDisplaySystem::confWeaponsVertexFromComponent(PlayerConfComponent *playerComp,
                                                         SpriteTextureComponent *weaponSpriteComp)
{
    PositionVertexComponent *posComp = stairwayToComponentManager().
            searchComponentByType<PositionVertexComponent>(playerComp->m_weaponEntity,
                                                           Components_e::POSITION_VERTEX_COMPONENT);
    TimerComponent *timerComp = stairwayToComponentManager().
            searchComponentByType<TimerComponent>(playerComp->m_weaponEntity,
                                                  Components_e::TIMER_COMPONENT);
    MemPositionsVertexComponents *memPosComp = stairwayToComponentManager().
            searchComponentByType<MemPositionsVertexComponents>(playerComp->m_weaponEntity,
                                                                Components_e::MEM_POSITIONS_VERTEX_COMPONENT);
    WeaponComponent *weaponComp = stairwayToComponentManager().
            searchComponentByType<WeaponComponent>(playerComp->m_weaponEntity,
                                                                Components_e::WEAPON_COMPONENT);
    assert(weaponComp);
    assert(memPosComp);
    assert(timerComp);
    assert(posComp);
    assert(!posComp->m_vertex.empty());
    if(weaponComp->m_weaponChange)
    {
        setDisplayWeaponChange(posComp, playerComp, memPosComp);
    }
    else
    {
        if(playerComp->m_playerShoot)
        {
            timerComp->m_cycleCountA = 0;
            weaponComp->m_numWeaponSprite = weaponComp->getStdCurrentWeaponSprite() + 1;
            setWeaponSprite(playerComp->m_weaponEntity, weaponComp->m_numWeaponSprite);
            playerComp->m_playerShoot = false;
            weaponComp->m_timerShootActive = true;
            weaponComp->m_shootFirstPhase = true;
        }
        else if(weaponComp->m_timerShootActive)
        {
            treatWeaponShootAnimation(playerComp, timerComp);
        }
        else if(!weaponComp->m_timerShootActive)
        {
            setWeaponMovement(playerComp, posComp, memPosComp);
        }
    }
    uint32_t index = static_cast<uint32_t>(VertexID_e::WEAPON);
    m_vertices[index].clear();
    m_vertices[index].loadVertexStandartTextureComponent(*posComp, *weaponSpriteComp);
}

//===================================================================
void StaticDisplaySystem::treatWeaponShootAnimation(PlayerConfComponent *playerComp, TimerComponent *timerComp)
{
    WeaponComponent *weaponComp = stairwayToComponentManager().
            searchComponentByType<WeaponComponent>(playerComp->m_weaponEntity, Components_e::WEAPON_COMPONENT);
    assert(weaponComp);
    const WeaponData &currentWeapon = weaponComp->m_weaponsData[weaponComp->m_currentWeapon];
    uint32_t spriteFirstAnim = currentWeapon.m_memPosSprite.first, spriteNumLastAnim = currentWeapon.m_lastAnimNum;
    if(++timerComp->m_cycleCountA > currentWeapon.m_intervalLatency)
    {
        if(currentWeapon.m_animMode == AnimationMode_e::CONSTANT)
        {
            if(weaponComp->m_numWeaponSprite == spriteNumLastAnim)
            {
                weaponComp->m_numWeaponSprite = spriteFirstAnim;
                weaponComp->m_timerShootActive = false;
            }
            else
            {
                ++weaponComp->m_numWeaponSprite;
            }
        }
        else
        {
            if(weaponComp->m_shootFirstPhase)
            {
                if(weaponComp->m_numWeaponSprite == currentWeapon.m_memPosSprite.second)
                {
                    if(currentWeapon.m_animMode == AnimationMode_e::STANDART)
                    {
                        weaponComp->m_numWeaponSprite = spriteFirstAnim;
                        weaponComp->m_timerShootActive = false;
                    }
                    else if(currentWeapon.m_animMode == AnimationMode_e::RETURN)
                    {
                        --weaponComp->m_numWeaponSprite;
                    }
                    weaponComp->m_shootFirstPhase = false;
                }
                else
                {
                    if(weaponComp->m_numWeaponSprite == currentWeapon.m_memPosSprite.first + 2)
                    {
                        AudioComponent *audioComp = stairwayToComponentManager().
                                searchComponentByType<AudioComponent>(playerComp->m_weaponEntity, Components_e::AUDIO_COMPONENT);
                        assert(audioComp);
                        audioComp->m_soundElements[weaponComp->m_reloadSoundAssociated[
                                weaponComp->m_currentWeapon]]->m_toPlay = true;
                    }
                    ++weaponComp->m_numWeaponSprite;
                }
            }
            else
            {
                if(currentWeapon.m_animMode == AnimationMode_e::RETURN && weaponComp->m_numWeaponSprite == spriteNumLastAnim)
                {
                    weaponComp->m_timerShootActive = false;
                    weaponComp->m_numWeaponSprite = spriteFirstAnim;
                }
                else if(currentWeapon.m_animMode == AnimationMode_e::STANDART && weaponComp->m_numWeaponSprite == spriteFirstAnim)
                {
                    weaponComp->m_timerShootActive = false;
                    return;
                }
                else
                {
                    --weaponComp->m_numWeaponSprite;
                }
            }
        }
        setWeaponSprite(playerComp->m_weaponEntity, weaponComp->m_numWeaponSprite);
        timerComp->m_cycleCountA = 0;
    }
}


//===================================================================
void StaticDisplaySystem::setWeaponMovement(PlayerConfComponent *playerComp,
                                            PositionVertexComponent *posComp,
                                            MemPositionsVertexComponents *memPosComp)
{
    WeaponComponent *weaponComp = stairwayToComponentManager().
            searchComponentByType<WeaponComponent>(playerComp->m_weaponEntity,
                                                   Components_e::WEAPON_COMPONENT);
    assert(weaponComp);
    uint32_t index = weaponComp->getStdCurrentWeaponSprite();
    if(playerComp->m_inMovement)
    {
        modVertexPos(posComp, weaponComp->m_currentWeaponMove);
        //check X var
        if(weaponComp->m_currentWeaponMove.first < EPSILON_FLOAT &&
                posComp->m_vertex[0].first <= m_forkWeaponMovementX.first)
        {
            weaponComp->m_currentWeaponMove.first *= -1.0f;
            //fix go down issue
            modVertexPos(posComp, {EPSILON_FLOAT,
                                   std::abs(memPosComp->m_vectSpriteData[index][0].second -
                                   posComp->m_vertex[0].second)});
        }
        else if(weaponComp->m_currentWeaponMove.first > EPSILON_FLOAT &&
                posComp->m_vertex[0].first >= m_forkWeaponMovementX.second)
        {
            weaponComp->m_currentWeaponMove.first *= -1.0f;
            //fix go down issue
            modVertexPos(posComp, {EPSILON_FLOAT,
                                   std::abs(memPosComp->m_vectSpriteData[index][0].second -
                                   posComp->m_vertex[0].second)});
        }
        //check Y var
        if(posComp->m_vertex[2].second >= (-1.0f -
                                           std::abs(weaponComp->m_currentWeaponMove.second)))
        {
            modVertexPos(posComp, {EPSILON_FLOAT,
                                   std::abs(weaponComp->m_currentWeaponMove.second) * -1.0f});
        }
        if((posComp->m_vertex[0].second < memPosComp->m_vectSpriteData[index][0].second -
            std::abs(weaponComp->m_currentWeaponMove.second)) &&
                ((posComp->m_vertex[0].first < m_middleWeaponMovementX &&
            weaponComp->m_currentWeaponMove.first < EPSILON_FLOAT) ||
                (posComp->m_vertex[0].first > m_middleWeaponMovementX &&
                 weaponComp->m_currentWeaponMove.first > EPSILON_FLOAT)))
        {
            weaponComp->m_currentWeaponMove.second = std::abs(weaponComp->m_currentWeaponMove.second);
        }
        else
        {
            weaponComp->m_currentWeaponMove.second =
                    std::abs(weaponComp->m_currentWeaponMove.second) * (-1.0f);
        }
        weaponComp->m_spritePositionCorrected = false;
    }
    //put weapon to standart position
    else if(!weaponComp->m_spritePositionCorrected)
    {
        bool standardPosReachedY = false, standardPosReachedX = false;
        float modX;
        if(posComp->m_vertex[0].first < memPosComp->m_vectSpriteData[index][0].first)
        {
            modX = std::abs(weaponComp->m_currentWeaponMove.first) * 1.5f;
        }
        else if(posComp->m_vertex[0].first > memPosComp->m_vectSpriteData[index][0].first)
        {
            modX = -std::abs(weaponComp->m_currentWeaponMove.first) * 1.5f;
        }
        float modY;
        if(posComp->m_vertex[2].second < DOWN_WEAPON_POS_Y)
        {
            modY = std::abs(weaponComp->m_currentWeaponMove.second);
        }
        else
        {
            modY = EPSILON_FLOAT;
            standardPosReachedY = true;
        }
        if(std::abs(std::abs(posComp->m_vertex[0].first) -
                    std::abs(memPosComp->m_vectSpriteData[index][0].first)) < 0.01f)
        {
            modX = EPSILON_FLOAT;
            standardPosReachedX = true;
        }
        if(!standardPosReachedX || !standardPosReachedY)
        {
            modVertexPos(posComp, {modX, modY});
        }
        else
        {
            for(uint32_t i = 0; i < 4; ++i)
            {
                posComp->m_vertex[i] =
                        memPosComp->m_vectSpriteData[weaponComp->getStdCurrentWeaponSprite()][i];
            }
            weaponComp->m_spritePositionCorrected = true;
        }
    }
}

//===================================================================
void StaticDisplaySystem::setDisplayWeaponChange(PositionVertexComponent *posComp,
                                                 PlayerConfComponent *playerComp,
                                                 MemPositionsVertexComponents *memPosComp)
{
    WeaponComponent *weaponComp = stairwayToComponentManager().
            searchComponentByType<WeaponComponent>(playerComp->m_weaponEntity,
                                                   Components_e::WEAPON_COMPONENT);
    assert(weaponComp);
    //move down
    if(weaponComp->m_previousWeapon != weaponComp->m_currentWeapon)
    {
        modVertexPos(posComp, {EPSILON_FLOAT, -m_speedMoveWeaponChange});
        if(posComp->m_vertex[0].second <= -1.0f)
        {
            weaponComp->m_previousWeapon = weaponComp->m_currentWeapon;
            setWeaponSprite(playerComp->m_weaponEntity, weaponComp->getStdCurrentWeaponSprite());
            float diffVert = posComp->m_vertex[3].second - posComp->m_vertex[0].second;
            modVertexPos(posComp, {EPSILON_FLOAT, diffVert});
        }
    }
    //move up
    else
    {
        modVertexPos(posComp, {EPSILON_FLOAT, m_speedMoveWeaponChange});
        uint32_t index = weaponComp->getStdCurrentWeaponSprite();
        if(posComp->m_vertex[0].second >= memPosComp->m_vectSpriteData[index][0].second)
        {
            posComp->m_vertex[0].second = memPosComp->m_vectSpriteData[index][0].second;
            weaponComp->m_weaponChange = false;
        }
    }
}

//===================================================================
void modVertexPos(PositionVertexComponent *posComp, const PairFloat_t &mod)
{
    for(uint32_t i = 0; i < posComp->m_vertex.size(); ++i)
    {
        posComp->m_vertex[i].first += mod.first;
        posComp->m_vertex[i].second += mod.second;
    }
}

//===================================================================
void StaticDisplaySystem::drawLineWriteVertex(PositionVertexComponent *posComp, WriteComponent *writeComp)
{
    if(writeComp->m_fontSpriteData.empty())
    {
        return;
    }
    posComp->m_vertex.clear();
    posComp->m_vertex.reserve(writeComp->m_fontSpriteData.size() * 4);
    float currentX = writeComp->m_upLeftPositionGL.first, diffX,
            currentY = writeComp->m_upLeftPositionGL.second, diffY = writeComp->m_fontSize;
    std::array<PairFloat_t, 4> *memArray = &(writeComp->m_fontSpriteData[0].get().m_texturePosVertex);
    float cohef = ((*memArray)[2].second - (*memArray)[0].second) / writeComp->m_fontSize;
    uint32_t cmptSpriteData = 0;
    for(uint32_t i = 0; i < writeComp->m_str.size(); ++i)
    {
        if(writeComp->m_str[i] == ' ')
        {
            currentX += writeComp->m_fontSize / 4.0f;
            continue;
        }
        else if(writeComp->m_str[i] == '\\')
        {
            currentX = writeComp->m_upLeftPositionGL.first;
            currentY -= diffY;
            continue;
        }
        assert(cmptSpriteData < writeComp->m_fontSpriteData.size());
        memArray = &(writeComp->m_fontSpriteData[cmptSpriteData].get().m_texturePosVertex);
        diffX = ((*memArray)[1].first - (*memArray)[0].first) / cohef;
        posComp->m_vertex.emplace_back(PairFloat_t{currentX, currentY});
        posComp->m_vertex.emplace_back(PairFloat_t{currentX + diffX, currentY});
        posComp->m_vertex.emplace_back(PairFloat_t{currentX + diffX, currentY - diffY});
        posComp->m_vertex.emplace_back(PairFloat_t{currentX, currentY - diffY});
        currentX += diffX;
        ++cmptSpriteData;
    }
}

//===================================================================
void StaticDisplaySystem::setShader(Shader &shader)
{
    m_shader = &shader;
}

//===================================================================
void StaticDisplaySystem::setWeaponSprite(uint32_t weaponEntity, uint32_t weaponNumSprite)
{
    PositionVertexComponent *pos = stairwayToComponentManager().
            searchComponentByType<PositionVertexComponent>(weaponEntity,
                                                           Components_e::POSITION_VERTEX_COMPONENT);
    SpriteTextureComponent *spriteText = stairwayToComponentManager().
            searchComponentByType<SpriteTextureComponent>(weaponEntity,
                                                          Components_e::SPRITE_TEXTURE_COMPONENT);
    MemSpriteDataComponent *memSprite = stairwayToComponentManager().
            searchComponentByType<MemSpriteDataComponent>(weaponEntity,
                                                          Components_e::MEM_SPRITE_DATA_COMPONENT);
    MemPositionsVertexComponents *memPosVertex = stairwayToComponentManager().
            searchComponentByType<MemPositionsVertexComponents>(weaponEntity,
                                                                Components_e::MEM_POSITIONS_VERTEX_COMPONENT);
    assert(pos);
    assert(spriteText);
    assert(memSprite);
    assert(memPosVertex);
    if(memPosVertex->m_vectSpriteData.empty())
    {
        return;
    }
    //set sprite
    spriteText->m_spriteData = memSprite->m_vectSpriteData[weaponNumSprite];
    //set vertex pos
    pos->m_vertex.resize(memPosVertex->m_vectSpriteData[weaponNumSprite].size());
    for(uint32_t i = 0; i < pos->m_vertex.size(); ++i)
    {
        pos->m_vertex[i] = memPosVertex->m_vectSpriteData[weaponNumSprite][i];
    }
}

//===================================================================
void StaticDisplaySystem::memDisplayMenuEntities(uint32_t numMenuResolutionWrite, uint32_t numFullscreenMenuEntity)
{
    m_resolutionDisplayMenuEntity = numMenuResolutionWrite;
    m_fullscreenMenuEntity = numFullscreenMenuEntity;
}

//===================================================================
std::string treatInfoMessageEndLine(const std::string &str, uint32_t lineSize)
{
    if(str.size() < lineSize)
    {
        return str;
    }
    std::string ret = str;
    uint32_t currentPos = 0;
    size_t found = 0;
    while(currentPos < ret.size())
    {
        found = ret.find('\\', currentPos + 1);
        if(found == std::string::npos)
        {
            found = ret.size();
        }
        if(found < currentPos + lineSize)
        {
            currentPos = found;
            continue;
        }
        for(uint32_t i = currentPos + lineSize; i < found; i += lineSize)
        {
            ret.insert(ret.begin() + i, '\\');
        }
        currentPos = found;
    }
    return ret;
}

//===================================================================
float getLeftTextPosition(std::string_view str)
{
    uint32_t maxLineSize = 0, pos = 0;
    size_t find, currentPos = 0;
    do
    {
        find = str.find('\\', currentPos + 1);
        if(find == std::string::npos)
        {
            pos = str.size();
        }
        else
        {
            pos = find;
        }
        if(maxLineSize < (pos - currentPos))
        {
            maxLineSize = pos - currentPos;
        }
        currentPos = find;
    }while(find != std::string::npos);
    return maxLineSize * -0.02f;
}
