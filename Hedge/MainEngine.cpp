#include "MainEngine.hpp"
#include "ECS/Systems/GravitySystem.hpp"
#include "Game.hpp"
#include "constants.hpp"
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/ColorVertexComponent.hpp>
#include <ECS/Components/MapCoordComponent.hpp>
#include <ECS/Components/SpriteTextureComponent.hpp>
#include <ECS/Components/MoveableComponent.hpp>
#include <ECS/Components/CircleCollisionComponent.hpp>
#include <ECS/Components/RectangleCollisionComponent.hpp>
#include <ECS/Components/GeneralCollisionComponent.hpp>
#include <ECS/Components/MemSpriteDataComponent.hpp>
#include <ECS/Components/CheckpointComponent.hpp>
#include <ECS/Components/PlayerConfComponent.hpp>
#include <ECS/Components/MemPositionsVertexComponents.hpp>
#include <ECS/Components/SegmentCollisionComponent.hpp>
#include <ECS/Components/WriteComponent.hpp>
#include <ECS/Components/TimerComponent.hpp>
#include <ECS/Components/EnemyConfComponent.hpp>
#include <ECS/Components/ShotConfComponent.hpp>
#include <ECS/Components/WeaponComponent.hpp>
#include <ECS/Components/LogComponent.hpp>
#include <ECS/Systems/ColorDisplaySystem.hpp>
#include <ECS/Systems/MapDisplaySystem.hpp>
#include <ECS/Systems/CollisionSystem.hpp>
#include <ECS/Systems/VisionSystem.hpp>
#include <ECS/Systems/StaticDisplaySystem.hpp>
#include <ECS/Systems/IASystem.hpp>
#include <ECS_Headers/Component.hpp>
#include <LevelManager.hpp>
#include <cassert>
#include <bitset>
#include <ECS_Headers/ECSManager.hpp>
#include <alias.hpp>

struct sMemPositionsVertexComponents : public ECS::Component
{
    sMemPositionsVertexComponents()
    {
        static_assert(std::is_convertible_v<sMemPositionsVertexComponents, ECS::Component>);
    }
    std::vector<std::array<PairFloat_t, 4>> m_vectSpriteData;
    virtual ~sMemPositionsVertexComponents() = default;
};

//===================================================================
void MainEngine::init(Game *refGame)
{
    std::srand(std::time(nullptr));
    Ecsm_t::instance().associateCompManager(std::make_unique<EcsCompManager_t>());
    instanciateSystems();
    linkSystemsToGraphicEngine();
    linkSystemsToPhysicalEngine();
    linkSystemsToSoundEngine();
    m_audioEngine.initOpenAL();
    m_refGame = refGame;
}

//===================================================================
LevelState MainEngine::displayTitleMenu(const LevelManager &levelManager)
{
    uint16_t genericBackgroundMenuSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getGenericMenuSpriteName()),
            titleBackgroundMenuSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getTitleMenuSpriteName()),
            leftBackgroundMenuSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getLeftMenuSpriteName()),
            rightLeftBackgroundMenuSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getRightLeftMenuSpriteName());
    const std::vector<SpriteData> &vectSprite = levelManager.getPictureData().getSpriteData();
    m_memBackgroundGenericMenu = &vectSprite[genericBackgroundMenuSpriteId];
    m_memBackgroundTitleMenu = &vectSprite[titleBackgroundMenuSpriteId];
    m_memBackgroundLeftMenu = &vectSprite[leftBackgroundMenuSpriteId];
    m_memBackgroundRightLeftMenu = &vectSprite[rightLeftBackgroundMenuSpriteId];
    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerComp);
    playerComp->m_menuMode = MenuMode_e::TITLE;
    setMenuEntries(*playerComp);
    m_gamePaused = true;
    m_titleMenuMode = true;
    //prevent to exit
    m_currentLevelState = LevelState_e::EXIT;
    Ecsm_t::instance().updateEntitiesFromSystems();

    //game paused
    do
    {
        m_graphicEngine.runIteration(m_gamePaused);
        m_physicalEngine.runIteration(m_gamePaused);
        if(m_graphicEngine.windowShouldClose())
        {
            return {LevelState_e::EXIT, 0, false};
        }
        if(m_physicalEngine.toogledFullScreenSignal())
        {
            m_graphicEngine.toogleFullScreen();
            validDisplayMenu();
            m_physicalEngine.reinitToggleFullScreen();
        }
    }while(m_currentLevelState != LevelState_e::NEW_GAME && m_currentLevelState != LevelState_e::LOAD_GAME);
    m_titleMenuMode = false;
    uint32_t levelToLoad = m_levelToLoad->first;
    m_levelToLoad = {};
    return {m_currentLevelState, levelToLoad, playerComp->m_previousMenuMode == MenuMode_e::LOAD_CUSTOM_LEVEL};
}

//===================================================================
LevelState MainEngine::mainLoop(uint32_t levelNum, LevelState_e levelState, bool afterLoadFailure, bool customLevel)
{
    m_levelEnd = false;
    m_currentLevelState = levelState;
    m_currentLevel = levelNum;
    m_graphicEngine.getMapSystem().confLevelData();
    m_physicalEngine.updateMousePos();
    if(!afterLoadFailure)
    {
        initLevel(levelNum, levelState);
        if(!m_graphicEngine.prologueEmpty() && !m_memCheckpointData)
        {
            displayTransitionMenu(MenuMode_e::LEVEL_PROLOGUE);
        }
        if(levelState != LevelState_e::LOAD_GAME)
        {
            if(levelState == LevelState_e::RESTART_FROM_CHECKPOINT || (levelState == LevelState_e::GAME_OVER && m_memCheckpointData))
            {
                assert(m_memCheckpointData);
                assert(m_currentSave);
            }
            else
            {
                if(levelState == LevelState_e::RESTART_LEVEL)
                {
                    m_graphicEngine.setRestartLevelMode();
                }
                saveGameProgress(levelNum);
            }
        }
    }
    std::chrono::duration<double> elapsed_seconds/*, fps*/;
    Ecsm_t::instance().updateEntitiesFromSystems();
    m_graphicEngine.unsetTransition(m_gamePaused);
    std::chrono::time_point<std::chrono::system_clock> clock/*, clockFrame*/;
    clock = std::chrono::system_clock::now();
    m_physicalEngine.updateMousePos();
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerConf);
    do
    {
        elapsed_seconds = std::chrono::system_clock::now() - clock;
        if(FPS_VALUE > elapsed_seconds.count())
        {
            continue;
        }
        //display FPS
//        fps = std::chrono::system_clock::now() - clockFrame;
//        std::cout << 1.0f / fps.count() << "  " << fps.count() << " FPS\n";
//        clockFrame = std::chrono::system_clock::now();
        //UpdateEntities TMP
        Ecsm_t::instance().updateEntitiesFromSystems();
        clock = std::chrono::system_clock::now();
        m_physicalEngine.runIteration(m_gamePaused);
        //LOAD if level to load break the loop
        if(m_levelToLoad)
        {
            if(!m_memCheckpointLevelState)
            {
                m_memEnemiesStateFromCheckpoint.clear();
            }
            uint32_t levelToLoad = m_levelToLoad->first;
            bool customLevelMode = m_levelToLoad->second;
            m_levelToLoad = {};
            if(m_currentLevelState == LevelState_e::NEW_GAME || m_currentLevelState == LevelState_e::RESTART_LEVEL)
            {
                if(m_currentLevelState == LevelState_e::NEW_GAME)
                {
                    m_playerMemGear = false;
                }
                clearCheckpointData();
            }
            return {m_currentLevelState, levelToLoad, customLevelMode};
        }
        clearObjectToDelete();
        if(m_physicalEngine.toogledFullScreenSignal())
        {
            m_graphicEngine.toogleFullScreen();
            validDisplayMenu();
            m_physicalEngine.reinitToggleFullScreen();
        }
        m_graphicEngine.runIteration(m_gamePaused);
        //MUUUUUUUUUUUUSSSSS
        m_audioEngine.runIteration();
        if(playerConf->m_checkpointReached)
        {
            //MEM ENTITIES TO DELETE WHEN CHECKPOINT IS REACHED
            m_currentEntitiesDelete = m_memStaticEntitiesDeletedFromCheckpoint;
            saveGameProgressCheckpoint(levelNum, *playerConf->m_checkpointReached, *playerConf->m_currentCheckpoint);
            playerConf->m_checkpointReached = {};
        }
        //level end
        if(m_levelEnd)
        {
            m_currentLevelState = LevelState_e::LEVEL_END;
            clearCheckpointData();
            //end level
            playerConf->m_inMovement = false;
            playerConf->m_infoWriteData = {false, ""};
            savePlayerGear(true);
            m_graphicEngine.setTransition(m_gamePaused);
            displayTransitionMenu();
            if(!m_graphicEngine.epilogueEmpty())
            {
                displayTransitionMenu(MenuMode_e::LEVEL_EPILOGUE);
            }
            m_memEnemiesStateFromCheckpoint.clear();
            return {m_currentLevelState, {}, customLevel};
        }
        //Player dead
        else if(!playerConf->m_life)
        {
            playerConf->m_playerShoot = false;
            playerConf->m_infoWriteData = {false, ""};
            AudioComponent *audioComp = Ecsm_t::instance().getComponent<AudioComponent, Components_e::AUDIO_COMPONENT>(m_playerEntity);
            assert(audioComp);
            //play death sound
            audioComp->m_soundElements[1]->m_toPlay = true;
            m_audioEngine.getSoundSystem()->execSystem();
            if(!m_memCheckpointLevelState)
            {
                clearCheckpointData();
            }
            //display red transition
            m_graphicEngine.setTransition(m_gamePaused, true);
            displayTransitionMenu(MenuMode_e::TRANSITION_LEVEL, true);
            return {LevelState_e::GAME_OVER, {}, customLevel};
        }
    }while(!m_graphicEngine.windowShouldClose());
    return {LevelState_e::EXIT, {}, customLevel};
}

//===================================================================
void MainEngine::initLevel(uint32_t levelNum, LevelState_e levelState)
{
    bool beginLevel = isLoadFromLevelBegin(m_currentLevelState);
    if(beginLevel)
    {
        PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
        assert(playerConf);
        m_memCheckpointLevelState = std::nullopt;
        playerConf->m_currentCheckpoint->first = 0;
        if(levelState == LevelState_e::NEW_GAME)
        {
            m_graphicEngine.updateSaveNum(levelNum, m_currentSave, 0, "", true);
        }
        else if(!m_memCustomLevelLoadedData)
        {
            m_graphicEngine.updateSaveNum(levelNum, m_currentSave, {}, "", true);
        }
    }
    //don't load gear for custom level
    if(!m_memCustomLevelLoadedData)
    {
        if(m_playerMemGear)
        {
            loadPlayerGear(beginLevel);
        }
        else
        {
            savePlayerGear(beginLevel);
            saveGameProgress(m_currentLevel);
        }
    }
    //display FPS
    //    std::chrono::duration<double> fps;
    //    std::chrono::time_point<std::chrono::system_clock> clockFrame  = std::chrono::system_clock::now();
    if(m_currentLevelState == LevelState_e::NEW_GAME || m_currentLevelState == LevelState_e::LOAD_GAME ||
            m_currentLevelState == LevelState_e::RESTART_LEVEL || m_currentLevelState == LevelState_e::RESTART_FROM_CHECKPOINT)
    {
        m_vectMemPausedTimer.clear();
        if(m_gamePaused)
        {
            setUnsetPaused();
        }
    }
    if(m_memCheckpointLevelState)
    {
        loadGameProgressCheckpoint();
    }
    else
    {
        m_memStaticEntitiesDeletedFromCheckpoint.clear();
        m_currentEntitiesDelete.clear();
    }
}

//===================================================================
void MainEngine::saveGameProgressCheckpoint(uint32_t levelNum, const PairUI_t &checkpointReached,
                                            const std::pair<uint32_t, Direction_e> &checkpointData)
{
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerConf);
    uint32_t enemiesKilled = (playerConf->m_enemiesKilled) ? *playerConf->m_enemiesKilled : 0;
    uint32_t secretsFound = (playerConf->m_secretsFound) ? *playerConf->m_secretsFound : 0;
    m_memCheckpointLevelState = {levelNum, checkpointData.first, secretsFound, enemiesKilled, checkpointData.second,
                                 checkpointReached};
    //OOOK SAVE GEAR BEGIN LEVEL
    savePlayerGear(false);
    saveEnemiesCheckpoint();
    std::vector<PairUI_t> revealedMap;
    const std::map<uint32_t, PairUI_t> &map = m_graphicEngine.getMapSystem().getDetectedMapData();
    revealedMap.reserve(map.size());
    for(std::map<uint32_t, PairUI_t>::const_iterator it = map.begin(); it != map.end(); ++it)
    {
        revealedMap.emplace_back(it->second);
    }
    m_memCheckpointData = {checkpointData.first, secretsFound, enemiesKilled, checkpointReached,
                           checkpointData.second, m_memEnemiesStateFromCheckpoint,
                           m_memMoveableWallCheckpointData, m_memTriggerWallMoveableWallCheckpointData,
                           m_memStaticEntitiesDeletedFromCheckpoint, revealedMap, playerConf->m_card};
    saveGameProgress(m_currentLevel, m_currentSave, &(*m_memCheckpointData));
}

//===================================================================
void MainEngine::saveEnemiesCheckpoint()
{
    std::vector<MemCheckpointEnemiesState> vectEnemiesData;
    vectEnemiesData.reserve(m_memEnemiesStateFromCheckpoint.size());
    for(uint32_t i = 0; i < m_memEnemiesStateFromCheckpoint.size(); ++i)
    {
        EnemyConfComponent *enemyComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(m_memEnemiesStateFromCheckpoint[i].m_entityNum);
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_memEnemiesStateFromCheckpoint[i].m_entityNum);
        assert(enemyComp);
        assert(mapComp);
        m_memEnemiesStateFromCheckpoint[i].m_dead = (enemyComp->m_behaviourMode == EnemyBehaviourMode_e::DEAD ||
                                                     enemyComp->m_behaviourMode == EnemyBehaviourMode_e::DYING);
        m_memEnemiesStateFromCheckpoint[i].m_enemyPos = mapComp->m_absoluteMapPositionPX;
        m_memEnemiesStateFromCheckpoint[i].m_objectPickedUp = false;
        m_memEnemiesStateFromCheckpoint[i].m_life = enemyComp->m_life;
        if(enemyComp->m_dropedObjectEntity)
        {
            //check if entity still exists
            GeneralCollisionComponent *genConf = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(*enemyComp->m_dropedObjectEntity);
            if(!genConf)
            {
                m_memEnemiesStateFromCheckpoint[i].m_objectPickedUp = true;
            }
        }
    }
}

//===================================================================
void MainEngine::saveGameProgress(uint32_t levelNum, std::optional<uint32_t> numSaveFile,
                                  const MemCheckpointElementsState *checkpointData)
{
    uint32_t saveNum = numSaveFile ? *numSaveFile : m_currentSave;
    if(!checkpointData)
    {
        m_memCheckpointLevelState = std::nullopt;
        if(m_memCustomLevelLoadedData)
        {
            m_memCustomLevelLoadedData->m_playerConfCheckpoint = m_memPlayerConfBeginLevel;
        }
    }
    if(checkpointData)
    {
        if(!m_memCustomLevelLoadedData)
        {
            m_graphicEngine.updateGraphicCheckpointData(checkpointData, saveNum);
        }
        else
        {
            m_memCustomLevelLoadedData->m_checkpointLevelData = *checkpointData;
            m_memCustomLevelLoadedData->m_playerConfCheckpoint = m_memPlayerConfCheckpoint;
            memCustomLevelRevealedMap();
        }
    }
    if(!m_memCustomLevelLoadedData)
    {
        std::string date = m_refGame->saveGameProgressINI(m_memPlayerConfBeginLevel, m_memPlayerConfCheckpoint,
                                                          levelNum, saveNum, checkpointData);
        m_graphicEngine.updateSaveNum(levelNum, saveNum, {}, date);
    }
}

//===================================================================
void MainEngine::memCustomLevelRevealedMap()
{
    const std::map<uint32_t, PairUI_t> &revealedMap = m_graphicEngine.getMapSystem().getRevealedMap();
    m_revealedMapData.clear();
    m_revealedMapData.reserve(revealedMap.size());
    for(std::map<uint32_t, PairUI_t>::const_iterator it = revealedMap.begin(); it != revealedMap.end(); ++it)
    {
        m_revealedMapData.emplace_back(it->second);
    }
}

//===================================================================
void MainEngine::savePlayerGear(bool beginLevel)
{
    if(m_memCustomLevelLoadedData)
    {
        return;
    }
    PlayerConfComponent *playerConfComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    WeaponComponent *weaponConf = Ecsm_t::instance().getComponent<WeaponComponent, Components_e::WEAPON_COMPONENT>(playerConfComp->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::WEAPON)]);
    assert(playerConfComp);
    assert(weaponConf);
    MemPlayerConf &playerConf = beginLevel ? m_memPlayerConfBeginLevel : m_memPlayerConfCheckpoint;
    playerConf.m_ammunationsCount.resize(weaponConf->m_weaponsData.size());
    playerConf.m_weapons.resize(weaponConf->m_weaponsData.size());
    for(uint32_t i = 0; i < playerConf.m_ammunationsCount.size(); ++i)
    {
        playerConf.m_ammunationsCount[i] =
                weaponConf->m_weaponsData[i].m_ammunationsCount;
        playerConf.m_weapons[i] = weaponConf->m_weaponsData[i].m_posses;
    }
    playerConf.m_currentWeapon = weaponConf->m_currentWeapon;
    playerConf.m_previousWeapon = weaponConf->m_previousWeapon;
    playerConf.m_life = playerConfComp->m_life;
    m_playerMemGear = true;
    if(beginLevel)
    {
        m_memCheckpointLevelState = std::nullopt;
    }
}

//===================================================================
void MainEngine::unsetFirstLaunch()
{
    PlayerConfComponent *playerConfComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerConfComp);
    playerConfComp->m_firstMenu = false;
}

//===================================================================
void MainEngine::instanciateSystems()
{
    Ecsm_t::instance().addNewSystem(std::make_unique<ColorDisplaySystem>());
    Ecsm_t::instance().addNewSystem(std::make_unique<MapDisplaySystem>());
    Ecsm_t::instance().addNewSystem(std::make_unique<InputSystem>());
    Ecsm_t::instance().addNewSystem(std::make_unique<CollisionSystem>());
    Ecsm_t::instance().addNewSystem(std::make_unique<VisionSystem>());
    Ecsm_t::instance().addNewSystem(std::make_unique<StaticDisplaySystem>());
    Ecsm_t::instance().addNewSystem(std::make_unique<IASystem>());
    Ecsm_t::instance().addNewSystem(std::make_unique<SoundSystem>());
    Ecsm_t::instance().addNewSystem(std::make_unique<GravitySystem>());
}

//===================================================================
void MainEngine::clearMemSoundElements()
{
    m_memSoundElements.m_teleports = std::nullopt;
    m_memSoundElements.m_enemies = std::nullopt;
    m_memSoundElements.m_barrels = std::nullopt;
    m_memSoundElements.m_damageZone = std::nullopt;
    m_memSoundElements.m_visibleShots = std::nullopt;
}

//===================================================================
void MainEngine::loadPlayerGear(bool beginLevel)
{
    PlayerConfComponent *playerConfComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    WeaponComponent *weaponConf = Ecsm_t::instance().getComponent<WeaponComponent, Components_e::WEAPON_COMPONENT>(playerConfComp->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::WEAPON)]);
    assert(playerConfComp);
    assert(weaponConf);
    MemPlayerConf &playerConf = beginLevel ? m_memPlayerConfBeginLevel : m_memPlayerConfCheckpoint;
    assert(playerConf.m_ammunationsCount.size() == weaponConf->m_weaponsData.size());
    for(uint32_t i = 0; i < playerConf.m_ammunationsCount.size(); ++i)
    {
        weaponConf->m_weaponsData[i].m_ammunationsCount = playerConf.m_ammunationsCount[i];
        weaponConf->m_weaponsData[i].m_posses = playerConf.m_weapons[i];
    }
    weaponConf->m_currentWeapon = playerConf.m_currentWeapon;
    weaponConf->m_previousWeapon = playerConf.m_previousWeapon;
    playerConfComp->m_life = playerConf.m_life;

    StaticDisplaySystem *staticDisplay = Ecsm_t::instance().getSystem<StaticDisplaySystem>(static_cast<uint32_t>(Systems_e::STATIC_DISPLAY_SYSTEM));
    assert(staticDisplay);
    //update FPS weapon sprite
    //weapon type weapon sprite
    staticDisplay->setWeaponSprite(playerConfComp->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::WEAPON)],
            weaponConf->m_weaponsData[weaponConf->m_currentWeapon].m_memPosSprite.first);
}

//===================================================================
void MainEngine::displayTransitionMenu(MenuMode_e mode, bool redTransition)
{
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerConf);
    float topEpiloguePosition;
    playerConf->m_menuMode = mode;
    setMenuEntries(*playerConf);
    WriteComponent *writeConf = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::MENU_ENTRIES)]);
    assert(writeConf);
    if(mode == MenuMode_e::LEVEL_EPILOGUE)
    {
        topEpiloguePosition = getTopEpilogueVerticalPosition(*writeConf);
        writeConf->m_upLeftPositionGL.second = -1.0f;
        m_audioEngine.playEpilogueMusic();
    }
    m_gamePaused = true;
    m_physicalEngine.setModeTransitionMenu(true);
    m_graphicEngine.mainDisplay(m_gamePaused);
    playerConf->m_currentCursorPos = 0;
    m_graphicEngine.unsetTransition(m_gamePaused, redTransition);
    do
    {
        m_graphicEngine.runIteration(m_gamePaused);
        m_physicalEngine.runIteration(m_gamePaused);
        if(mode == MenuMode_e::LEVEL_EPILOGUE)
        {
            writeConf->m_upLeftPositionGL.second += 0.005f;
            if(writeConf->m_upLeftPositionGL.second > topEpiloguePosition)
            {
                m_gamePaused = false;
            }
        }
    }while(m_gamePaused);
    m_physicalEngine.setModeTransitionMenu(false);
    m_graphicEngine.setTransition(true);
    if(playerConf->m_menuMode == MenuMode_e::TRANSITION_LEVEL)
    {
        m_graphicEngine.fillMenuWrite(*writeConf, MenuMode_e::BASE);
    }
}

//===================================================================
float getTopEpilogueVerticalPosition(const WriteComponent &writeComp)
{
    uint32_t nbLine = 1;
    std::string::size_type pos = 0;
    do
    {
        pos = writeComp.m_vectMessage[0].second.find('\\', ++pos);
        if(pos != std::string::npos)
        {
            ++nbLine;
        }
        else
        {
            break;
        }
    }while(true);
    //base position + line y size * line number
    return 1.0f + (writeComp.m_fontSize * 2.0f) * nbLine;
}

//===================================================================
void MainEngine::confPlayerVisibleShoot(std::vector<uint32_t> &playerVisibleShots,
                                        const PairFloat_t &point, float degreeAngle)
{
    m_physicalEngine.confPlayerVisibleShoot(playerVisibleShots, point, degreeAngle);
}

//===================================================================
void MainEngine::playerAttack(uint32_t playerEntity, PlayerConfComponent &playerComp, const PairFloat_t &point)
{
    WeaponComponent *weaponConf = Ecsm_t::instance().getComponent<WeaponComponent,
                                                                  Components_e::WEAPON_COMPONENT>(playerComp.m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::WEAPON)]);
    assert(weaponConf);
    assert(weaponConf->m_currentWeapon < weaponConf->m_weaponsData.size());
    WeaponData &currentWeapon = weaponConf->m_weaponsData[
            weaponConf->m_currentWeapon];
    AttackType_e attackType = currentWeapon.m_attackType;
    if(attackType == AttackType_e::MELEE)
    {
        GeneralCollisionComponent *actionGenColl = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(
            playerComp.m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::HIT_MELEE)]);
        MoveableComponent *playerMoveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(playerEntity);
        MapCoordComponent *actionMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(playerComp.m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::HIT_MELEE)]);
        MapCoordComponent *playerMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(playerEntity);
        assert(actionGenColl);
        assert(playerMoveComp);
        assert(actionMapComp);
        assert(playerMapComp);
        std::optional<PairUI_t> coord = getLevelCoord(actionMapComp->m_absoluteMapPositionPX);
        if(coord)
        {
            addEntityToZone(playerComp.m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::HIT_MELEE)], *coord);
        }
        confActionShape(*actionMapComp, *actionGenColl, *playerMapComp, *playerMoveComp);
        return;
    }
    else if(attackType == AttackType_e::BULLETS)
    {
        if(currentWeapon.m_simultaneousShots == 1)
        {
            // confPlayerBullet(&playerComp, point, degreeAngle, currentWeapon.m_currentBullet);
            if(currentWeapon.m_currentBullet < MAX_SHOTS - 1)
            {
                ++currentWeapon.m_currentBullet;
            }
            else
            {
                currentWeapon.m_currentBullet = 0;
            }
        }
        else
        {
            for(uint32_t i = 0; i < currentWeapon.m_simultaneousShots; ++i)
            {
                // confPlayerBullet(&playerComp, point, degreeAngle, i);
            }
        }
    }
    else if(attackType == AttackType_e::VISIBLE_SHOTS)
    {
        assert(currentWeapon.m_visibleShootEntities);
        if(playerComp.m_currentAim[static_cast<uint32_t>(PlayerAimDirection_e::UP)])
        {
            if(playerComp.m_currentAim[static_cast<uint32_t>(PlayerAimDirection_e::RIGHT)])
            {
                confPlayerVisibleShoot((*currentWeapon.m_visibleShootEntities), point, 45.0f);
            }
            else if(playerComp.m_currentAim[static_cast<uint32_t>(PlayerAimDirection_e::LEFT)])
            {
                confPlayerVisibleShoot((*currentWeapon.m_visibleShootEntities), point, 135.0f);
            }
            else
            {
                confPlayerVisibleShoot((*currentWeapon.m_visibleShootEntities), point, 90.0f);
            }
        }
        else if(playerComp.m_currentAim[static_cast<uint32_t>(PlayerAimDirection_e::DOWN)])
        {
            if(playerComp.m_currentAim[static_cast<uint32_t>(PlayerAimDirection_e::RIGHT)])
            {
                confPlayerVisibleShoot((*currentWeapon.m_visibleShootEntities), point, -45.0f);
            }
            else if(playerComp.m_currentAim[static_cast<uint32_t>(PlayerAimDirection_e::LEFT)])
            {
                confPlayerVisibleShoot((*currentWeapon.m_visibleShootEntities), point, 225.0f);
            }
            else
            {
                GravityComponent *gravityComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(playerEntity);
                assert(gravityComp);
                //if in the air shoot down
                if(!gravityComp->m_memOnGround)
                {
                    confPlayerVisibleShoot((*currentWeapon.m_visibleShootEntities), point, -90.0f);
                }
                else
                {
                    treatBasicDirectionShoot(playerComp, currentWeapon, point);
                }
            }
        }
        else
        {
            treatBasicDirectionShoot(playerComp, currentWeapon, point);
        }
    }
    assert(weaponConf->m_weaponsData[weaponConf->m_currentWeapon].m_ammunationsCount > 0);
    --weaponConf->m_weaponsData[weaponConf->m_currentWeapon].m_ammunationsCount;
}

//===================================================================
void MainEngine::treatBasicDirectionShoot(PlayerConfComponent &playerComp, WeaponData &currentWeapon, const PairFloat_t &point)
{
    if(playerComp.m_currentDirectionRight)
    {
        confPlayerVisibleShoot((*currentWeapon.m_visibleShootEntities), point, 0.0f);
    }
    else
    {
        confPlayerVisibleShoot((*currentWeapon.m_visibleShootEntities), point, 180.0f);
    }
}

//===================================================================
void MainEngine::confPlayerBullet(PlayerConfComponent *playerComp,
                                  const PairFloat_t &point, float degreeAngle,
                                  uint32_t numBullet)
{
    assert(numBullet < MAX_SHOTS);
    WeaponComponent *weaponComp = Ecsm_t::instance().getComponent<WeaponComponent, Components_e::WEAPON_COMPONENT>(playerComp->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::WEAPON)]);
    assert(weaponComp);
    uint32_t bulletEntity = (*weaponComp->m_weaponsData[weaponComp->m_currentWeapon].m_segmentShootEntities)[numBullet];
    GeneralCollisionComponent *genColl = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(bulletEntity);
    SegmentCollisionComponent *segmentColl = Ecsm_t::instance().getComponent<SegmentCollisionComponent, Components_e::SEGMENT_COLLISION_COMPONENT>(bulletEntity);
    ShotConfComponent *shotComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(bulletEntity);
    MoveableComponent *moveImpactComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(shotComp->m_impactEntity);
    assert(genColl);
    assert(segmentColl);
    assert(shotComp);
    assert(moveImpactComp);
    segmentColl->m_impactEntity = shotComp->m_impactEntity;
    confBullet(*genColl, *segmentColl, *moveImpactComp, CollisionTag_e::BULLET_PLAYER_CT, point, degreeAngle);
}

//===================================================================
void MainEngine::createPlayerImpactEntities(const std::vector<SpriteData> &vectSpriteData, WeaponComponent &weaponConf,
                                            const MapImpactData_t &mapImpactData)
{
    for(uint32_t i = 0; i < weaponConf.m_weaponsData.size(); ++i)
    {
        if(weaponConf.m_weaponsData[i].m_attackType == AttackType_e::BULLETS)
        {
            MapImpactData_t::const_iterator it =
                mapImpactData.find(weaponConf.m_weaponsData[i].m_impactID);
            assert(it != mapImpactData.end());
            assert(weaponConf.m_weaponsData[i].m_segmentShootEntities);
            for(uint32_t j = 0; j < weaponConf.m_weaponsData[i].m_segmentShootEntities->size(); ++j)
            {
                ShotConfComponent *shotComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(
                    (*weaponConf.m_weaponsData[i].m_segmentShootEntities)[j]);
                shotComp->m_impactEntity = confShotImpactEntity(vectSpriteData, it->second);
            }
        }
    }
}

//===================================================================
void confActionShape(MapCoordComponent &mapCompAction, GeneralCollisionComponent &genCompAction,
                     const MapCoordComponent &attackerMapComp, const MoveableComponent &attackerMoveComp)
{
    mapCompAction.m_absoluteMapPositionPX = attackerMapComp.m_absoluteMapPositionPX;
    moveElementFromAngle(LEVEL_HALF_TILE_SIZE_PX, getRadiantAngle(attackerMoveComp.m_degreeOrientation),
                         mapCompAction.m_absoluteMapPositionPX);
    genCompAction.m_active = true;
}

//===================================================================
void confBullet(GeneralCollisionComponent &genColl,
                SegmentCollisionComponent &segmentColl, MoveableComponent &moveImpactComp,
                CollisionTag_e collTag, const PairFloat_t &point, float degreeAngle)
{
    assert(collTag == CollisionTag_e::BULLET_ENEMY_CT || collTag == CollisionTag_e::BULLET_PLAYER_CT);
    moveImpactComp.m_degreeOrientation = degreeAngle;
    genColl.m_tagA = collTag;
    genColl.m_shape = CollisionShape_e::SEGMENT_C;
    genColl.m_active = true;
    float diff = std::rand() / ((RAND_MAX + 1u) / 9) - 4.0f;
    segmentColl.m_degreeOrientation = degreeAngle + diff;
    if(segmentColl.m_degreeOrientation < EPSILON_FLOAT)
    {
        segmentColl.m_degreeOrientation += 360.0f;
    }
    segmentColl.m_points.first = point;
}

//===================================================================
void MainEngine::setUnsetPaused()
{
    m_gamePaused = !m_gamePaused;
    if(m_gamePaused)
    {
        PlayerConfComponent *playerConfComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
        assert(playerConfComp);
        playerConfComp->m_currentCursorPos = 0;
        memTimerPausedValue();
    }
    else
    {
        applyTimerPausedValue();
    }
}

//===================================================================
void MainEngine::clearLevel()
{
    m_audioEngine.clearSourceAndBuffer();
    m_physicalEngine.clearSystems();
    m_graphicEngine.clearSystems();
    m_memTriggerCreated.clear();
    Ecsm_t::instance().clearEntities();
    m_memWall.clear();
    clearMemSoundElements();
}

//===================================================================
void MainEngine::confSystems()
{
    m_graphicEngine.confSystems();
}

//===================================================================
void MainEngine::clearObjectToDelete()
{
    const std::vector<uint32_t> &vect = m_physicalEngine.getStaticEntitiesToDelete();
    const std::vector<uint32_t> &vectBarrelsCheckpointRem = m_physicalEngine.getBarrelEntitiesDestruct();
    const std::vector<uint32_t> &vectBarrels = m_graphicEngine.getBarrelEntitiesToDelete();
    if(vect.empty() && vectBarrels.empty() && vectBarrelsCheckpointRem.empty())
    {
        return;
    }
    for(uint32_t i = 0; i < vect.size(); ++i)
    {
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(vect[i]);
        assert(mapComp);
        m_memStaticEntitiesDeletedFromCheckpoint.insert(mapComp->m_coord);
        Ecsm_t::instance().removeEntity(vect[i]);
        removeEntityToZone(vect[i]);
    }
    //mem destruct barrel current checkpoint
    for(uint32_t i = 0; i < vectBarrelsCheckpointRem.size(); ++i)
    {
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(vectBarrelsCheckpointRem[i]);
        assert(mapComp);
        m_memStaticEntitiesDeletedFromCheckpoint.insert(mapComp->m_coord);
    }
    //clear barrel current game entities
    for(uint32_t i = 0; i < vectBarrels.size(); ++i)
    {
        Ecsm_t::instance().removeEntity(vectBarrels[i]);
        removeEntityToZone(vectBarrels[i]);
    }
    m_physicalEngine.clearVectObjectToDelete();
    m_physicalEngine.clearVectBarrelsDestruct();
    m_graphicEngine.clearBarrelEntitiesToDelete();
}

//===================================================================
void MainEngine::memTimerPausedValue()
{
    std::set<uint32_t> setCacheComp;
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> arrEntities;
    arrEntities[Components_e::TIMER_COMPONENT] = 1;
    setCacheComp.insert(Components_e::TIMER_COMPONENT);
    std::optional<std::set<uint32_t>> usedEntities = Ecsm_t::instance().getEntitiesCustomComponents(setCacheComp, arrEntities);
    assert(usedEntities);
    assert(m_vectMemPausedTimer.empty());
    m_vectMemPausedTimer.reserve((*usedEntities).size());
    for(std::set<uint32_t>::const_iterator it = (*usedEntities).begin(); it != (*usedEntities).end(); ++it)
    {
        TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(*it);
        assert(timerComp);
        time_t time = (std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) -
                       std::chrono::system_clock::to_time_t(timerComp->m_clock));
        m_vectMemPausedTimer.emplace_back(*it, time);
    }
}

//===================================================================
void MainEngine::applyTimerPausedValue()
{
    for(uint32_t i = 0; i < m_vectMemPausedTimer.size(); ++i)
    {
        TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(m_vectMemPausedTimer[i].first);
        assert(timerComp);
        timerComp->m_clock = std::chrono::system_clock::from_time_t( std::chrono::system_clock::to_time_t(
                    std::chrono::system_clock::now()) - m_vectMemPausedTimer[i].second);
    }
    m_vectMemPausedTimer.clear();
}

//===================================================================
void MainEngine::loadColorEntities()
{
    uint32_t damageEntity = createColorEntity(),
            getObjectEntity = createColorEntity(),
            scratchEntity = createColorEntity(),
            transitionEntity = createColorEntity(),
            musicVolume = createColorEntity(),
            turnSensitivity = createColorEntity(),
            effectVolume = createColorEntity();
    confUnifiedColorEntity(transitionEntity, {0.0f, 0.0f, 0.0f}, true);
    confUnifiedColorEntity(damageEntity, {0.7f, 0.2f, 0.1f}, true);
    confUnifiedColorEntity(getObjectEntity, {0.1f, 0.7f, 0.5f}, true);
    confUnifiedColorEntity(scratchEntity, {0.0f, 0.0f, 0.0f}, false);
    confMenuBarMenuEntity(musicVolume, effectVolume, turnSensitivity);
     Ecsm_t::instance().getSystem<ColorDisplaySystem>(static_cast<uint32_t>(Systems_e::COLOR_DISPLAY_SYSTEM))->
            loadColorEntities(damageEntity, getObjectEntity, transitionEntity, scratchEntity, musicVolume, effectVolume, turnSensitivity);
}

//===================================================================
void MainEngine::confMenuBarMenuEntity(uint32_t musicEntity, uint32_t effectEntity, uint32_t turnSensitivity)
{
    //MUSIC VOLUME
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(musicEntity);
    ColorVertexComponent *colorComp = Ecsm_t::instance().getComponent<ColorVertexComponent, Components_e::COLOR_VERTEX_COMPONENT>(musicEntity);
    assert(posComp);
    assert(colorComp);
    float leftPos = LEFT_POS_STD_MENU_BAR, rightPos = leftPos + 0.01f + (getMusicVolume() * MAX_BAR_MENU_SIZE) / 100.0f,
    upPos = MAP_MENU_DATA.at(MenuMode_e::SOUND).first.second - 0.01f,
    downPos = upPos - (MENU_FONT_SIZE - 0.02f);
    if(!posComp->m_vertex.empty())
    {
        posComp->m_vertex.clear();
    }
    posComp->m_vertex.reserve(4);
    posComp->m_vertex.emplace_back(PairFloat_t{leftPos, upPos});
    posComp->m_vertex.emplace_back(PairFloat_t{rightPos, upPos});
    posComp->m_vertex.emplace_back(PairFloat_t{rightPos, downPos});
    posComp->m_vertex.emplace_back(PairFloat_t{leftPos, downPos});
    if(!colorComp->m_vertex.empty())
    {
        colorComp->m_vertex.clear();
    }
    colorComp->m_vertex.reserve(4);
    colorComp->m_vertex.emplace_back(TupleTetraFloat_t{0.5f, 0.0f, 0.0f, 1.0f});
    colorComp->m_vertex.emplace_back(TupleTetraFloat_t{0.5f, 0.0f, 0.0f, 1.0f});
    colorComp->m_vertex.emplace_back(TupleTetraFloat_t{0.5f, 0.0f, 0.0f, 1.0f});
    colorComp->m_vertex.emplace_back(TupleTetraFloat_t{0.5f, 0.0f, 0.0f, 1.0f});
    //EFFECT VOLUME
    PositionVertexComponent *posCompA = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(effectEntity);
    ColorVertexComponent *colorCompA = Ecsm_t::instance().getComponent<ColorVertexComponent, Components_e::COLOR_VERTEX_COMPONENT>(effectEntity);
    assert(posCompA);
    assert(colorCompA);
    upPos -= MENU_FONT_SIZE;
    downPos -= MENU_FONT_SIZE;
    rightPos = leftPos + 0.01f + (getEffectsVolume() * MAX_BAR_MENU_SIZE) / 100.0f;
    posCompA->m_vertex.reserve(4);
    posCompA->m_vertex.emplace_back(PairFloat_t{leftPos, upPos});
    posCompA->m_vertex.emplace_back(PairFloat_t{rightPos, upPos});
    posCompA->m_vertex.emplace_back(PairFloat_t{rightPos, downPos});
    posCompA->m_vertex.emplace_back(PairFloat_t{leftPos, downPos});
    colorCompA->m_vertex.reserve(4);
    colorCompA->m_vertex.emplace_back(TupleTetraFloat_t{0.5f, 0.0f, 0.0f, 1.0f});
    colorCompA->m_vertex.emplace_back(TupleTetraFloat_t{0.5f, 0.0f, 0.0f, 1.0f});
    colorCompA->m_vertex.emplace_back(TupleTetraFloat_t{0.5f, 0.0f, 0.0f, 1.0f});
    colorCompA->m_vertex.emplace_back(TupleTetraFloat_t{0.5f, 0.0f, 0.0f, 1.0f});
    //TURN SENSITIVITY
    PositionVertexComponent *posCompB = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(turnSensitivity);
    ColorVertexComponent *colorCompB = Ecsm_t::instance().getComponent<ColorVertexComponent, Components_e::COLOR_VERTEX_COMPONENT>(turnSensitivity);
    assert(posCompB);
    assert(colorCompB);
    upPos = (MAP_MENU_DATA.at(MenuMode_e::INPUT).first.second - 0.01f) -
            MENU_FONT_SIZE * static_cast<uint32_t>(InputMenuCursorPos_e::TURN_SENSITIVITY),
    downPos = upPos - MENU_FONT_SIZE;
    rightPos = 0.1f + ((getTurnSensitivity() - MIN_TURN_SENSITIVITY) * MAX_BAR_MENU_SIZE) / DIFF_TOTAL_SENSITIVITY;
    posCompB->m_vertex.reserve(4);
    posCompB->m_vertex.emplace_back(PairFloat_t{leftPos, upPos});
    posCompB->m_vertex.emplace_back(PairFloat_t{rightPos, upPos});
    posCompB->m_vertex.emplace_back(PairFloat_t{rightPos, downPos});
    posCompB->m_vertex.emplace_back(PairFloat_t{leftPos, downPos});
    colorCompB->m_vertex.reserve(4);
    colorCompB->m_vertex.emplace_back(TupleTetraFloat_t{0.5f, 0.0f, 0.0f, 1.0f});
    colorCompB->m_vertex.emplace_back(TupleTetraFloat_t{0.5f, 0.0f, 0.0f, 1.0f});
    colorCompB->m_vertex.emplace_back(TupleTetraFloat_t{0.5f, 0.0f, 0.0f, 1.0f});
    colorCompB->m_vertex.emplace_back(TupleTetraFloat_t{0.5f, 0.0f, 0.0f, 1.0f});
}


//===================================================================
void MainEngine::confUnifiedColorEntity(uint32_t entityNum, const tupleFloat_t &color, bool transparent)
{
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(entityNum);
    assert(posComp);
    if(!posComp->m_vertex.empty())
    {
        posComp->m_vertex.clear();
    }
    posComp->m_vertex.reserve(4);
    posComp->m_vertex.emplace_back(PairFloat_t{-1.0f, 1.0f});
    posComp->m_vertex.emplace_back(PairFloat_t{1.0f, 1.0f});
    posComp->m_vertex.emplace_back(PairFloat_t{1.0f, -1.0f});
    posComp->m_vertex.emplace_back(PairFloat_t{-1.0f, -1.0f});
    ColorVertexComponent *colorComp = Ecsm_t::instance().getComponent<ColorVertexComponent, Components_e::COLOR_VERTEX_COMPONENT>(entityNum);
    assert(colorComp);
    if(!colorComp->m_vertex.empty())
    {
        colorComp->m_vertex.clear();
    }
    colorComp->m_vertex.reserve(4);
    float alpha = transparent ? 0.4f : 1.0f;
    colorComp->m_vertex.emplace_back(TupleTetraFloat_t{std::get<0>(color), std::get<1>(color), std::get<2>(color), alpha});
    colorComp->m_vertex.emplace_back(TupleTetraFloat_t{std::get<0>(color), std::get<1>(color), std::get<2>(color), alpha});
    colorComp->m_vertex.emplace_back(TupleTetraFloat_t{std::get<0>(color), std::get<1>(color), std::get<2>(color), alpha});
    colorComp->m_vertex.emplace_back(TupleTetraFloat_t{std::get<0>(color), std::get<1>(color), std::get<2>(color), alpha});
}

//===================================================================
uint32_t MainEngine::createColorEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::COLOR_VERTEX_COMPONENT] = 1;
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createCheckpointEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::CHECKPOINT_COMPONENT] = 1;
    vect[Components_e::MAP_COORD_COMPONENT] = 1;
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    vect[Components_e::RECTANGLE_COLLISION_COMPONENT] = 1;
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createLogEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    vect[Components_e::SPRITE_TEXTURE_COMPONENT] = 1;
    vect[Components_e::MAP_COORD_COMPONENT] = 1;
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    vect[Components_e::CIRCLE_COLLISION_COMPONENT] = 1;
    vect[Components_e::LOG_COMPONENT] = 1;
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createSecretEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::MAP_COORD_COMPONENT] = 1;
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    vect[Components_e::RECTANGLE_COLLISION_COMPONENT] = 1;
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createTextureEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    vect[Components_e::SPRITE_TEXTURE_COMPONENT] = 1;
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
void MainEngine::loadBackgroundEntities(const GroundCeilingData &groundData, const GroundCeilingData &ceilingData,
                                        const LevelManager &levelManager)
{

    uint32_t entity, colorIndex = static_cast<uint32_t>(DisplayType_e::COLOR),
            simpleTextIndex = static_cast<uint32_t>(DisplayType_e::SIMPLE_TEXTURE),
            tiledTextIndex = static_cast<uint32_t>(DisplayType_e::TEXTURED_TILE);
    if(groundData.m_apparence[simpleTextIndex])
    {
        entity = createBackgroundEntity(false);
        confGroundSimpleTextBackgroundComponents(entity, groundData, levelManager.getPictureSpriteData());
        memGroundBackgroundFPSSystemEntity(entity, true);
    }
    else if(groundData.m_apparence[colorIndex])
    {
        entity = createBackgroundEntity(true);
        confColorBackgroundComponents(entity, groundData, true);
        memColorSystemEntity(entity);
    }
    if(groundData.m_apparence[tiledTextIndex])
    {
        entity = createBackgroundEntity(false);
        confTiledTextBackgroundComponents(entity, groundData, levelManager.getPictureSpriteData());
        memGroundBackgroundFPSSystemEntity(entity, false);
    }

    if(ceilingData.m_apparence[simpleTextIndex])
    {
        entity = createBackgroundEntity(false);
        confCeilingSimpleTextBackgroundComponents(entity, ceilingData, levelManager.getPictureSpriteData());
        memCeilingBackgroundFPSSystemEntity(entity, true);
    }
    else if(ceilingData.m_apparence[colorIndex])
    {
        entity = createBackgroundEntity(true);
        confColorBackgroundComponents(entity, ceilingData, false);
        memColorSystemEntity(entity);
    }
    if(ceilingData.m_apparence[tiledTextIndex])
    {
        entity = createBackgroundEntity(false);
        confTiledTextBackgroundComponents(entity, ceilingData, levelManager.getPictureSpriteData());
        memCeilingBackgroundFPSSystemEntity(entity, false);
    }
    loadFogEntities();
}

//===================================================================
void MainEngine::loadFogEntities()
{
    uint32_t entity = createBackgroundEntity(true);
    //GROUND
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(entity);
    ColorVertexComponent *colorComp = Ecsm_t::instance().getComponent<ColorVertexComponent, Components_e::COLOR_VERTEX_COMPONENT>(entity);
    assert(posComp);
    assert(colorComp);
    //ceiling + dark fog middle + Ground
    posComp->m_vertex.reserve(12);
    colorComp->m_vertex.reserve(12);
    //ceiling
    posComp->m_vertex.emplace_back(-1.0f, 0.3f);
    posComp->m_vertex.emplace_back(1.0f, 0.3f);
    posComp->m_vertex.emplace_back(1.0f, 0.12f);
    posComp->m_vertex.emplace_back(-1.0f, 0.12f);

    colorComp->m_vertex.emplace_back(0.0f, 0.0f, 0.0f, 0.0f);
    colorComp->m_vertex.emplace_back(0.0f, 0.0f, 0.0f, 0.0f);
    colorComp->m_vertex.emplace_back(0.0f, 0.0f, 0.0f, 1.0f);
    colorComp->m_vertex.emplace_back(0.0f, 0.0f, 0.0f, 1.0f);

    //fog
    posComp->m_vertex.emplace_back(-1.0f, 0.12f);
    posComp->m_vertex.emplace_back(1.0f, 0.12f);
    posComp->m_vertex.emplace_back(1.0f, -0.12f);
    posComp->m_vertex.emplace_back(-1.0f, -0.12f);

    colorComp->m_vertex.emplace_back(0.0f, 0.0f, 0.0f, 1.0f);
    colorComp->m_vertex.emplace_back(0.0f, 0.0f, 0.0f, 1.0f);
    colorComp->m_vertex.emplace_back(0.0f, 0.0f, 0.0f, 1.0f);
    colorComp->m_vertex.emplace_back(0.0f, 0.0f, 0.0f, 1.0f);

    //ground
    posComp->m_vertex.emplace_back(-1.0f, -0.12f);
    posComp->m_vertex.emplace_back(1.0f, -0.12f);
    posComp->m_vertex.emplace_back(1.0f, -0.3f);
    posComp->m_vertex.emplace_back(-1.0f, -0.3f);

    colorComp->m_vertex.emplace_back(0.0f, 0.0f, 0.0f, 1.0f);
    colorComp->m_vertex.emplace_back(0.0f, 0.0f, 0.0f, 1.0f);
    colorComp->m_vertex.emplace_back(0.0f, 0.0f, 0.0f, 0.0f);
    colorComp->m_vertex.emplace_back(0.0f, 0.0f, 0.0f, 0.0f);
    memFogColorEntity(entity);
}

//===================================================================
uint32_t MainEngine::createBackgroundEntity(bool color)
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    if(color)
    {
        vect[Components_e::COLOR_VERTEX_COMPONENT] = 1;
    }
    else
    {
        vect[Components_e::SPRITE_TEXTURE_COMPONENT] = 1;
    }
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
void MainEngine::loadGraphicPicture(const PictureData &picData, const FontData &fontData)
{
    m_graphicEngine.loadPictureData(picData, fontData);
}

//===================================================================
void MainEngine::loadExistingLevelNumSaves(const std::array<std::optional<DataLevelWriteMenu>, 3> &existingLevelNum)
{
    m_graphicEngine.loadExistingLevelNumSaves(existingLevelNum);
}

//===================================================================
void MainEngine::loadExistingCustomLevel(const std::vector<std::string> &customLevels)
{
    m_graphicEngine.loadExistingCustomLevel(customLevels);
}

//===================================================================
void MainEngine::loadLevel(const LevelManager &levelManager)
{
    m_memWallPos.clear();
    m_physicalEngine.clearVectObjectToDelete();
    m_physicalEngine.clearVectBarrelsDestruct();
    m_physicalEngine.updateZonesColl();
    m_graphicEngine.clearBarrelEntitiesToDelete();
    loadBackgroundEntities(levelManager.getPictureData().getGroundData(),
                           levelManager.getPictureData().getCeilingData(),
                           levelManager);
    Level::initLevelElementArray();
    loadPlayerEntity(levelManager);
    MapCoordComponent *map = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerEntity);
    assert(map);
    m_physicalEngine.addEntityToZone(m_playerEntity, map->m_coord);
    if(m_memCheckpointLevelState)
    {
        if(m_memCustomLevelLoadedData)
        {
            assert(m_memCustomLevelLoadedData->m_checkpointLevelData);
            loadCheckpointSavedGame(*m_memCustomLevelLoadedData->m_checkpointLevelData, true);
        }
        else
        {
            std::optional<MemLevelLoadedData> savedData = m_refGame->loadSavedGame(m_currentSave);
            assert(savedData);
            loadCheckpointSavedGame(*savedData->m_checkpointLevelData, true);
        }
    }
    bool exit = loadStaticElementEntities(levelManager);
    exit |= loadEnemiesEntities(levelManager);
    assert(exit);
    loadCheckpointsEntities(levelManager);
    loadSecretsEntities(levelManager);
    loadWallEntities(levelManager.getMoveableWallData(), levelManager.getPictureData().getSpriteData());
    loadLogsEntities(levelManager, levelManager.getPictureData().getSpriteData());
    loadRevealedMap();
    std::string prologue = treatInfoMessageEndLine(levelManager.getLevelPrologue()),
            epilogue = treatInfoMessageEndLine(levelManager.getLevelEpilogue(), 27);
    m_graphicEngine.updatePrologueAndEpilogue(prologue, epilogue);
    //MUUUUUUUUUUUUSSSSS
    m_audioEngine.memoriseEpilogueMusicFilename(levelManager.getLevelEpilogueMusic());
    m_audioEngine.loadMusicFromFile(levelManager.getLevel().getMusicFilename());
    m_audioEngine.playMusic();
}

//===================================================================
void MainEngine::loadGameProgressCheckpoint()
{
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerEntity);
    MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(m_playerEntity);
    PositionVertexComponent *pos = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(m_playerEntity);
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(mapComp);
    assert(moveComp);
    assert(pos);
    assert(playerConf);
    mapComp->m_absoluteMapPositionPX = getCenteredAbsolutePosition(m_memCheckpointLevelState->m_playerPos);
    moveComp->m_degreeOrientation = getDegreeAngleFromDirection(m_memCheckpointLevelState->m_direction);
    m_memStaticEntitiesDeletedFromCheckpoint = m_currentEntitiesDelete;
    updatePlayerArrow(*moveComp, *pos);
    playerConf->m_currentCheckpoint = {m_memCheckpointLevelState->m_checkpointNum, m_memCheckpointLevelState->m_direction};
    playerConf->m_enemiesKilled = m_memCheckpointLevelState->m_ennemiesKilled;
    playerConf->m_secretsFound = m_memCheckpointLevelState->m_secretsFound;
}

//===================================================================
uint32_t MainEngine::loadWeaponsEntity(const LevelManager &levelManager)
{
    uint32_t weaponEntity = createWeaponEntity(), weaponToTreat;
    const std::vector<SpriteData> &vectSprite = levelManager.getPictureData().getSpriteData();
    const std::vector<WeaponINIData> &vectWeapons = levelManager.getWeaponsData();
    MemSpriteDataComponent *memSprite = Ecsm_t::instance().getComponent<MemSpriteDataComponent, Components_e::MEM_SPRITE_DATA_COMPONENT>(weaponEntity);
    MemPositionsVertexComponents *memPosVertex = Ecsm_t::instance().getComponent<MemPositionsVertexComponents, Components_e::MEM_POSITIONS_VERTEX_COMPONENT>(weaponEntity);
    WeaponComponent *weaponConf = Ecsm_t::instance().getComponent<WeaponComponent, Components_e::WEAPON_COMPONENT>(weaponEntity);
    AudioComponent *audioComp = Ecsm_t::instance().getComponent<AudioComponent, Components_e::AUDIO_COMPONENT>(weaponEntity);
    assert(memSprite);
    assert(memPosVertex);
    assert(weaponConf);
    assert(audioComp);
    weaponConf->m_weaponsData.resize(vectWeapons.size());
    audioComp->m_soundElements.resize(vectWeapons.size());
    m_vectMemWeaponsDefault.resize(vectWeapons.size());
    std::fill(m_vectMemWeaponsDefault.begin(), m_vectMemWeaponsDefault.end(), std::pair<bool, uint32_t>{false, 0});
    for(uint32_t i = 0; i < weaponConf->m_weaponsData.size(); ++i)
    {
        weaponConf->m_weaponsData[i].m_ammunationsCount = 0;
        weaponConf->m_weaponsData[i].m_posses = false;
    }
    uint32_t totalSize = 0;
    for(uint32_t i = 0; i < vectWeapons.size(); ++i)
    {
        totalSize += vectWeapons[i].m_spritesData.size();
    }
    memSprite->m_vectSpriteData.reserve(totalSize);
    float posUp, posDown = DOWN_WEAPON_POS_Y, posLeft, posRight, diffLateral;
    memSprite->m_vectSpriteData.reserve(vectWeapons.size());
    for(uint32_t i = 0; i < vectWeapons.size(); ++i)
    {
        weaponToTreat = vectWeapons[i].m_order;
        if(vectWeapons[i].m_startingPossess)
        {
            m_vectMemWeaponsDefault[i].first = true;
            weaponConf->m_weaponsData[weaponToTreat].m_posses = true;
            weaponConf->m_currentWeapon = vectWeapons[i].m_order;
            weaponConf->m_previousWeapon = vectWeapons[i].m_order;
        }
        if(vectWeapons[i].m_startingAmmoCount)
        {
            m_vectMemWeaponsDefault[i].second = *vectWeapons[i].m_startingAmmoCount;
            weaponConf->m_weaponsData[weaponToTreat].m_ammunationsCount = *vectWeapons[i].m_startingAmmoCount;
        }
        weaponConf->m_weaponsData[weaponToTreat].m_weaponPower = vectWeapons[i].m_damage;
        weaponConf->m_weaponsData[weaponToTreat].m_animMode = vectWeapons[i].m_animMode;
        weaponConf->m_weaponsData[weaponToTreat].m_intervalLatency = vectWeapons[i].m_animationLatency / FPS_VALUE;
        weaponConf->m_weaponsData[weaponToTreat].m_visibleShotID = vectWeapons[i].m_visibleShootID;
        weaponConf->m_weaponsData[weaponToTreat].m_weaponName = vectWeapons[i].m_weaponName;
        weaponConf->m_weaponsData[weaponToTreat].m_impactID = vectWeapons[i].m_impactID;
        weaponConf->m_weaponsData[weaponToTreat].m_shotVelocity = vectWeapons[i].m_shotVelocity;
        weaponConf->m_weaponsData[weaponToTreat].m_maxAmmunations = vectWeapons[i].m_maxAmmo;
        weaponConf->m_weaponsData[weaponToTreat].m_memPosSprite = {memSprite->m_vectSpriteData.size(),
                                                                   memSprite->m_vectSpriteData.size() + vectWeapons[i].m_spritesData.size() - 1};
        weaponConf->m_weaponsData[weaponToTreat].m_lastAnimNum = memSprite->m_vectSpriteData.size() + vectWeapons[i].m_lastAnimNum;
        weaponConf->m_weaponsData[weaponToTreat].m_attackType = vectWeapons[i].m_attackType;
        weaponConf->m_weaponsData[weaponToTreat].m_simultaneousShots = vectWeapons[i].m_simultaneousShots;
        if(!vectWeapons[i].m_shotSound.empty())
        {
            audioComp->m_soundElements[weaponToTreat] = loadSound(vectWeapons[i].m_shotSound);
            m_audioEngine.memAudioMenuSound(audioComp->m_soundElements[weaponToTreat]->m_sourceALID);
        }
        if(!vectWeapons[i].m_reloadSound.empty())
        {
            weaponConf->m_reloadSoundAssociated.insert({weaponToTreat,
                                                       audioComp->m_soundElements.size()});
            audioComp->m_soundElements.push_back(loadSound(vectWeapons[i].m_reloadSound));
        }
        weaponConf->m_weaponsData[weaponToTreat].m_damageRay = vectWeapons[i].m_damageCircleRay;
        for(uint32_t j = 0; j < vectWeapons[i].m_spritesData.size(); ++j)
        {
            memSprite->m_vectSpriteData.emplace_back(&vectSprite[vectWeapons[i].m_spritesData[j].m_numSprite]);
            posUp = DOWN_WEAPON_POS_Y + vectWeapons[i].m_spritesData[j].m_GLSize.second;
            diffLateral = vectWeapons[i].m_spritesData[j].m_GLSize.first / 2.0f;
            posLeft = -diffLateral;
            posRight = diffLateral;
            memPosVertex->m_vectSpriteData.emplace_back(std::array<PairFloat_t, 4>{
                                                            {
                                                                {posLeft, posUp},
                                                                {posRight, posUp},
                                                                {posRight, posDown},
                                                                {posLeft, posDown}
                                                            }
                                                        });
        }
    }
    weaponConf->m_previewDisplayData = levelManager.getWeaponsPreviewData();
    assert(weaponConf->m_currentWeapon < vectWeapons.size());
    return weaponEntity;
}

//===================================================================
bool MainEngine::loadEnemiesEntities(const LevelManager &levelManager)
{
    const std::map<std::string, EnemyData> &enemiesData = levelManager.getEnemiesData();
    bool exit = false;
    std::array<SoundElement, 3> currentSoundElements;
    bool loadFromCheckpoint = (!m_memEnemiesStateFromCheckpoint.empty());
    m_currentLevelEnemiesNumber = 0;
    m_currentLevelEnemiesKilled = 0;
    for(std::map<std::string, EnemyData>::const_iterator it = enemiesData.begin(); it != enemiesData.end(); ++it)
    {
        currentSoundElements[0] = loadSound(it->second.m_detectBehaviourSoundFile);
        currentSoundElements[1] = loadSound(it->second.m_attackSoundFile);
        currentSoundElements[2] = loadSound(it->second.m_deathSoundFile);
        const SpriteData &memSpriteData = levelManager.getPictureData().
                getSpriteData()[it->second.m_staticFrontSprites[0]];
        for(uint32_t j = 0; j < it->second.m_TileGamePosition.size(); ++j)
        {
            exit |= createEnemy(levelManager, memSpriteData, it->second, loadFromCheckpoint, j, currentSoundElements, it->second.m_inGameSpriteSize);
        }
    }
    return exit;
}

//===================================================================
void MainEngine::loadWallEntities(const std::map<std::string, MoveableWallData> &wallData,
                                  const std::vector<SpriteData> &vectSprite)
{
    assert(!Level::getLevelCaseType().empty());
    // TriggerWallMoveType_e memTriggerType;
    bool moveable;
    uint32_t shapeNum = 0;
    std::vector<uint32_t> vectMemEntities;
    bool loadFromCheckpoint = (m_memCheckpointLevelState != std::nullopt);
    if(!loadFromCheckpoint)
    {
        m_memMoveableWallCheckpointData.clear();
        m_memTriggerWallMoveableWallCheckpointData.clear();
    }
    //Shape Wall Loop
    for(std::map<std::string, MoveableWallData>::const_iterator iter = wallData.begin(); iter != wallData.end(); ++iter, ++shapeNum)
    {
        vectMemEntities.clear();
        assert(!iter->second.m_sprites.empty());
        assert(iter->second.m_sprites[0] < vectSprite.size());
        moveable = !(iter->second.m_directionMove.empty());
        if(moveable)
        {
            if(!loadFromCheckpoint)
            {
                m_memTriggerWallMoveableWallCheckpointData.insert({shapeNum, {}});
                m_memTriggerWallMoveableWallCheckpointData[shapeNum].first.resize(iter->second.m_TileGamePosition.size());
            }
        }
        vectMemEntities = loadWallEntitiesWallLoop(vectSprite, *iter, moveable, shapeNum, loadFromCheckpoint);
    }
}

//===================================================================
std::vector<uint32_t> MainEngine::loadWallEntitiesWallLoop(const std::vector<SpriteData> &vectSprite,
                                                           const std::pair<std::string, MoveableWallData> &currentShape,
                                                           bool moveable, uint32_t shapeNum, bool loadFromCheckpoint)
{
    std::vector<uint32_t> vectMemEntities;
    const SpriteData &memSpriteData = vectSprite[currentShape.second.m_sprites[0]];
    uint32_t wallNum = 0;
    pairI_t moveableWallCorrectedPos;
    //Wall Loop
    for(std::set<PairUI_t>::const_iterator it = currentShape.second.m_TileGamePosition.begin();
         it != currentShape.second.m_TileGamePosition.end(); ++it, ++wallNum)
    {
        moveableWallCorrectedPos = {0, 0};
        if(currentShape.second.m_removeGamePosition.find(*it) != currentShape.second.m_removeGamePosition.end())
        {
            m_memWallPos.erase(*it);
            continue;
        }
        uint32_t numEntity = createWallEntity(currentShape.second.m_sprites.size() > 1, moveable);
        std::map<PairUI_t, uint32_t>::iterator itt = m_memWallPos.find(*it);
        if(itt != m_memWallPos.end())
        {
            Ecsm_t::instance().removeEntity(m_memWallPos[*it]);
            m_memWallPos[*it] = numEntity;
        }
        else
        {
            m_memWallPos.insert({*it, numEntity});
        }
        //if load from checkpoint
        if(loadFromCheckpoint)
        {
            if(!m_memMoveableWallCheckpointData.empty() && currentShape.second.m_triggerType != TriggerWallMoveType_e::WALL)
            {
                moveableWallCorrectedPos = getModifMoveableWallDataCheckpoint(currentShape.second.m_directionMove,
                                                                              m_memMoveableWallCheckpointData[shapeNum].first,
                                                                              currentShape.second.m_triggerBehaviourType);
            }
            else if(moveable && !m_memTriggerWallMoveableWallCheckpointData.empty() && currentShape.second.m_triggerType == TriggerWallMoveType_e::WALL)
            {
                assert(m_memTriggerWallMoveableWallCheckpointData.find(shapeNum) != m_memTriggerWallMoveableWallCheckpointData.end());
                assert(wallNum < m_memTriggerWallMoveableWallCheckpointData[shapeNum].first.size());
                moveableWallCorrectedPos = getModifMoveableWallDataCheckpoint(currentShape.second.m_directionMove,
                                                                              m_memTriggerWallMoveableWallCheckpointData[shapeNum].first[wallNum],
                                                                              currentShape.second.m_triggerBehaviourType);
            }
        }
        moveableWallCorrectedPos = {it->first + moveableWallCorrectedPos.first, it->second + moveableWallCorrectedPos.second};
        if(moveableWallCorrectedPos.first < 0)
        {
            moveableWallCorrectedPos.first = 0;
        }
        if(moveableWallCorrectedPos.second < 0)
        {
            moveableWallCorrectedPos.second = 0;
        }
        confBaseWallData(numEntity, memSpriteData, moveableWallCorrectedPos,
                         currentShape.second.m_triggerBehaviourType, moveable);
        if(!moveable)
        {
            continue;
        }
        vectMemEntities.emplace_back(numEntity);
    }
    return vectMemEntities;
}

//===================================================================
void MainEngine::confBaseWallData(uint32_t wallEntity, const SpriteData &memSpriteData,
                                  const PairUI_t& coordLevel, TriggerBehaviourType_e triggerType, bool moveable)
{
    confBaseComponent(wallEntity, memSpriteData, coordLevel,
                      CollisionShape_e::RECTANGLE_C, CollisionTag_e::WALL_CT);

    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(wallEntity);
    assert(spriteComp);
    LevelCaseType_e type = moveable ? LevelCaseType_e::WALL_MOVE_LC : LevelCaseType_e::WALL_LC;
    std::optional<ElementRaycast> element  = Level::getElementCase(coordLevel);
    if(moveable && triggerType != TriggerBehaviourType_e::AUTO &&
        (!element || element->m_typeStd != LevelCaseType_e::WALL_LC))
    {
        Level::memStaticMoveWallEntity(coordLevel, wallEntity);
    }
    Level::addElementCase(*spriteComp, coordLevel, type, wallEntity);
}

//===================================================================
bool MainEngine::createEnemy(const LevelManager &levelManager, const SpriteData &memSpriteData, const EnemyData &enemyData,
                             bool loadFromCheckpoint, uint32_t index, const std::array<SoundElement, 3> &soundElements, const std::pair<float, float> &inGameSpriteSize)
{
    bool exit = false;
    uint32_t numEntity = createEnemyEntity();
    confBaseComponent(numEntity, memSpriteData, enemyData.m_TileGamePosition[index],
                      CollisionShape_e::RECTANGLE_C, CollisionTag_e::ENEMY_CT, inGameSpriteSize);
    EnemyConfComponent *enemyComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(numEntity);
    assert(enemyComp);
    if(enemyData.m_endLevelPos && (*enemyData.m_endLevelPos) == enemyData.m_TileGamePosition[index])
    {
        enemyComp->m_endLevel = true;
        exit = true;
    }
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(numEntity);
    assert(spriteComp);
    spriteComp->m_displaySize = inGameSpriteSize;
    enemyComp->m_life = enemyData.m_life;
    enemyComp->m_visibleShot = !(enemyData.m_visibleShootID.empty());
    enemyComp->m_countTillLastAttack = 0;
    enemyComp->m_meleeOnly = enemyData.m_meleeOnly;
    enemyComp->m_frozenOnAttack = enemyData.m_frozenOnAttack;
    enemyComp->m_simultaneousShot = enemyData.m_simultaneousShot ? *enemyData.m_simultaneousShot : 1;
    if(enemyData.m_meleeDamage)
    {
        enemyComp->m_meleeAttackDamage = *enemyData.m_meleeDamage;
    }
    if(!enemyData.m_dropedObjectID.empty())
    {
        enemyComp->m_dropedObjectEntity = createEnemyDropObject(levelManager, enemyData, index, loadFromCheckpoint, m_currentLevelEnemiesNumber);
    }
    if(enemyComp->m_visibleShot)
    {
        if(!loadFromCheckpoint || !m_memEnemiesStateFromCheckpoint[m_currentLevelEnemiesNumber].m_dead)
        {
            enemyComp->m_visibleAmmo.resize(4);
            confAmmoEntities(enemyComp->m_visibleAmmo, CollisionTag_e::BULLET_ENEMY_CT,
                             enemyComp->m_visibleShot, enemyData.m_attackPower,
                             enemyData.m_shotVelocity, enemyData.m_damageZone);
        }
    }
    else
    {
        loadNonVisibleEnemyAmmoStuff(loadFromCheckpoint, m_currentLevelEnemiesNumber, enemyData, levelManager, *enemyComp);
    }
    loadEnemySprites(levelManager.getPictureData().getSpriteData(),
                     enemyData, numEntity, *enemyComp, levelManager.getVisibleShootDisplayData());
    MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(numEntity);
    assert(moveComp);
    moveComp->m_velocity = enemyData.m_velocity;
    moveComp->m_currentDegreeMoveDirection = 0.0f;
    moveComp->m_degreeOrientation = 0.0f;
    AudioComponent *audiocomponent = Ecsm_t::instance().getComponent<AudioComponent, Components_e::AUDIO_COMPONENT>(numEntity);
    assert(audiocomponent);
    audiocomponent->m_soundElements.reserve(3);
    audiocomponent->m_soundElements.emplace_back(soundElements[0]);
    audiocomponent->m_soundElements.emplace_back(soundElements[1]);
    audiocomponent->m_soundElements.emplace_back(soundElements[2]);
    audiocomponent->m_maxDistance = 500.0f;
    TimerComponent *timerComponent = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(numEntity);
    assert(timerComponent);
    timerComponent->m_cycleCountA = 0;
    memCheckpointEnemiesData(loadFromCheckpoint, numEntity, m_currentLevelEnemiesNumber);
    ++m_currentLevelEnemiesNumber;
    return exit;
}

//===================================================================
pairI_t getModifMoveableWallDataCheckpoint(const std::vector<std::pair<Direction_e, uint32_t>> &vectDir,
                                           uint32_t timesActionned, TriggerBehaviourType_e triggerBehaviour)
{
    if(timesActionned == 0 || (triggerBehaviour == TriggerBehaviourType_e::REVERSABLE && timesActionned % 2 == 0))
    {
        return {0, 0};
    }
    else
    {
        pairI_t posModif = {0, 0};
        for(uint32_t i = 0; i < vectDir.size(); ++i)
        {
            switch(vectDir[i].first)
            {
            case Direction_e::EAST:
                posModif.first += vectDir[i].second;
                break;
            case Direction_e::WEST:
                posModif.first -= vectDir[i].second;
                break;
            case Direction_e::NORTH:
                posModif.second -= vectDir[i].second;
                break;
            case Direction_e::SOUTH:
                posModif.second += vectDir[i].second;
                break;
            }
        }
        if(triggerBehaviour != TriggerBehaviourType_e::REVERSABLE)
        {
            posModif.first *= timesActionned;
            posModif.second *= timesActionned;
        }
        return posModif;
    }
}

//===================================================================
void MainEngine::loadNonVisibleEnemyAmmoStuff(bool loadFromCheckpoint, uint32_t currentEnemy,
                                              const EnemyData &enemyData, const LevelManager &levelManager,
                                              EnemyConfComponent &enemyComp)
{
    if(loadFromCheckpoint && m_memEnemiesStateFromCheckpoint[currentEnemy].m_dead)
    {
        return;
    }
    enemyComp.m_stdAmmo.resize(MAX_SHOTS);
    confAmmoEntities(enemyComp.m_stdAmmo, CollisionTag_e::BULLET_ENEMY_CT, enemyComp.m_visibleShot,
                     enemyData.m_attackPower);
    const MapImpactData_t &map = levelManager.getImpactDisplayData();
    MapImpactData_t::const_iterator itt = map.find(enemyData.m_impactID);
    assert(itt != map.end());
    for(uint32_t j = 0; j < enemyComp.m_stdAmmo.size(); ++j)
    {
        ShotConfComponent *shotComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(enemyComp.m_stdAmmo[j]);
        assert(shotComp);
        shotComp->m_impactEntity = confShotImpactEntity(levelManager.getPictureSpriteData(), itt->second);
    }
}

//===================================================================
void MainEngine::memCheckpointEnemiesData(bool loadFromCheckpoint, uint32_t enemyEntity, uint32_t cmpt)
{
    if(loadFromCheckpoint)
    {
        m_memEnemiesStateFromCheckpoint[cmpt].m_entityNum = enemyEntity;
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(enemyEntity);
        EnemyConfComponent *enemyComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(enemyEntity);
        assert(mapComp);
        assert(enemyComp);
        mapComp->m_absoluteMapPositionPX = m_memEnemiesStateFromCheckpoint[cmpt].m_enemyPos;
        enemyComp->m_life = m_memEnemiesStateFromCheckpoint[cmpt].m_life;
        if(m_memEnemiesStateFromCheckpoint[cmpt].m_dead)
        {
            enemyComp->m_displayMode = EnemyDisplayMode_e::DEAD;
            enemyComp->m_behaviourMode = EnemyBehaviourMode_e::DEAD;
            enemyComp->m_currentSprite = enemyComp->m_mapSpriteAssociate.find(EnemySpriteType_e::DYING)->second.second;
        }
    }
    else
    {
        m_memEnemiesStateFromCheckpoint.push_back({enemyEntity, 0, false, false, {}});
    }
}

//===================================================================
void MainEngine::loadCheckpointsEntities(const LevelManager &levelManager)
{
    const std::vector<std::pair<PairUI_t, Direction_e>> &container = levelManager.getCheckpointsData();
    uint32_t entityNum;
    for(uint32_t i = 0; i < container.size(); ++i)
    {
        if(m_currentEntitiesDelete.find(container[i].first) != m_currentEntitiesDelete.end())
        {
            continue;
        }
        entityNum = createCheckpointEntity();
        initStdCollisionCase(entityNum, container[i].first, CollisionTag_e::CHECKPOINT_CT);
        CheckpointComponent *checkComponent = Ecsm_t::instance().getComponent<CheckpointComponent, Components_e::CHECKPOINT_COMPONENT>(entityNum);
        assert(checkComponent);
        checkComponent->m_checkpointNumber = i + 1;
        checkComponent->m_direction = container[i].second;
    }
}


//===================================================================
void MainEngine::loadSecretsEntities(const LevelManager &levelManager)
{
    const std::vector<PairUI_t> &container = levelManager.getSecretsData();
    m_currentLevelSecretsNumber = container.size();
    uint32_t entityNum;
    for(uint32_t i = 0; i < container.size(); ++i)
    {
        if(m_currentEntitiesDelete.find(container[i]) != m_currentEntitiesDelete.end())
        {
            continue;
        }
        entityNum = createCheckpointEntity();
        initStdCollisionCase(entityNum, container[i], CollisionTag_e::SECRET_CT);
    }
}

//===================================================================
void MainEngine::loadLogsEntities(const LevelManager &levelManager, const std::vector<SpriteData> &vectSprite)
{
    const std::vector<LogLevelData> &container = levelManager.getLogsData();
    const std::map<std::string, LogStdData> &stdLogData = levelManager.getStdLogData();
    uint32_t entityNum;
    std::map<std::string, LogStdData>::const_iterator it;
    for(uint32_t i = 0; i < container.size(); ++i)
    {
        entityNum = createLogEntity();
        it = stdLogData.find(container[i].m_id);
        confBaseComponent(entityNum, vectSprite[it->second.m_spriteNum], container[i].m_pos,
                CollisionShape_e::CIRCLE_C, CollisionTag_e::LOG_CT);
        assert(it != stdLogData.end());
        CircleCollisionComponent *circleComp = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(entityNum);
        assert(circleComp);
        circleComp->m_ray = 10.0f;
        LogComponent *logComp = Ecsm_t::instance().getComponent<LogComponent, Components_e::LOG_COMPONENT>(entityNum);
        assert(logComp);
        logComp->m_message = treatInfoMessageEndLine(container[i].m_message);
        SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(entityNum);
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNum);
        assert(spriteComp);
        assert(mapComp);
        Level::addElementCase(*spriteComp, mapComp->m_coord, LevelCaseType_e::EMPTY_LC, entityNum);
    }
}

//===================================================================
void MainEngine::loadRevealedMap()
{
    m_graphicEngine.getMapSystem().clearRevealedMap();
    for(uint32_t i = 0; i < m_revealedMapData.size(); ++i)
    {
        std::optional<ElementRaycast> element = Level::getElementCase(m_revealedMapData[i]);
        assert(element);
        m_graphicEngine.getMapSystem().addDiscoveredEntity(element->m_numEntity, m_revealedMapData[i]);
    }
}

//===================================================================
void MainEngine::initStdCollisionCase(uint32_t entityNum, const PairUI_t &mapPos, CollisionTag_e tag)
{
    MapCoordComponent *mapComponent = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNum);
    assert(mapComponent);
    mapComponent->m_coord = mapPos;
    m_physicalEngine.addEntityToZone(entityNum, mapComponent->m_coord);
    mapComponent->m_absoluteMapPositionPX = {mapPos.first * LEVEL_TILE_SIZE_PX, mapPos.second * LEVEL_TILE_SIZE_PX};
    GeneralCollisionComponent *genCompComponent = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(entityNum);
    assert(genCompComponent);
    genCompComponent->m_shape = CollisionShape_e::RECTANGLE_C;
    genCompComponent->m_tagA = tag;
    RectangleCollisionComponent *rectComponent = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(entityNum);
    assert(rectComponent);
    rectComponent->m_size = {LEVEL_TILE_SIZE_PX, LEVEL_TILE_SIZE_PX};
}

//===================================================================
uint32_t MainEngine::createEnemyDropObject(const LevelManager &levelManager, const EnemyData &enemyData,
                                           uint32_t iterationNum, bool loadFromCheckpoint, uint32_t cmpt)
{
    std::map<std::string, StaticLevelElementData>::const_iterator itt = levelManager.getObjectData().find(enemyData.m_dropedObjectID);
    assert(itt != levelManager.getObjectData().end());
    std::optional<uint32_t> objectEntity = createStaticElementEntity(LevelStaticElementType_e::OBJECT, itt->second,
                                                      levelManager.getPictureSpriteData(), iterationNum, true);
    assert(objectEntity);
    GeneralCollisionComponent *genComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(*objectEntity);
    assert(genComp);
    genComp->m_active = false;
    if(loadFromCheckpoint && m_memEnemiesStateFromCheckpoint[cmpt].m_dead &&
            !m_memEnemiesStateFromCheckpoint[cmpt].m_objectPickedUp)
    {
        genComp->m_active = true;
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(*objectEntity);
        assert(mapComp);
        mapComp->m_absoluteMapPositionPX = m_memEnemiesStateFromCheckpoint[cmpt].m_enemyPos;
    }
    return *objectEntity;
}

//===================================================================
void MainEngine::createPlayerAmmoEntities(PlayerConfComponent &playerConf, CollisionTag_e collTag)
{
    WeaponComponent *weaponComp = Ecsm_t::instance().getComponent<WeaponComponent, Components_e::WEAPON_COMPONENT>(playerConf.m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::WEAPON)]);
    assert(weaponComp);
    for(uint32_t i = 0; i < weaponComp->m_weaponsData.size(); ++i)
    {
        if(weaponComp->m_weaponsData[i].m_attackType == AttackType_e::BULLETS)
        {
            weaponComp->m_weaponsData[i].m_segmentShootEntities = std::vector<uint32_t>();
            (*weaponComp->m_weaponsData[i].m_segmentShootEntities).resize(MAX_SHOTS);
            confAmmoEntities((*weaponComp->m_weaponsData[i].m_segmentShootEntities),
                             collTag, false, weaponComp->m_weaponsData[i].m_weaponPower);
        }
    }
}

//===================================================================
void MainEngine::confAmmoEntities(std::vector<uint32_t> &ammoEntities, CollisionTag_e collTag, bool visibleShot,
                                  uint32_t damage, float shotVelocity, std::optional<float> damageRay)
{
    for(uint32_t j = 0; j < ammoEntities.size(); ++j)
    {
        ammoEntities[j] = createAmmoEntity(collTag, visibleShot);
        ShotConfComponent *shotConfComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(ammoEntities[j]);
        assert(shotConfComp);
        shotConfComp->m_damage = damage;
        if(damageRay)
        {
            shotConfComp->m_damageCircleRayData = createDamageZoneEntity(damage, CollisionTag_e::EXPLOSION_CT, LEVEL_TILE_SIZE_PX);
        }
        if(visibleShot)
        {
            MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(ammoEntities[j]);
            assert(moveComp);
            moveComp->m_velocity = shotVelocity;
        }
    }
}

//===================================================================
uint32_t MainEngine::createAmmoEntity(CollisionTag_e collTag, bool visibleShot)
{
    uint32_t ammoNum;
    if(!visibleShot)
    {
        ammoNum = createShotEntity();
    }
    else
    {
        ammoNum = createVisibleShotEntity();
    }
    GeneralCollisionComponent *genColl = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(ammoNum);
    assert(genColl);
    genColl->m_active = false;
    genColl->m_tagA = collTag;
    genColl->m_shape = (visibleShot) ? CollisionShape_e::CIRCLE_C : CollisionShape_e::SEGMENT_C;
    if(visibleShot)
    {
        confVisibleAmmo(ammoNum);
    }
    return ammoNum;
}

//===================================================================
void MainEngine::setMenuEntries(PlayerConfComponent &playerComp, std::optional<uint32_t> cursorPos)
{
    if(playerComp.m_menuMode == MenuMode_e::CONFIRM_RESTART_FROM_LAST_CHECKPOINT && !m_memCheckpointLevelState)
    {
        playerComp.m_menuMode = MenuMode_e::BASE;
        return;
    }
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerConf);
    playerConf->m_currentCursorPos = cursorPos ? *cursorPos : 0;
    //TITLE MENU
    WriteComponent *writeComp = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::TITLE_MENU)]);
    assert(writeComp);
    m_graphicEngine.fillTitleMenuWrite(*writeComp, playerComp.m_menuMode, playerComp.m_previousMenuMode);
    //MENU ENTRIES
    WriteComponent *writeConf = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::MENU_ENTRIES)]);
    assert(writeConf);
    writeConf->m_upLeftPositionGL = MAP_MENU_DATA.at(playerComp.m_menuMode).first;
    if(writeConf->m_vectMessage.empty())
    {
        writeConf->addTextLine({{}, ""});
    }
    //SELECTED MENU ENTRY
    WriteComponent *writeCompSelect = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::MENU_SELECTED_LINE)]);
    assert(writeCompSelect);
    m_graphicEngine.fillMenuWrite(*writeConf, playerComp.m_menuMode, playerComp.m_currentCursorPos,
                                  {&playerComp, m_currentLevelSecretsNumber, m_currentLevelEnemiesNumber});
    if(playerComp.m_menuMode == MenuMode_e::LEVEL_PROLOGUE ||
            playerComp.m_menuMode == MenuMode_e::LEVEL_EPILOGUE ||
            playerComp.m_menuMode == MenuMode_e::TRANSITION_LEVEL)
    {
        return;
    }
    m_graphicEngine.confMenuSelectedLine(playerComp, *writeCompSelect, *writeConf);
    if(playerComp.m_menuMode == MenuMode_e::INPUT)
    {
        updateConfirmLoadingMenuInfo(playerComp);
    }
    else if(playerComp.m_menuMode == MenuMode_e::CONFIRM_QUIT_INPUT_FORM ||
            playerComp.m_menuMode == MenuMode_e::CONFIRM_LOADING_GAME_FORM ||
            playerComp.m_menuMode == MenuMode_e::CONFIRM_RESTART_LEVEL ||
            playerComp.m_menuMode == MenuMode_e::CONFIRM_RESTART_FROM_LAST_CHECKPOINT)
    {
        updateConfirmLoadingMenuInfo(playerComp);
        playerComp.m_currentCursorPos = 0;
    }
    else if(playerComp.m_menuMode != MenuMode_e::NEW_KEY && playerComp.m_menuMode != MenuMode_e::TITLE
            && playerComp.m_menuMode != MenuMode_e::BASE)
    {
        playerComp.m_currentCursorPos = 0;
    }
    if(playerComp.m_menuMode == MenuMode_e::DISPLAY || playerComp.m_menuMode == MenuMode_e::SOUND ||
            playerComp.m_menuMode == MenuMode_e::INPUT || playerComp.m_menuMode == MenuMode_e::LOAD_GAME ||
            playerComp.m_menuMode == MenuMode_e::LOAD_CUSTOM_LEVEL || playerComp.m_menuMode == MenuMode_e::NEW_GAME ||
            playerComp.m_menuMode == MenuMode_e::TITLE || playerComp.m_menuMode == MenuMode_e::BASE)
    {
        writeConf->m_vectMessage[0].first = writeConf->m_upLeftPositionGL.first;
    }
    else
    {
        writeConf->m_vectMessage[0].first = {};
    }
    m_graphicEngine.updateMenuCursorPosition(playerComp);
}

//===================================================================
void MainEngine::updateConfirmLoadingMenuInfo(PlayerConfComponent &playerComp)
{
    WriteComponent *writeComp = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(
        playerComp.m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::MENU_INFO_WRITE)]);
    assert(writeComp);
    writeComp->clear();
    writeComp->m_fontSpriteData.reserve(4);
    writeComp->m_vectMessage.reserve(4);
    if(playerComp.m_menuMode == MenuMode_e::INPUT)
    {
        writeComp->m_upLeftPositionGL = {-0.6f, -0.7f};
        writeComp->addTextLine({writeComp->m_upLeftPositionGL.first, ""});
        writeComp->m_vectMessage.back().second = playerComp.m_keyboardInputMenuMode ? "Keyboard\\Switch Gamepad : G Or RL" :
                                                                 "Gamepad\\Switch Keyboard : G Or RL";
    }
    else if(playerComp.m_menuMode == MenuMode_e::CONFIRM_QUIT_INPUT_FORM)
    {
        writeComp->m_upLeftPositionGL = {-0.6f, 0.3f};
        writeComp->addTextLine({{}, "Do You Want To Save Changes?"});
    }
    else if(playerComp.m_menuMode == MenuMode_e::CONFIRM_LOADING_GAME_FORM ||
            playerComp.m_menuMode == MenuMode_e::CONFIRM_RESTART_LEVEL ||
            playerComp.m_menuMode == MenuMode_e::CONFIRM_RESTART_FROM_LAST_CHECKPOINT ||
            playerComp.m_menuMode == MenuMode_e::CONFIRM_QUIT_GAME)
    {
        if(!playerComp.m_firstMenu)
        {
            writeComp->m_upLeftPositionGL = {-0.8f, 0.5f};
            writeComp->addTextLine({{}, "All Your Progress Until Last Save"});
            writeComp->addTextLine({{}, "Will Be Lost"});
        }
        else
        {
            writeComp->m_upLeftPositionGL = {-0.8f, 0.5f};
        }
        if(playerComp.m_menuMode == MenuMode_e::CONFIRM_LOADING_GAME_FORM)
        {
            if(playerComp.m_previousMenuMode == MenuMode_e::NEW_GAME && checkSavedGameExists(playerComp.m_currentCursorPos + 1))
            {
                writeComp->addTextLine({{}, "Previous File Will Be Erased"});
            }
            if(!writeComp->m_vectMessage.empty())
            {
                writeComp->addTextLine({{}, "Continue Anyway?"});
            }
            //TITLE MENU CASE
            else
            {
                writeComp->m_upLeftPositionGL = {-0.3f, 0.3f};
                if(playerComp.m_previousMenuMode == MenuMode_e::NEW_GAME)
                {
                    writeComp->addTextLine({{}, "Begin New Game?"});
                }
                else if(playerComp.m_previousMenuMode == MenuMode_e::LOAD_GAME)
                {
                    writeComp->addTextLine({{}, "Load Game?"});
                }
                else if(playerComp.m_previousMenuMode == MenuMode_e::LOAD_CUSTOM_LEVEL)
                {
                    writeComp->addTextLine({{}, "Load Custom Game?"});
                }
            }
        }
        else if(playerComp.m_menuMode == MenuMode_e::CONFIRM_QUIT_GAME)
        {
            writeComp->addTextLine({{}, "Do You Really Want To Quit The Game?"});
        }
    }
    m_graphicEngine.confWriteComponent(*writeComp);
}

//===================================================================
void MainEngine::updateWriteComp(WriteComponent &writeComp)
{
    m_graphicEngine.confWriteComponent(writeComp);
}

//===================================================================
void MainEngine::updateStringWriteEntitiesInputMenu(bool keyboardInputMenuMode, bool defaultInput)
{
    m_graphicEngine.updateStringWriteEntitiesInputMenu(keyboardInputMenuMode, defaultInput);
}

//===================================================================
void MainEngine::confGlobalSettings(const SettingsData &settingsData)
{
    //AUDIO
    if(settingsData.m_musicVolume > 100)
    {
        m_audioEngine.updateMusicVolume(100);
        m_graphicEngine.updateMusicVolumeBar(100);
    }
    else
    {
        if(settingsData.m_musicVolume)
        {
            m_audioEngine.updateMusicVolume(*settingsData.m_musicVolume);
            m_graphicEngine.updateMusicVolumeBar(*settingsData.m_musicVolume);
        }
    }
    if(settingsData.m_effectsVolume > 100)
    {
        m_audioEngine.updateEffectsVolume(100, false);
        m_graphicEngine.updateEffectsVolumeBar(100);
    }
    else
    {
        if(settingsData.m_musicVolume)
        {
            m_audioEngine.updateEffectsVolume(*settingsData.m_effectsVolume, false);
            m_graphicEngine.updateEffectsVolumeBar(*settingsData.m_effectsVolume);
        }
    }
    //DISPLAY
    if(settingsData.m_fullscreen && *settingsData.m_fullscreen)
    {
        m_graphicEngine.toogleFullScreen();
    }
    if(settingsData.m_resolutionWidth && settingsData.m_resolutionHeight)
    {
        m_graphicEngine.setSizeResolution({*settingsData.m_resolutionWidth, *settingsData.m_resolutionHeight});
    }
    //INPUT
    //KEYBOARD
    if(settingsData.m_arrayKeyboard)
    {
        m_physicalEngine.setKeyboardKey(*settingsData.m_arrayKeyboard);
    }
    //GAMEPAD
    if(settingsData.m_arrayGamepad)
    {
        m_physicalEngine.setGamepadKey(*settingsData.m_arrayGamepad);
    }
    if(settingsData.m_turnSensitivity)
    {
        updateTurnSensitivity(*settingsData.m_turnSensitivity);
    }
}

//===================================================================
void MainEngine::validDisplayMenu()
{
    m_graphicEngine.validDisplayMenu();
    m_refGame->saveDisplaySettings(m_graphicEngine.getResolutions()[m_graphicEngine.getCurrentResolutionNum()].first,
            m_graphicEngine.fullscreenMode());
}

//===================================================================
void MainEngine::reinitPlayerGear()
{
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerConf);
    WeaponComponent *weaponConf = Ecsm_t::instance().getComponent<WeaponComponent, Components_e::WEAPON_COMPONENT>(playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::MENU_ENTRIES)]);
    //if first launch return
    if(!weaponConf)
    {
        return;
    }
    playerConf->m_card.clear();
    playerConf->m_checkpointReached = {};
    playerConf->m_currentCheckpoint = {};
    playerConf->m_enemiesKilled = {};
    playerConf->m_life = 100;
    playerConf->m_secretsFound = {};
    for(uint32_t i = 0; i < weaponConf->m_weaponsData.size(); ++i)
    {
        weaponConf->m_weaponsData[i].m_posses = m_vectMemWeaponsDefault[i].first;
        weaponConf->m_weaponsData[i].m_ammunationsCount = m_vectMemWeaponsDefault[i].second;
    }
}

//===================================================================
void MainEngine::setInfoDataWrite(std::string_view message)
{
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerConf);
    playerConf->m_infoWriteData = {true, message.data()};
}

//===================================================================
void MainEngine::playTriggerSound()
{
    AudioComponent *audioComp = Ecsm_t::instance().getComponent<AudioComponent, Components_e::AUDIO_COMPONENT>(m_playerEntity);
    assert(audioComp);
    audioComp->m_soundElements[2]->m_toPlay = true;
    m_audioEngine.getSoundSystem()->execSystem();
}

//===================================================================
void MainEngine::confMenuSelectedLine()
{
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    WriteComponent *writeMenuComp = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::MENU_ENTRIES)]);
    WriteComponent *writeMenuSelectedComp = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::MENU_SELECTED_LINE)]);
    assert(playerConf);
    assert(writeMenuComp);
    assert(writeMenuSelectedComp);
    m_graphicEngine.confMenuSelectedLine(*playerConf, *writeMenuSelectedComp, *writeMenuComp);
}

//===================================================================
void MainEngine::setPlayerDeparture(const PairUI_t &pos, Direction_e dir)
{
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerEntity);
    MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(m_playerEntity);
    assert(mapComp);
    assert(moveComp);
    mapComp->m_absoluteMapPositionPX = getCenteredAbsolutePosition(pos);
    moveComp->m_degreeOrientation = getDegreeAngleFromDirection(dir);
}

//===================================================================
void MainEngine::saveAudioSettings()
{
    m_refGame->saveAudioSettings(getMusicVolume(), getEffectsVolume());
}

//===================================================================
void MainEngine::saveInputSettings(const std::map<ControlKey_e, GamepadInputState> &gamepadArray,
                                   const std::map<ControlKey_e, MouseKeyboardInputState> &keyboardArray)
{
    m_refGame->saveInputSettings(gamepadArray, keyboardArray);
}

//===================================================================
void MainEngine::saveTurnSensitivitySettings()
{
    m_refGame->saveTurnSensitivitySettings(m_physicalEngine.getTurnSensitivity());
}

//===================================================================
bool MainEngine::loadSavedGame(uint32_t saveNum, LevelState_e levelMode)
{
    m_memCustomLevelLoadedData = 0;
    m_currentLevelState = levelMode;
    m_currentSave = saveNum;
    if(m_currentLevelState == LevelState_e::NEW_GAME)
    {
        m_levelToLoad = {1, false};
        return true;
    }
    std::optional<MemLevelLoadedData> savedData = m_refGame->loadSavedGame(saveNum);
    if(!savedData)
    {
        return false;
    }
    m_memPlayerConfBeginLevel = *savedData->m_playerConfBeginLevel;
    if(savedData->m_checkpointLevelData)
    {
        PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
        assert(playerConf);
        playerConf->m_currentCheckpoint = {savedData->m_checkpointLevelData->m_checkpointNum, savedData->m_checkpointLevelData->m_direction};
    }
    assert(!m_memPlayerConfBeginLevel.m_ammunationsCount.empty());
    if(savedData->m_playerConfCheckpoint)
    {
        m_memPlayerConfCheckpoint = *savedData->m_playerConfCheckpoint;
    }
    if(m_currentLevelState == LevelState_e::RESTART_LEVEL || m_currentLevelState == LevelState_e::RESTART_FROM_CHECKPOINT)
    {
        m_levelToLoad = {m_currentLevel, false};
    }
    else if(m_currentLevelState == LevelState_e::LOAD_GAME)
    {
        m_levelToLoad = {savedData->m_levelNum, false};
    }
    m_playerMemGear = true;
    if(savedData->m_checkpointLevelData)
    {
        loadCheckpointSavedGame(*savedData->m_checkpointLevelData);
    }
    if(m_memCustomLevelLoadedData)
    {
        loadRevealedMap();
    }
    return true;
}

//===================================================================
bool MainEngine::loadCustomLevelGame(LevelState_e levelMode)
{
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerConf);
    m_currentLevelState = levelMode;
    m_levelToLoad = {playerConf->m_levelToLoad, true};
    m_currentLevel = playerConf->m_levelToLoad;
    if(!m_memCustomLevelLoadedData)
    {
        m_memCustomLevelLoadedData = std::make_unique<MemCustomLevelLoadedData>();
    }
    return true;
}

//===================================================================
void MainEngine::loadCheckpointSavedGame(const MemCheckpointElementsState &checkpointData, bool loadFromSameLevel)
{
    m_memCheckpointLevelState = {m_levelToLoad->first, checkpointData.m_checkpointNum, checkpointData.m_secretsNumber,
                                 checkpointData.m_enemiesKilled, checkpointData.m_direction,
                                 checkpointData.m_checkpointPos};
    if(!loadFromSameLevel)
    {
        m_currentEntitiesDelete = checkpointData.m_staticElementDeleted;
    }
    m_memEnemiesStateFromCheckpoint = checkpointData.m_enemiesData;
    m_memMoveableWallCheckpointData = checkpointData.m_moveableWallData;
    m_memTriggerWallMoveableWallCheckpointData = checkpointData.m_triggerWallMoveableWallData;
    m_revealedMapData = checkpointData.m_revealedMapData;
    m_memCheckpointData = {checkpointData.m_checkpointNum, checkpointData.m_secretsNumber,
                           checkpointData.m_enemiesKilled, checkpointData.m_checkpointPos,
                           checkpointData.m_direction, m_memEnemiesStateFromCheckpoint,
                           m_memMoveableWallCheckpointData, m_memTriggerWallMoveableWallCheckpointData,
                           m_memStaticEntitiesDeletedFromCheckpoint, checkpointData.m_revealedMapData, checkpointData.m_card};
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerConf);
    playerConf->m_card = m_memCheckpointData->m_card;
}

//===================================================================
bool MainEngine::checkSavedGameExists(uint32_t saveNum)const
{
    return m_refGame->checkSavedGameExists(saveNum);
}

//===================================================================
void MainEngine::clearCheckpointData()
{
    m_memStaticEntitiesDeletedFromCheckpoint.clear();
    m_currentEntitiesDelete.clear();
    m_memMoveableWallCheckpointData.clear();
    m_memTriggerWallMoveableWallCheckpointData.clear();
    m_memCheckpointLevelState = std::nullopt;
    m_memEnemiesStateFromCheckpoint.clear();
    m_revealedMapData.clear();
    if(m_memCustomLevelLoadedData)
    {
        m_memCustomLevelLoadedData->m_checkpointLevelData = std::nullopt;
    }
    m_memCheckpointData.reset();
}

//===================================================================
bool MainEngine::isLoadFromLevelBegin(LevelState_e levelState)const
{
    if(levelState == LevelState_e::RESTART_FROM_CHECKPOINT ||
            (m_memCheckpointLevelState && (levelState == LevelState_e::GAME_OVER || levelState == LevelState_e::LOAD_GAME)))
    {
        return false;
    }
    return true;
}

//===================================================================
void MainEngine::createPlayerVisibleShotEntity(WeaponComponent &weaponConf)
{
    for(uint32_t i = 0; i < weaponConf.m_weaponsData.size(); ++i)
    {
        if(weaponConf.m_weaponsData[i].m_attackType == AttackType_e::VISIBLE_SHOTS)
        {
            weaponConf.m_weaponsData[i].m_visibleShootEntities = std::vector<uint32_t>();
            (*weaponConf.m_weaponsData[i].m_visibleShootEntities).resize(1);
            for(uint32_t j = 0; j < weaponConf.m_weaponsData[i].m_visibleShootEntities->size(); ++j)
            {
                (*weaponConf.m_weaponsData[i].m_visibleShootEntities)[j] = createAmmoEntity(CollisionTag_e::BULLET_PLAYER_CT, true);
            }
        }
    }
}

//===================================================================
uint32_t MainEngine::confShotImpactEntity(const std::vector<SpriteData> &vectSpriteData,
                                          const PairImpactData_t &shootDisplayData)
{
    uint32_t impactEntity = createShotImpactEntity();
    GeneralCollisionComponent *genComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(impactEntity);
    CircleCollisionComponent *circleComp = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(impactEntity);
    assert(genComp);
    assert(circleComp);
    circleComp->m_ray = 2.0f;
    genComp->m_active = false;
    genComp->m_tagA = CollisionTag_e::IMPACT_CT;
    genComp->m_tagB = CollisionTag_e::IMPACT_CT;
    genComp->m_shape = CollisionShape_e::CIRCLE_C;
    loadShotImpactSprite(vectSpriteData, shootDisplayData, impactEntity);
    return impactEntity;
}

//===================================================================
void MainEngine::loadEnemySprites(const std::vector<SpriteData> &vectSprite, const EnemyData &enemiesData, uint32_t numEntity,
                                  EnemyConfComponent &enemyComp, const MapVisibleShotData_t &visibleShot)
{
    MemSpriteDataComponent *memSpriteComp = Ecsm_t::instance().getComponent<MemSpriteDataComponent, Components_e::MEM_SPRITE_DATA_COMPONENT>(numEntity);
    assert(memSpriteComp);
    insertEnemySpriteFromType(vectSprite, enemyComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                              enemiesData.m_staticFrontSprites, EnemySpriteType_e::STATIC_FRONT);
    insertEnemySpriteFromType(vectSprite, enemyComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                              enemiesData.m_staticFrontLeftSprites, EnemySpriteType_e::STATIC_FRONT_LEFT);
    insertEnemySpriteFromType(vectSprite, enemyComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                              enemiesData.m_staticFrontRightSprites, EnemySpriteType_e::STATIC_FRONT_RIGHT);
    insertEnemySpriteFromType(vectSprite, enemyComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                              enemiesData.m_staticBackSprites, EnemySpriteType_e::STATIC_BACK);
    insertEnemySpriteFromType(vectSprite, enemyComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                              enemiesData.m_staticBackLeftSprites, EnemySpriteType_e::STATIC_BACK_LEFT);
    insertEnemySpriteFromType(vectSprite, enemyComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                              enemiesData.m_staticBackRightSprites, EnemySpriteType_e::STATIC_BACK_RIGHT);
    insertEnemySpriteFromType(vectSprite, enemyComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                              enemiesData.m_staticLeftSprites, EnemySpriteType_e::STATIC_LEFT);
    insertEnemySpriteFromType(vectSprite, enemyComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                              enemiesData.m_staticRightSprites, EnemySpriteType_e::STATIC_RIGHT);
    insertEnemySpriteFromType(vectSprite, enemyComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                              enemiesData.m_attackSprites, EnemySpriteType_e::ATTACK);
    insertEnemySpriteFromType(vectSprite, enemyComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                              enemiesData.m_dyingSprites, EnemySpriteType_e::DYING);
    insertEnemySpriteFromType(vectSprite, enemyComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                              enemiesData.m_touched, EnemySpriteType_e::TOUCHED);
    if(enemyComp.m_visibleShot)
    {
        loadVisibleShotData(vectSprite, enemyComp.m_visibleAmmo, enemiesData.m_visibleShootID, visibleShot);
    }
}

//===================================================================
void insertEnemySpriteFromType(const std::vector<SpriteData> &vectSprite,
                               mapEnemySprite_t &mapSpriteAssociate,
                               std::vector<SpriteData const *> &vectSpriteData,
                               const std::vector<uint16_t> &enemyMemArray,
                               EnemySpriteType_e type)
{
    //second pair {first pos last pos}
    mapSpriteAssociate.insert({type, {vectSpriteData.size(), vectSpriteData.size() +
                                      enemyMemArray.size() - 1}});
    for(uint32_t j = 0; j < enemyMemArray.size(); ++j)
    {
        vectSpriteData.emplace_back(&vectSprite[enemyMemArray[j]]);
    }
}

//TMP ENEMY DATA FIRST ENEMY
//===================================================================
void MainEngine::loadPlayerSprites(const std::vector<SpriteData> &vectSprite, const PlayerData &playerData, uint32_t numEntity,
                                   PlayerConfComponent &playerComp)
{
    MemSpriteDataComponent *memSpriteComp = Ecsm_t::instance().getComponent<MemSpriteDataComponent, Components_e::MEM_SPRITE_DATA_COMPONENT>(numEntity);
    assert(memSpriteComp);
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(numEntity);
    spriteComp->m_spriteData = &vectSprite[playerData.m_stayRightSprites[0]];
    insertPlayerSpriteFromType(vectSprite, playerComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                               playerData.m_runRightSprites, PlayerSpriteElementType_e::RUN_RIGHT);
    insertPlayerSpriteFromType(vectSprite, playerComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                               playerData.m_shootUpLookRightSprites, PlayerSpriteElementType_e::SHOOT_UP_LOOK_RIGHT);
    insertPlayerSpriteFromType(vectSprite, playerComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                               playerData.m_shootRightSprites, PlayerSpriteElementType_e::SHOOT_RIGHT);
    insertPlayerSpriteFromType(vectSprite, playerComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                               playerData.m_stayRightSprites, PlayerSpriteElementType_e::STAY_RIGHT);
    insertPlayerSpriteFromType(vectSprite, playerComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                               playerData.m_jumpRightSprites, PlayerSpriteElementType_e::JUMP_RIGHT);
    insertPlayerSpriteFromType(vectSprite, playerComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                               playerData.m_damageRightSprites, PlayerSpriteElementType_e::DAMAGE_RIGHT);
    insertPlayerSpriteFromType(vectSprite, playerComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                               playerData.m_runLeftSprites, PlayerSpriteElementType_e::RUN_LEFT);
    insertPlayerSpriteFromType(vectSprite, playerComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                               playerData.m_shootUpLookLeftSprites, PlayerSpriteElementType_e::SHOOT_UP_LOOK_LEFT);
    insertPlayerSpriteFromType(vectSprite, playerComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                               playerData.m_shootLeftSprites, PlayerSpriteElementType_e::SHOOT_LEFT);
    insertPlayerSpriteFromType(vectSprite, playerComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                               playerData.m_jumpLeftSprites, PlayerSpriteElementType_e::JUMP_LEFT);
    insertPlayerSpriteFromType(vectSprite, playerComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                               playerData.m_stayLeftSprites, PlayerSpriteElementType_e::STAY_LEFT);
    insertPlayerSpriteFromType(vectSprite, playerComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                               playerData.m_damageLeftSprites, PlayerSpriteElementType_e::DAMAGE_LEFT);
    insertPlayerSpriteFromType(vectSprite, playerComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                               playerData.m_shootRunLeft, PlayerSpriteElementType_e::SHOOT_RUN_LEFT);
    insertPlayerSpriteFromType(vectSprite, playerComp.m_mapSpriteAssociate, memSpriteComp->m_vectSpriteData,
                               playerData.m_shootRunRight, PlayerSpriteElementType_e::SHOOT_RUN_RIGHT);
    // if(playerComp.m_visibleShot)
    // {
    //     loadVisibleShotData(vectSprite, enemyComp.m_visibleAmmo, enemiesData.m_visibleShootID, visibleShot);
    // }
}

//===================================================================
void insertPlayerSpriteFromType(const std::vector<SpriteData> &vectSprite,
                               MapPlayerSprite_t &mapSpriteAssociate,
                               std::vector<SpriteData const *> &vectSpriteData,
                               const std::vector<uint16_t> &playerMemArray,
                               PlayerSpriteElementType_e type)
{
    //second pair {first pos last pos}
    mapSpriteAssociate.insert({type, {vectSpriteData.size(), vectSpriteData.size() +
                                                                 playerMemArray.size() - 1}});
    for(uint32_t j = 0; j < playerMemArray.size(); ++j)
    {
        vectSpriteData.emplace_back(&vectSprite[playerMemArray[j]]);
    }
}

//===================================================================
void MainEngine::loadVisibleShotData(const std::vector<SpriteData> &vectSprite, const std::vector<uint32_t> &visibleAmmo,
                                     const std::string &visibleShootID, const MapVisibleShotData_t &visibleShot)
{
    for(uint32_t k = 0; k < visibleAmmo.size(); ++k)
    {
        SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(visibleAmmo[k]);
        MemSpriteDataComponent *memSpriteComp = Ecsm_t::instance().getComponent<MemSpriteDataComponent, Components_e::MEM_SPRITE_DATA_COMPONENT>(visibleAmmo[k]);
        AudioComponent *audioComp = Ecsm_t::instance().getComponent<AudioComponent, Components_e::AUDIO_COMPONENT>(visibleAmmo[k]);
        ShotConfComponent *shotComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(visibleAmmo[k]);
        assert(spriteComp);
        assert(memSpriteComp);
        assert(audioComp);
        assert(shotComp);
        MapVisibleShotData_t::const_iterator it = visibleShot.find(visibleShootID);
        assert(it != visibleShot.end());
        if(!m_memSoundElements.m_visibleShots)
        {
            m_memSoundElements.m_visibleShots = std::map<std::string, SoundElement>();
        }
        if(m_memSoundElements.m_visibleShots->find(it->second.first) == m_memSoundElements.m_visibleShots->end())
        {
            m_memSoundElements.m_visibleShots->insert({it->second.first, loadSound(it->second.first)});
        }
        audioComp->m_soundElements.push_back(m_memSoundElements.m_visibleShots->at(it->second.first));
        memSpriteComp->m_vectSpriteData.reserve(it->second.second.size());
        for(uint32_t l = 0; l < it->second.second.size(); ++l)
        {
            memSpriteComp->m_vectSpriteData.emplace_back(&vectSprite[it->second.second[l].m_numSprite]);
        }
        spriteComp->m_spriteData = memSpriteComp->m_vectSpriteData[0];
        //OOOK
        shotComp->m_ejectExplosionRay = LEVEL_HALF_TILE_SIZE_PX;
    }
}

//===================================================================
void MainEngine::confVisibleAmmo(uint32_t ammoEntity)
{
    PairFloat_t pairSpriteSize = {0.2f, 0.3f};
    float collisionRay = pairSpriteSize.first * LEVEL_HALF_TILE_SIZE_PX;
    CircleCollisionComponent *circleComp = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(ammoEntity);
    MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(ammoEntity);
    assert(circleComp);
    assert(moveComp);
    circleComp->m_ray = collisionRay;
    moveComp->m_velocity = 5.0f;
}

//===================================================================
uint32_t MainEngine::createDisplayTeleportEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    vect[Components_e::SPRITE_TEXTURE_COMPONENT] = 1;
    vect[Components_e::MEM_SPRITE_DATA_COMPONENT] = 1;
    vect[Components_e::TIMER_COMPONENT] = 1;
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    vect[Components_e::AUDIO_COMPONENT] = 1;
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createWeaponEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    vect[Components_e::SPRITE_TEXTURE_COMPONENT] = 1;
    vect[Components_e::MEM_SPRITE_DATA_COMPONENT] = 1;
    vect[Components_e::MEM_POSITIONS_VERTEX_COMPONENT] = 1;
    vect[Components_e::TIMER_COMPONENT] = 1;
    vect[Components_e::WEAPON_COMPONENT] = 1;
    vect[Components_e::AUDIO_COMPONENT] = 1;
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createWallEntity(bool multiSprite, bool moveable)
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    vect[Components_e::SPRITE_TEXTURE_COMPONENT] = 1;
    vect[Components_e::MAP_COORD_COMPONENT] = 1;
    vect[Components_e::RECTANGLE_COLLISION_COMPONENT] = 1;
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    if(multiSprite)
    {
        vect[Components_e::MEM_SPRITE_DATA_COMPONENT] = 1;
        vect[Components_e::TIMER_COMPONENT] = 1;
    }
    if(moveable)
    {
        vect[Components_e::MOVEABLE_COMPONENT] = 1;
    }
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createDoorEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    vect[Components_e::SPRITE_TEXTURE_COMPONENT] = 1;
    vect[Components_e::MAP_COORD_COMPONENT] = 1;
    vect[Components_e::RECTANGLE_COLLISION_COMPONENT] = 1;
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    vect[Components_e::TIMER_COMPONENT] = 1;
    vect[Components_e::AUDIO_COMPONENT] = 1;
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createEnemyEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    vect[Components_e::SPRITE_TEXTURE_COMPONENT] = 1;
    vect[Components_e::MAP_COORD_COMPONENT] = 1;
    vect[Components_e::RECTANGLE_COLLISION_COMPONENT] = 1;
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    vect[Components_e::MOVEABLE_COMPONENT] = 1;
    vect[Components_e::MEM_SPRITE_DATA_COMPONENT] = 1;
    vect[Components_e::ENEMY_CONF_COMPONENT] = 1;
    vect[Components_e::TIMER_COMPONENT] = 1;
    vect[Components_e::AUDIO_COMPONENT] = 1;
    vect[Components_e::GRAVITY_COMPONENT] = 1;
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createShotEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::SEGMENT_COLLISION_COMPONENT] = 1;
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    vect[Components_e::SHOT_CONF_COMPONENT] = 1;
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createTriggerEntity(bool visible)
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    vect[Components_e::MAP_COORD_COMPONENT] = 1;
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    vect[Components_e::CIRCLE_COLLISION_COMPONENT] = 1;
    if(visible)
    {
        vect[Components_e::SPRITE_TEXTURE_COMPONENT] = 1;

    }
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createVisibleShotEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::CIRCLE_COLLISION_COMPONENT] = 1;
    vect[Components_e::AUDIO_COMPONENT] = 1;
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    vect[Components_e::SPRITE_TEXTURE_COMPONENT] = 1;
    vect[Components_e::MOVEABLE_COMPONENT] = 1;
    vect[Components_e::MAP_COORD_COMPONENT] = 1;
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    vect[Components_e::TIMER_COMPONENT] = 1;
    vect[Components_e::SHOT_CONF_COMPONENT] = 1;
    vect[Components_e::MEM_SPRITE_DATA_COMPONENT] = 1;
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createShotImpactEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::MAP_COORD_COMPONENT] = 1;
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    vect[Components_e::SPRITE_TEXTURE_COMPONENT] = 1;
    vect[Components_e::TIMER_COMPONENT] = 1;
    vect[Components_e::MEM_SPRITE_DATA_COMPONENT] = 1;
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    vect[Components_e::CIRCLE_COLLISION_COMPONENT] = 1;
    vect[Components_e::MOVEABLE_COMPONENT] = 1;
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createWriteEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    vect[Components_e::WRITE_COMPONENT] = 1;
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createSimpleSpriteEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    vect[Components_e::SPRITE_TEXTURE_COMPONENT] = 1;
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createStaticEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    vect[Components_e::SPRITE_TEXTURE_COMPONENT] = 1;
    vect[Components_e::MAP_COORD_COMPONENT] = 1;
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    vect[Components_e::CIRCLE_COLLISION_COMPONENT] = 1;
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createObjectEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    vect[Components_e::SPRITE_TEXTURE_COMPONENT] = 1;
    vect[Components_e::MAP_COORD_COMPONENT] = 1;
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    vect[Components_e::CIRCLE_COLLISION_COMPONENT] = 1;
    vect[Components_e::OBJECT_CONF_COMPONENT] = 1;
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
void MainEngine::confBaseComponent(uint32_t entityNum, const SpriteData &memSpriteData,
                                   const std::optional<PairUI_t> &coordLevel, CollisionShape_e collisionShape,
                                   CollisionTag_e tag, std::optional<const PairFloat_t> inGameSpriteSize)
{
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(entityNum);
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNum);
    assert(spriteComp);
    assert(mapComp);
    spriteComp->m_spriteData = &memSpriteData;
    if(coordLevel)
    {
        mapComp->m_coord = *coordLevel;
            mapComp->m_absoluteMapPositionPX = getAbsolutePosition(*coordLevel);
        m_physicalEngine.addEntityToZone(entityNum, mapComp->m_coord);
    }
    GeneralCollisionComponent *tagComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(entityNum);
    assert(tagComp);
    tagComp->m_shape = collisionShape;
    if(collisionShape == CollisionShape_e::RECTANGLE_C)
    {
        RectangleCollisionComponent *rectComp = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(entityNum);
        assert(rectComp);
        if(tag == CollisionTag_e::WALL_CT)
        {
            rectComp->m_size = {LEVEL_TILE_SIZE_PX, LEVEL_TILE_SIZE_PX};
        }
        else
        {
            assert(inGameSpriteSize);
            rectComp->m_size = {inGameSpriteSize->first * LEVEL_TILE_SIZE_PX, inGameSpriteSize->second * LEVEL_TILE_SIZE_PX};
        }
    }
    tagComp->m_tagA = tag;
}

//===================================================================
void MainEngine::loadPlayerEntity(const LevelManager &levelManager)
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::POSITION_VERTEX_COMPONENT] = 1;
    vect[Components_e::MAP_COORD_COMPONENT] = 1;
    vect[Components_e::MOVEABLE_COMPONENT] = 1;
    vect[Components_e::SPRITE_TEXTURE_COMPONENT] = 1;
    vect[Components_e::MEM_SPRITE_DATA_COMPONENT] = 1;
    vect[Components_e::MEM_POSITIONS_VERTEX_COMPONENT] = 1;
    vect[Components_e::INPUT_COMPONENT] = 1;
    vect[Components_e::CIRCLE_COLLISION_COMPONENT] = 1;
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    vect[Components_e::PLAYER_CONF_COMPONENT] = 1;
    vect[Components_e::TIMER_COMPONENT] = 1;
    vect[Components_e::AUDIO_COMPONENT] = 1;
    vect[Components_e::GRAVITY_COMPONENT] = 1;    
    uint32_t entityNum = Ecsm_t::instance().addEntity(vect);
    confPlayerEntity(levelManager, entityNum, levelManager.getLevel(),
                     loadWeaponsEntity(levelManager), loadDisplayTeleportEntity(levelManager));
    //notify player entity number
    m_graphicEngine.memPlayerDatas(entityNum);
    m_graphicEngine.getMapSystem().memPlayerEntity(m_playerEntity);
    m_physicalEngine.memPlayerEntity(entityNum);
    m_audioEngine.memPlayerEntity(entityNum);

}

//===================================================================
void MainEngine::confPlayerEntity(const LevelManager &levelManager, uint32_t entityNum, const Level &level,
                                  uint32_t numWeaponEntity, uint32_t numDisplayTeleportEntity)
{
    m_playerEntity = entityNum;
    const std::vector<SpriteData> &vectSpriteData =
            levelManager.getPictureData().getSpriteData();
    PositionVertexComponent *pos = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(entityNum);
    MapCoordComponent *map = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNum);
    MoveableComponent *move = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(entityNum);
    CircleCollisionComponent *circleColl = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(entityNum);
    GeneralCollisionComponent *tagColl = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(entityNum);
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(entityNum);
    assert(pos);
    assert(map);
    assert(move);
    assert(circleColl);
    assert(tagColl);
    assert(playerConf);
    const PlayerData &playerData = levelManager.getPlayerData();
    loadPlayerSprites(levelManager.getPictureData().getSpriteData(), playerData, entityNum, *playerConf);
    playerConf->m_life = 100;
    playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::WEAPON)] = numWeaponEntity;
    playerConf->m_levelToLoad = m_currentLevel;
    playerConf->m_memEntityAssociated = entityNum;
    playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::DISPLAY_TELEPORT)] = numDisplayTeleportEntity;
    AudioComponent *audioComp = Ecsm_t::instance().getComponent<AudioComponent, Components_e::AUDIO_COMPONENT>(entityNum);
    assert(audioComp);
    audioComp->m_soundElements.reserve(3);
    audioComp->m_soundElements.emplace_back(loadSound(levelManager.getPickObjectSoundFile()));
    audioComp->m_soundElements.emplace_back(loadSound(levelManager.getPlayerDeathSoundFile()));
    audioComp->m_soundElements.emplace_back(loadSound(levelManager.getTriggerSoundFile()));
    WeaponComponent *weaponConf = Ecsm_t::instance().getComponent<WeaponComponent, Components_e::WEAPON_COMPONENT>(playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::WEAPON)]);
    assert(weaponConf);
    createPlayerAmmoEntities(*playerConf, CollisionTag_e::BULLET_PLAYER_CT);
    createPlayerVisibleShotEntity(*weaponConf);
    confPlayerVisibleShotsSprite(vectSpriteData, levelManager.getVisibleShootDisplayData(), *weaponConf);
    createPlayerImpactEntities(vectSpriteData, *weaponConf, levelManager.getImpactDisplayData());
    map->m_coord = level.getPlayerDeparture();
    Direction_e playerDir = level.getPlayerDepartureDirection();
    move->m_degreeOrientation = getDegreeAngleFromDirection(playerDir);
    move->m_velocity = 2.5f;
    map->m_absoluteMapPositionPX = getCenteredAbsolutePosition(map->m_coord);
    circleColl->m_ray = PLAYER_RAY;
    updatePlayerArrow(*move, *pos);
    tagColl->m_tagA = CollisionTag_e::PLAYER_CT;
    tagColl->m_shape = CollisionShape_e::CIRCLE_C;
    //set standard weapon sprite
    StaticDisplaySystem *staticDisplay = Ecsm_t::instance().getSystem<StaticDisplaySystem>(static_cast<uint32_t>(Systems_e::STATIC_DISPLAY_SYSTEM));
    assert(staticDisplay);
    staticDisplay->setWeaponSprite(numWeaponEntity, weaponConf->m_weaponsData[weaponConf->m_currentWeapon].m_memPosSprite.first);
    confWriteEntities();
    confMenuEntities();
    confLifeAmmoPannelEntities();
    confWeaponsPreviewEntities();
    confActionEntity();
    confMapDetectShapeEntity(map->m_absoluteMapPositionPX);
    for(uint32_t i = 0; i < weaponConf->m_weaponsData.size(); ++i)
    {
        if(weaponConf->m_weaponsData[i].m_attackType == AttackType_e::MELEE)
        {
            playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::HIT_MELEE)] = createDamageZoneEntity(weaponConf->m_weaponsData[i].m_weaponPower,
                                                                   CollisionTag_e::HIT_PLAYER_CT, 10.0f, levelManager.getHitSoundFile());
            break;
        }
    }
}

//===================================================================
void MainEngine::confActionEntity()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    vect[Components_e::MAP_COORD_COMPONENT] = 1;
    vect[Components_e::CIRCLE_COLLISION_COMPONENT] = 1;
    uint32_t entityNum = Ecsm_t::instance().addEntity(vect);
    GeneralCollisionComponent *genCollComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(entityNum);
    CircleCollisionComponent *circleColl = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(entityNum);
    assert(genCollComp);
    assert(circleColl);
    genCollComp->m_active = false;
    genCollComp->m_shape = CollisionShape_e::CIRCLE_C;
    genCollComp->m_tagA = CollisionTag_e::PLAYER_ACTION_CT;
    circleColl->m_ray = 15.0f;
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerConf);
    playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::ACTION)] = entityNum;
}

//===================================================================
void MainEngine::confMapDetectShapeEntity(const PairFloat_t &playerPos)
{
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerConf);
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    vect[Components_e::RECTANGLE_COLLISION_COMPONENT] = 1;
    vect[Components_e::MAP_COORD_COMPONENT] = 1;
    playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::MAP_DETECT_SHAPE)] = Ecsm_t::instance().addEntity(vect);
    RectangleCollisionComponent *rectColl = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(
        playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::MAP_DETECT_SHAPE)]);
    assert(rectColl);

    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::MAP_DETECT_SHAPE)]);
    assert(mapComp);
    GeneralCollisionComponent *genComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(
        playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::MAP_DETECT_SHAPE)]);
    assert(genComp);
    mapComp->m_absoluteMapPositionPX = {playerPos.first - DETECT_RECT_SHAPE_HALF_SIZE,
                                        playerPos.second - DETECT_RECT_SHAPE_HALF_SIZE};
    rectColl->m_size = {DETECT_RECT_SHAPE_SIZE, DETECT_RECT_SHAPE_SIZE};
    genComp->m_shape = CollisionShape_e::RECTANGLE_C;
    genComp->m_tagA = CollisionTag_e::DETECT_MAP_CT;
}

//===================================================================
uint32_t MainEngine::createMeleeAttackEntity(bool sound)
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> vect;
    vect.fill(0);
    vect[Components_e::GENERAL_COLLISION_COMPONENT] = 1;
    vect[Components_e::CIRCLE_COLLISION_COMPONENT] = 1;
    vect[Components_e::MAP_COORD_COMPONENT] = 1;
    vect[Components_e::SHOT_CONF_COMPONENT] = 1;
    if(sound)
    {
        vect[Components_e::AUDIO_COMPONENT] = 1;
    }
    return Ecsm_t::instance().addEntity(vect);
}

//===================================================================
uint32_t MainEngine::createDamageZoneEntity(uint32_t damage, CollisionTag_e tag,
                                            float ray, const std::string &soundFile)
{
    uint32_t entityNum = createMeleeAttackEntity(!soundFile.empty());
    GeneralCollisionComponent *genCollComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(entityNum);
    CircleCollisionComponent *circleColl = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(entityNum);
    ShotConfComponent *shotComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(entityNum);
    assert(genCollComp);
    assert(circleColl);
    assert(shotComp);
    if(!soundFile.empty())
    {
        AudioComponent *audioComp = Ecsm_t::instance().getComponent<AudioComponent, Components_e::AUDIO_COMPONENT>(entityNum);
        assert(audioComp);
        if(!m_memSoundElements.m_damageZone)
        {
            m_memSoundElements.m_damageZone = loadSound(soundFile);
        }
        audioComp->m_soundElements.push_back(m_memSoundElements.m_damageZone);
    }
    genCollComp->m_active = false;
    genCollComp->m_shape = CollisionShape_e::CIRCLE_C;
    genCollComp->m_tagA = tag;
    circleColl->m_ray = ray;
    shotComp->m_damage = damage;
    return entityNum;
}

//===================================================================
void MainEngine::loadShotImpactSprite(const std::vector<SpriteData> &vectSpriteData,
                                      const PairImpactData_t &shootDisplayData,
                                      uint32_t impactEntity)
{
    MemSpriteDataComponent *memComp = Ecsm_t::instance().getComponent<MemSpriteDataComponent, Components_e::MEM_SPRITE_DATA_COMPONENT>(impactEntity);
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(impactEntity);
    assert(memComp);
    assert(spriteComp);
    for(uint32_t l = 0; l < shootDisplayData.first.size(); ++l)
    {
        memComp->m_vectSpriteData.emplace_back(
                    &vectSpriteData[shootDisplayData.first[l].m_numSprite]);
    }
    memComp->m_vectSpriteData.emplace_back(
                &vectSpriteData[shootDisplayData.second.m_numSprite]);
    spriteComp->m_spriteData = memComp->m_vectSpriteData[0];
}

//===================================================================
void MainEngine::confPlayerVisibleShotsSprite(const std::vector<SpriteData> &vectSpriteData,
                                              const MapVisibleShotData_t &shootDisplayData,
                                              WeaponComponent &weaponComp)
{
    for(uint32_t i = 0; i < weaponComp.m_weaponsData.size(); ++i)
    {
        if(weaponComp.m_weaponsData[i].m_visibleShootEntities)
        {
            confAmmoEntities(*weaponComp.m_weaponsData[i].m_visibleShootEntities, CollisionTag_e::BULLET_PLAYER_CT,
                             true, weaponComp.m_weaponsData[i].m_weaponPower, weaponComp.m_weaponsData[i].m_shotVelocity,
                             weaponComp.m_weaponsData[i].m_damageRay);
            loadVisibleShotData(vectSpriteData, *weaponComp.m_weaponsData[i].m_visibleShootEntities,
                                weaponComp.m_weaponsData[i].m_visibleShotID, shootDisplayData);
        }
    }
}

//===================================================================
void MainEngine::confWriteEntities()
{
    uint32_t numAmmoWrite = createWriteEntity(), numInfoWrite = createWriteEntity(), numLifeWrite = createWriteEntity(),
            numMenuWrite = createWriteEntity(), numTitleMenuWrite = createWriteEntity(),
            numInputModeMenuWrite = createWriteEntity(), numMenuSelectedLineWrite = createWriteEntity();
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);

    WeaponComponent *weaponConf = Ecsm_t::instance().getComponent<WeaponComponent, Components_e::WEAPON_COMPONENT>(playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::WEAPON)]);
    //INFO
    WriteComponent *writeConf = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(numInfoWrite);
    assert(playerConf);
    assert(weaponConf);
    assert(writeConf);
    writeConf->m_upLeftPositionGL = {-0.3f, 0.7f};
    writeConf->addTextLine({{}, ""});
    writeConf->m_fontSize = STD_FONT_SIZE;
    //AMMO
    WriteComponent *writeConfAmmo = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(numAmmoWrite);
    assert(writeConfAmmo);
    writeConfAmmo->m_upLeftPositionGL = {-0.8f, -0.9f};
    writeConfAmmo->m_fontSize = STD_FONT_SIZE;
    writeConfAmmo->addTextLine({writeConfAmmo->m_upLeftPositionGL.first, ""});
    m_graphicEngine.updateAmmoCount(*writeConfAmmo, *weaponConf);
    //LIFE
    WriteComponent *writeConfLife = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(numLifeWrite);
    assert(writeConfLife);
    writeConfLife->m_upLeftPositionGL = {-0.8f, -0.8f};
    writeConfLife->m_fontSize = STD_FONT_SIZE;
    writeConfLife->addTextLine({writeConfLife->m_upLeftPositionGL.first, ""});
    m_graphicEngine.updatePlayerLife(*writeConfLife, *playerConf);
    //MENU
    WriteComponent *writeConfMenu = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(numMenuWrite);
    assert(writeConfMenu);
    writeConfMenu->m_fontSize = MENU_FONT_SIZE;
    //TITLE MENU
    WriteComponent *writeConfTitleMenu = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(numTitleMenuWrite);
    assert(writeConfTitleMenu);
    writeConfTitleMenu->m_upLeftPositionGL = {-0.3f, 0.9f};
    writeConfTitleMenu->addTextLine({{}, ""});
    writeConfTitleMenu->m_fontSize = MENU_FONT_SIZE;
    //INPUT MENU MODE
    WriteComponent *writeConfInputMenu = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(numInputModeMenuWrite);
    assert(writeConfInputMenu);
    writeConfInputMenu->m_upLeftPositionGL = {-0.6f, -0.7f};
    writeConfInputMenu->m_fontSize = MENU_FONT_SIZE;
    confWriteEntitiesDisplayMenu();
    confWriteEntitiesInputMenu();
    playerConf->m_menuMode = MenuMode_e::BASE;
    playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::MENU_ENTRIES)] = numMenuWrite;
    playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::TITLE_MENU)] = numTitleMenuWrite;
    playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::MENU_INFO_WRITE)] = numInputModeMenuWrite;
    playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::MENU_SELECTED_LINE)] = numMenuSelectedLineWrite;
    WriteComponent *writeComp = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::TITLE_MENU)]);
    assert(writeComp);
    writeComp->addTextLine({-0.3f, ""});
    WriteComponent *writeCompTitle = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::TITLE_MENU)]);
    assert(writeCompTitle);
    writeCompTitle->m_fontSpriteData.emplace_back(VectSpriteDataRef_t{});
    //MENU SELECTED LINE
    WriteComponent *writeCompSelect = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(numMenuSelectedLineWrite);
    assert(writeCompSelect);
    writeCompSelect->m_fontSpriteData.emplace_back(VectSpriteDataRef_t{});
    writeCompSelect->m_fontSize = MENU_FONT_SIZE;
    writeCompSelect->m_fontType = Font_e::SELECTED;

    setMenuEntries(*playerConf);
    playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::AMMO_WRITE)] = numAmmoWrite;
    playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::LIFE_WRITE)] = numLifeWrite;
    playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::NUM_INFO_WRITE)] = numInfoWrite;
}

//===================================================================
void MainEngine::confWriteEntitiesDisplayMenu()
{
    uint32_t numMenuResolutionWrite = createWriteEntity(), numMenuFullscreenWrite = createWriteEntity();
    //Resolution
    WriteComponent *writeConfA = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(numMenuResolutionWrite);
    assert(writeConfA);
    writeConfA->m_upLeftPositionGL.first = MAP_MENU_DATA.at(MenuMode_e::DISPLAY).first.first + 1.0f;
    writeConfA->m_upLeftPositionGL.second = MAP_MENU_DATA.at(MenuMode_e::DISPLAY).first.second;
    writeConfA->m_fontSize = MENU_FONT_SIZE;
    if(writeConfA->m_vectMessage.empty())
    {
        writeConfA->addTextLine({writeConfA->m_upLeftPositionGL.first, ""});
    }
    //OOOOK default resolution
    writeConfA->m_vectMessage[0].second = m_graphicEngine.getResolutions()[0].second;
    m_graphicEngine.confWriteComponent(*writeConfA);
    //Fullscreen
    WriteComponent *writeConfB = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(numMenuFullscreenWrite);
    assert(writeConfB);
    writeConfB->m_upLeftPositionGL = {writeConfA->m_upLeftPositionGL.first, writeConfA->m_upLeftPositionGL.second - MENU_FONT_SIZE};
    writeConfB->m_fontSize = MENU_FONT_SIZE;
    if(writeConfB->m_vectMessage.empty())
    {
        writeConfB->addTextLine({writeConfB->m_upLeftPositionGL.first, ""});
    }
    writeConfB->m_vectMessage[0].second = "";
    m_graphicEngine.confWriteComponent(*writeConfB);
    Ecsm_t::instance().getSystem<StaticDisplaySystem>(static_cast<uint32_t>(Systems_e::STATIC_DISPLAY_SYSTEM))->memDisplayMenuEntities(numMenuResolutionWrite, numMenuFullscreenWrite);
}

//===================================================================
void MainEngine::confWriteEntitiesInputMenu()
{
    ArrayControlKey_t memKeyboardEntities, memGamepadEntities;
    PairFloat_t currentUpLeftPos = {MAP_MENU_DATA.at(MenuMode_e::INPUT).first.first + 1.0f, MAP_MENU_DATA.at(MenuMode_e::INPUT).first.second};
    for(uint32_t i = 0; i < memKeyboardEntities.size(); ++i)
    {
        //KEYBOARD
        memKeyboardEntities[i] = createWriteEntity();
        WriteComponent *writeConf = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(memKeyboardEntities[i]);
        assert(writeConf);
        writeConf->m_upLeftPositionGL = currentUpLeftPos;
        writeConf->m_fontSize = MENU_FONT_SIZE;
        //GAMEPAD
        memGamepadEntities[i] = createWriteEntity();
        WriteComponent *writeConfGamepad = Ecsm_t::instance().getComponent<WriteComponent, Components_e::WRITE_COMPONENT>(memGamepadEntities[i]);
        assert(writeConfGamepad);
        writeConfGamepad->m_upLeftPositionGL = currentUpLeftPos;
        writeConfGamepad->m_fontSize = MENU_FONT_SIZE;
        currentUpLeftPos.second -= MENU_FONT_SIZE;
    }
    Ecsm_t::instance().getSystem<StaticDisplaySystem>(static_cast<uint32_t>(Systems_e::STATIC_DISPLAY_SYSTEM))->memInputMenuEntities(memKeyboardEntities, memGamepadEntities);
    updateStringWriteEntitiesInputMenu(true);
}

//===================================================================
void MainEngine::confMenuEntities()
{
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerConf);
    assert(m_memBackgroundGenericMenu);
    playerConf->m_menuMode = MenuMode_e::BASE;
    confMenuEntity(PlayerEntities_e::MENU_TITLE_BACKGROUND);
    confMenuEntity(PlayerEntities_e::MENU_GENERIC_BACKGROUND);
    confMenuEntity(PlayerEntities_e::MENU_LEFT_BACKGROUND);
    confMenuEntity(PlayerEntities_e::MENU_RIGHT_LEFT_BACKGROUND);
}

//===================================================================
void MainEngine::confMenuEntity(PlayerEntities_e entityType)
{
    uint32_t backgroundEntity = createSimpleSpriteEntity();
    PositionVertexComponent *posVertex = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(backgroundEntity);
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(backgroundEntity);
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(posVertex);
    assert(spriteComp);
    assert(playerConf);
    playerConf->m_vectEntities[static_cast<uint32_t>(entityType)] = backgroundEntity;
    if(entityType == PlayerEntities_e::MENU_GENERIC_BACKGROUND)
    {
        spriteComp->m_spriteData = m_memBackgroundGenericMenu;
    }
    else if(entityType == PlayerEntities_e::MENU_TITLE_BACKGROUND)
    {
        spriteComp->m_spriteData = m_memBackgroundTitleMenu;
    }
    else if(entityType == PlayerEntities_e::MENU_LEFT_BACKGROUND)
    {
        spriteComp->m_spriteData = m_memBackgroundLeftMenu;
    }
    else if(entityType == PlayerEntities_e::MENU_RIGHT_LEFT_BACKGROUND)
    {
        spriteComp->m_spriteData = m_memBackgroundRightLeftMenu;
    }
    else
    {
        assert(false);
    }
    posVertex->m_vertex.resize(4);
    posVertex->m_vertex[0] = {-1.0f, 1.0f};
    posVertex->m_vertex[1] = {1.0f, 1.0f};
    posVertex->m_vertex[2] = {1.0f, -1.0f};
    posVertex->m_vertex[3] = {-1.0f, -1.0f};
}

//===================================================================
void MainEngine::confLifeAmmoPannelEntities()
{
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerConf);
    //PANNEL
    uint32_t lifeAmmoPannelEntity = createSimpleSpriteEntity();
    PositionVertexComponent *posCursor = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(lifeAmmoPannelEntity);
    SpriteTextureComponent *spriteCursor = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(lifeAmmoPannelEntity);
    assert(posCursor);
    assert(spriteCursor);
    playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::LIFE_AMMO_PANNEL)] = lifeAmmoPannelEntity;
    spriteCursor->m_spriteData = m_memPannel;
    posCursor->m_vertex.reserve(4);
    float up = -0.78f, down = -0.97f, left = -0.97f, right = -0.625f;
    posCursor->m_vertex.insert(posCursor->m_vertex.end(), {{left, up}, {right, up}, {right, down},{left, down}});
    //LIFE
    uint32_t lifeIconEntity = createSimpleSpriteEntity();
    PositionVertexComponent *posCursorA = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(lifeIconEntity);
    SpriteTextureComponent *spriteCursorA = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(lifeIconEntity);
    assert(posCursorA);
    assert(spriteCursorA);
    playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::LIFE_ICON)] = lifeIconEntity;
    spriteCursorA->m_spriteData = m_memLifeIcon;
    posCursorA->m_vertex.reserve(4);
    up = -0.8f, down = -0.87f, left = -0.95f, right = -0.9f;
    posCursorA->m_vertex.insert(posCursorA->m_vertex.end(), {{left, up}, {right, up}, {right, down},{left, down}});
    //AMMO
    uint32_t ammoIconEntity = createSimpleSpriteEntity();
    PositionVertexComponent *posCursorB = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(ammoIconEntity);
    SpriteTextureComponent *spriteCursorB = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(ammoIconEntity);
    assert(posCursorB);
    assert(spriteCursorB);
    playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::AMMO_ICON)] = ammoIconEntity;
    spriteCursorB->m_spriteData = m_memAmmoIcon;
    posCursorB->m_vertex.reserve(4);
    up = -0.9f, down = -0.95f, left = -0.95f, right = -0.9f;
    posCursorB->m_vertex.insert(posCursorB->m_vertex.end(), {{left, up}, {right, up}, {right, down},{left, down}});

}

//===================================================================
void MainEngine::confWeaponsPreviewEntities()
{
    PlayerConfComponent *playerConf = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerConf);
    playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::CURSOR_WEAPON_PREVIEW)] = createSimpleSpriteEntity();
    SpriteTextureComponent *spriteCursor = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(
        playerConf->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::CURSOR_WEAPON_PREVIEW)]);
    assert(spriteCursor);
    spriteCursor->m_spriteData = m_memPannel;
    for(uint32_t i = 0; i < playerConf->m_vectPossessedWeaponsPreviewEntities.size(); ++i)
    {
        playerConf->m_vectPossessedWeaponsPreviewEntities[i] = createSimpleSpriteEntity();
        SpriteTextureComponent *spriteCursorA = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(
            playerConf->m_vectPossessedWeaponsPreviewEntities[i]);
        switch(i)
        {
        case 0:
            spriteCursorA->m_spriteData = m_memPreviewFistIcon;
            break;
        case 1:
            spriteCursorA->m_spriteData = m_memPreviewGunIcon;
            break;
        case 2:
            spriteCursorA->m_spriteData = m_memPreviewShotgunIcon;
            break;
        case 3:
            spriteCursorA->m_spriteData = m_memPreviewPlasmaRifleIcon;
            break;
        case 4:
            spriteCursorA->m_spriteData = m_memPreviewMachineGunIcon;
            break;
        case 5:
            spriteCursorA->m_spriteData = m_memPreviewBazookaIcon;
            break;
        default:
            assert(false);
            break;
        }
    }
}

//===================================================================
bool MainEngine::loadStaticElementEntities(const LevelManager &levelManager)
{
    const std::vector<SpriteData> &vectSprite = levelManager.getPictureData().getSpriteData();
    loadStaticElementGroup(vectSprite, levelManager.getGroundData(), LevelStaticElementType_e::GROUND);
    loadStaticElementGroup(vectSprite, levelManager.getCeilingData(), LevelStaticElementType_e::CEILING);
    loadStaticElementGroup(vectSprite, levelManager.getObjectData(), LevelStaticElementType_e::OBJECT);
    loadStaticElementGroup(vectSprite, levelManager.getTeleportData(), LevelStaticElementType_e::TELEPORT);
    return loadExitElement(levelManager, levelManager.getExitElementData());
}

//===================================================================
void MainEngine::loadStaticSpriteEntities(const LevelManager &levelManager)
{
    uint16_t pannelSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getPannelSpriteName()),
            lifeIconSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getLifeIconSpriteName()),
            ammoIconSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getAmmoIconSpriteName()),
            fistIconSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getFistIconSpriteName()),
            gunIconSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getGunIconSpriteName()),
            shotgunIconSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getShotgunIconSpriteName()),
            plasmaRifleIconSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getPlasmaRifleIconSpriteName()),
            machineGunIconSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getMachineGunIconSpriteName()),
            GenericBackgroundSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getGenericMenuSpriteName()),
            TitleBackgroundSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getTitleMenuSpriteName()),
            leftBackgroundSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getLeftMenuSpriteName()),
            rightLeftBackgroundSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getRightLeftMenuSpriteName()),
            bazookaIconSpriteId = *levelManager.getPictureData().getIdentifier(levelManager.getBazookaIconSpriteName());

    m_memBackgroundGenericMenu = &levelManager.getPictureData().getSpriteData()[GenericBackgroundSpriteId];
    m_memBackgroundTitleMenu = &levelManager.getPictureData().getSpriteData()[TitleBackgroundSpriteId];
    m_memBackgroundLeftMenu = &levelManager.getPictureData().getSpriteData()[leftBackgroundSpriteId];
    m_memBackgroundRightLeftMenu = &levelManager.getPictureData().getSpriteData()[rightLeftBackgroundSpriteId];
    m_memPannel = &levelManager.getPictureData().getSpriteData()[pannelSpriteId];
    m_memLifeIcon = &levelManager.getPictureData().getSpriteData()[lifeIconSpriteId];
    m_memAmmoIcon = &levelManager.getPictureData().getSpriteData()[ammoIconSpriteId];

    m_memPreviewFistIcon = &levelManager.getPictureData().getSpriteData()[fistIconSpriteId];
    m_memPreviewGunIcon = &levelManager.getPictureData().getSpriteData()[gunIconSpriteId];
    m_memPreviewShotgunIcon = &levelManager.getPictureData().getSpriteData()[shotgunIconSpriteId];
    m_memPreviewPlasmaRifleIcon = &levelManager.getPictureData().getSpriteData()[plasmaRifleIconSpriteId];
    m_memPreviewMachineGunIcon = &levelManager.getPictureData().getSpriteData()[machineGunIconSpriteId];
    m_memPreviewBazookaIcon = &levelManager.getPictureData().getSpriteData()[bazookaIconSpriteId];
}

//===================================================================
SoundElement MainEngine::loadSound(const std::string &file)
{
    std::optional<ALuint> num = m_audioEngine.loadSoundEffectFromFile(file);
    if(!num)
    {
        std::cout << "loading " << file << " failed\n";
    }
    assert(num);
    return {m_audioEngine.getSoundSystem()->createSource(*num), *num, false};
}

//===================================================================
uint32_t MainEngine::loadDisplayTeleportEntity(const LevelManager &levelManager)
{
    uint32_t numEntity = createDisplayTeleportEntity();
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(numEntity);
    MemSpriteDataComponent *memSpriteComp = Ecsm_t::instance().getComponent<MemSpriteDataComponent, Components_e::MEM_SPRITE_DATA_COMPONENT>(numEntity);
    PositionVertexComponent *posCursor = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(numEntity);
    GeneralCollisionComponent *genColl = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(numEntity);
    assert(spriteComp);
    assert(memSpriteComp);
    assert(posCursor);
    assert(genColl);
    genColl->m_tagA = CollisionTag_e::GHOST_CT;
    genColl->m_tagB = CollisionTag_e::TELEPORT_ANIM_CT;
    genColl->m_active = false;
    posCursor->m_vertex.reserve(4);
    float up = 0.75f, down = -0.75f, left = -0.75f, right = 0.75f;
    posCursor->m_vertex.insert(posCursor->m_vertex.end(), {{left, up}, {right, up}, {right, down},{left, down}});
    const std::vector<SpriteData> &vectSprite = levelManager.getPictureData().getSpriteData();
    const std::vector<MemSpriteData> &visibleTeleportData = levelManager.getVisibleTeleportData();
    memSpriteComp->m_vectSpriteData.reserve(visibleTeleportData.size());
    for(uint32_t j = 0; j < visibleTeleportData.size(); ++j)
    {
        memSpriteComp->m_current = 0;
        memSpriteComp->m_vectSpriteData.emplace_back(&vectSprite[visibleTeleportData[j].m_numSprite]);
    }
    spriteComp->m_spriteData = memSpriteComp->m_vectSpriteData[0];
    m_graphicEngine.getVisionSystem().memTeleportAnimEntity(numEntity);
    AudioComponent *audioComp = Ecsm_t::instance().getComponent<AudioComponent, Components_e::AUDIO_COMPONENT>(numEntity);
    assert(audioComp);
    if(!m_memSoundElements.m_teleports)
    {
        m_memSoundElements.m_teleports = loadSound(levelManager.getTeleportSoundFile());
    }
    audioComp->m_soundElements.push_back(*m_memSoundElements.m_teleports);
    return numEntity;
}

//===================================================================
bool MainEngine::loadExitElement(const LevelManager &levelManager,
                                 const StaticLevelElementData &exit)
{
    if(exit.m_TileGamePosition.empty())
    {
        return false;
    }
    const SpriteData &memSpriteData = levelManager.getPictureData().
            getSpriteData()[exit.m_numSprite];
    uint32_t entityNum = createStaticEntity();
    confBaseComponent(entityNum, memSpriteData, exit.m_TileGamePosition[0],
            CollisionShape_e::CIRCLE_C, CollisionTag_e::EXIT_CT);
    CircleCollisionComponent *circleColl = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(entityNum);
    assert(circleColl);
    circleColl->m_ray = 5.0f;
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(entityNum);
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNum);
    assert(spriteComp);
    assert(mapComp);
    Level::addElementCase(*spriteComp, mapComp->m_coord, LevelCaseType_e::EMPTY_LC, entityNum);
    return true;
}

//===================================================================
void MainEngine::loadStaticElementGroup(const std::vector<SpriteData> &vectSpriteData,
                                        const std::map<std::string, StaticLevelElementData> &staticData,
                                        LevelStaticElementType_e elementType)
{
    std::map<std::string, StaticLevelElementData>::const_iterator it = staticData.begin();
    for(; it != staticData.end(); ++it)
    {
        for(uint32_t j = 0; j < it->second.m_TileGamePosition.size(); ++j)
        {
            createStaticElementEntity(elementType, it->second, vectSpriteData, j, false);
        }
    }
}

//===================================================================
std::optional<uint32_t> MainEngine::createStaticElementEntity(LevelStaticElementType_e elementType, const StaticLevelElementData &staticElementData,
                                                              const std::vector<SpriteData> &vectSpriteData, uint32_t iterationNum, bool enemyDrop)
{
    CollisionTag_e tag;
    uint32_t entityNum;
    const SpriteData &memSpriteData = vectSpriteData[staticElementData.m_numSprite];
    if(!enemyDrop && m_currentEntitiesDelete.find(staticElementData.m_TileGamePosition[iterationNum]) !=
            m_currentEntitiesDelete.end())
    {
        return {};
    }
    if(elementType == LevelStaticElementType_e::OBJECT)
    {
        tag = CollisionTag_e::OBJECT_CT;
        entityNum = confObjectEntity(staticElementData);
    }
    else
    {
        if(staticElementData.m_traversable)
        {
            tag = CollisionTag_e::GHOST_CT;
        }
        else
        {
            tag = CollisionTag_e::STATIC_SET_CT;
        }
        entityNum = createStaticEntity();
    }
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNum);
    assert(mapComp);
    //Enemy dropable object case (will be activated and positionned at enemy death)
    if(iterationNum >= staticElementData.m_TileGamePosition.size())
    {
        confBaseComponent(entityNum, memSpriteData, {}, CollisionShape_e::CIRCLE_C, tag);
    }
    else
    {
        confBaseComponent(entityNum, memSpriteData, staticElementData.m_TileGamePosition[iterationNum], CollisionShape_e::CIRCLE_C, tag);
    }
    CircleCollisionComponent *circleComp = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(entityNum);
    assert(circleComp);
    circleComp->m_ray = staticElementData.m_inGameSpriteSize.first * LEVEL_THIRD_TILE_SIZE_PX;
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(entityNum);
    assert(spriteComp);
    Level::addElementCase(*spriteComp, mapComp->m_coord, LevelCaseType_e::EMPTY_LC, entityNum);
    m_physicalEngine.addEntityToZone(entityNum, mapComp->m_coord);
    return entityNum;
}

//===================================================================
uint32_t MainEngine::confObjectEntity(const StaticLevelElementData &objectData)
{
    uint32_t entityNum = createObjectEntity();
    ObjectConfComponent *objComp = Ecsm_t::instance().getComponent<ObjectConfComponent, Components_e::OBJECT_CONF_COMPONENT>(entityNum);
    assert(objComp);
    objComp->m_type = objectData.m_type;
    if(objComp->m_type == ObjectType_e::AMMO_WEAPON || objComp->m_type == ObjectType_e::WEAPON ||
            objComp->m_type == ObjectType_e::HEAL)
    {
        objComp->m_containing = objectData.m_containing;
        objComp->m_weaponID = objectData.m_weaponID;
    }
    else if(objComp->m_type == ObjectType_e::CARD)
    {
        objComp->m_cardName = objectData.m_cardName;
        objComp->m_cardID = objectData.m_cardID;
    }
    return entityNum;
}


//===================================================================
void MainEngine::confColorBackgroundComponents(uint32_t entity, const GroundCeilingData &groundData, bool ground)
{
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(entity);
    assert(posComp);
    posComp->m_vertex.reserve(4);
    if(ground)
    {
        posComp->m_vertex.emplace_back(-1.0f, 0.0f);
        posComp->m_vertex.emplace_back(1.0f, 0.0f);
        posComp->m_vertex.emplace_back(1.0f, -1.0f);
        posComp->m_vertex.emplace_back(-1.0f, -1.0f);
    }
    else
    {
        posComp->m_vertex.emplace_back(-1.0f, 1.0f);
        posComp->m_vertex.emplace_back(1.0f, 1.0f);
        posComp->m_vertex.emplace_back(1.0f, 0.0f);
        posComp->m_vertex.emplace_back(-1.0f, 0.0f);
    }
    ColorVertexComponent *colorComp = Ecsm_t::instance().getComponent<ColorVertexComponent, Components_e::COLOR_VERTEX_COMPONENT>(entity);
    assert(colorComp);
    colorComp->m_vertex.reserve(4);
    colorComp->m_vertex.emplace_back(std::get<0>(groundData.m_color[0]), std::get<1>(groundData.m_color[0]),
            std::get<2>(groundData.m_color[0]), 1.0);
    colorComp->m_vertex.emplace_back(std::get<0>(groundData.m_color[1]), std::get<1>(groundData.m_color[1]),
            std::get<2>(groundData.m_color[1]), 1.0);
    colorComp->m_vertex.emplace_back(std::get<0>(groundData.m_color[2]), std::get<1>(groundData.m_color[2]),
            std::get<2>(groundData.m_color[2]), 1.0);
    colorComp->m_vertex.emplace_back(std::get<0>(groundData.m_color[3]), std::get<1>(groundData.m_color[3]),
            std::get<2>(groundData.m_color[3]), 1.0);
}

//===================================================================
void MainEngine::confGroundSimpleTextBackgroundComponents(uint32_t entity, const GroundCeilingData &groundData,
                                                          const std::vector<SpriteData> &vectSprite)
{
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(entity);
    assert(posComp);
    posComp->m_vertex.reserve(6);
    posComp->m_vertex.emplace_back(-1.0f, 0.0f);
    posComp->m_vertex.emplace_back(1.0f, 0.0f);
    posComp->m_vertex.emplace_back(1.0f, -1.0f);
    posComp->m_vertex.emplace_back(-1.0f, -1.0f);
    posComp->m_vertex.emplace_back(3.0f, 0.0f);
    posComp->m_vertex.emplace_back(3.0f, -1.0f);
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(entity);
    assert(spriteComp);
    assert(vectSprite.size() >= groundData.m_spriteSimpleTextNum);
    spriteComp->m_spriteData = &vectSprite[groundData.m_spriteSimpleTextNum];
}

//===================================================================
void MainEngine::confCeilingSimpleTextBackgroundComponents(uint32_t entity, const GroundCeilingData &groundData,
                                                           const std::vector<SpriteData> &vectSprite)
{
    PositionVertexComponent *posComp = Ecsm_t::instance().getComponent<PositionVertexComponent, Components_e::POSITION_VERTEX_COMPONENT>(entity);
    assert(posComp);
    posComp->m_vertex.reserve(6);
    posComp->m_vertex.emplace_back(-1.0f, 1.0f);
    posComp->m_vertex.emplace_back(1.0f, 1.0f);
    posComp->m_vertex.emplace_back(1.0f, 0.0f);
    posComp->m_vertex.emplace_back(-1.0f, 0.0f);
    posComp->m_vertex.emplace_back(3.0f, 1.0f);
    posComp->m_vertex.emplace_back(3.0f, 0.0f);
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(entity);
    assert(spriteComp);
    assert(vectSprite.size() >= groundData.m_spriteSimpleTextNum);
    spriteComp->m_spriteData = &vectSprite[groundData.m_spriteSimpleTextNum];
}

//===================================================================
void MainEngine::confTiledTextBackgroundComponents(uint32_t entity, const GroundCeilingData &backgroundData,
                                                   const std::vector<SpriteData> &vectSprite)
{
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(entity);
    assert(spriteComp);
    assert(vectSprite.size() >= backgroundData.m_spriteTiledTextNum);
    spriteComp->m_spriteData = &vectSprite[backgroundData.m_spriteTiledTextNum];
}

//===================================================================
void MainEngine::linkSystemsToGraphicEngine()
{
    ColorDisplaySystem * color = Ecsm_t::instance().getSystem<ColorDisplaySystem>(static_cast<uint32_t>(Systems_e::COLOR_DISPLAY_SYSTEM));
    MapDisplaySystem *map = Ecsm_t::instance().getSystem<MapDisplaySystem>(static_cast<uint32_t>(Systems_e::MAP_DISPLAY_SYSTEM));
    VisionSystem *vision = Ecsm_t::instance().getSystem<VisionSystem>(static_cast<uint32_t>(Systems_e::VISION_SYSTEM));
    StaticDisplaySystem *staticDisplay = Ecsm_t::instance().getSystem<StaticDisplaySystem>(static_cast<uint32_t>(Systems_e::STATIC_DISPLAY_SYSTEM));
    assert(color);
    assert(map);
    assert(vision);
    assert(staticDisplay);
    staticDisplay->linkMainEngine(this);
    m_graphicEngine.linkSystems(color, map, vision, staticDisplay);
}

//===================================================================
void MainEngine::linkSystemsToPhysicalEngine()
{   
    InputSystem * input = Ecsm_t::instance().getSystem<InputSystem>(static_cast<uint32_t>(Systems_e::INPUT_SYSTEM));
    CollisionSystem * coll = Ecsm_t::instance().getSystem<CollisionSystem>(static_cast<uint32_t>(Systems_e::COLLISION_SYSTEM));
    IASystem *iaSystem = Ecsm_t::instance().getSystem<IASystem>(static_cast<uint32_t>(Systems_e::IA_SYSTEM));
    GravitySystem *gravSystem = Ecsm_t::instance().getSystem<GravitySystem>(static_cast<uint32_t>(Systems_e::GRAVITY_SYSTEM));
    assert(input);
    assert(coll);
    assert(iaSystem);
    assert(gravSystem);
    input->linkMainEngine(this);
    input->init(m_graphicEngine.getGLWindow());
    iaSystem->linkMainEngine(this);
    coll->linkMainEngine(this);
    m_physicalEngine.linkSystems(input, coll, iaSystem, gravSystem);
}

//===================================================================
void MainEngine::linkSystemsToSoundEngine()
{
    SoundSystem * soundSystem = Ecsm_t::instance().getSystem<SoundSystem>(static_cast<uint32_t>(Systems_e::SOUND_SYSTEM));
    assert(soundSystem);
    m_audioEngine.linkSystem(soundSystem);
}

//===================================================================
float getDegreeAngleFromDirection(Direction_e direction)
{
    switch(direction)
    {
    case Direction_e::NORTH:
        return 90.0f;
    case Direction_e::EAST:
        return 0.0f;
    case Direction_e::SOUTH:
        return 270.0f;
    case Direction_e::WEST:
        return 180.0f;
    }
    return 0.0f;
}

//===================================================================
float randFloat(float min, float max)
{
    return std::fmod(static_cast<float>(std::rand()), max) + min;
}
