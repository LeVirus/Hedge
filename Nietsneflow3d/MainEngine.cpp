#include "MainEngine.hpp"
#include "constants.hpp"
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/ColorVertexComponent.hpp>
#include <ECS/Components/MapCoordComponent.hpp>
#include <ECS/Components/SpriteTextureComponent.hpp>
#include <ECS/Components/StaticElementComponent.hpp>
#include <ECS/Components/MoveableComponent.hpp>
#include <ECS/Components/CircleCollisionComponent.hpp>
#include <ECS/Components/RectangleCollisionComponent.hpp>
#include <ECS/Components/GeneralCollisionComponent.hpp>
#include <ECS/Components/MemSpriteDataComponent.hpp>
#include <ECS/Components/VisionComponent.hpp>
#include <ECS/Components/DoorComponent.hpp>
#include <ECS/Components/PlayerConfComponent.hpp>
#include <ECS/Components/MemPositionsVertexComponents.hpp>
#include <ECS/Components/SegmentCollisionComponent.hpp>
#include <ECS/Components/WriteComponent.hpp>
#include <ECS/Components/TimerComponent.hpp>
#include <ECS/Systems/ColorDisplaySystem.hpp>
#include <ECS/Systems/MapDisplaySystem.hpp>
#include <ECS/Systems/CollisionSystem.hpp>
#include <ECS/Systems/FirstPersonDisplaySystem.hpp>
#include <ECS/Systems/VisionSystem.hpp>
#include <ECS/Systems/DoorSystem.hpp>
#include <ECS/Systems/StaticDisplaySystem.hpp>
#include <ECS/Systems/IASystem.hpp>
#include <LevelManager.hpp>
#include <cassert>

//===================================================================
void MainEngine::init()
{
    m_ecsManager.init();
    linkSystemsToGraphicEngine();
    linkSystemsToPhysicalEngine();
}

//===================================================================
void MainEngine::mainLoop()
{
    m_graphicEngine.getMapSystem().confLevelData();
    do
    {
        m_physicalEngine.runIteration(m_gamePaused);
        m_graphicEngine.runIteration(m_gamePaused);
    }while(!m_graphicEngine.windowShouldClose());
}

//===================================================================
void MainEngine::playerShoot(PlayerConfComponent *playerComp, const pairFloat_t &point,
                       float degreeAngle)
{
    if(playerComp->m_currentWeapon == WeaponsType_e::GUN
            || playerComp->m_currentWeapon == WeaponsType_e::SHOTGUN)
    {
        if(!playerComp->m_shootEntities[0])
        {
            playerComp->m_shootEntities[0] = createShotEntity();
        }
    }
    else
    {
        return;
    }
    confBullet(CollisionTag_e::BULLET_PLAYER_CT, *playerComp->m_shootEntities[0],
               point, degreeAngle);
    uint32_t currentWeapon = static_cast<uint32_t>(playerComp->m_currentWeapon);
    assert(playerComp->m_ammunationsCount[currentWeapon] > 0);
    --playerComp->m_ammunationsCount[currentWeapon];
    updateDisplayAmmoCount(playerComp);
}

//===================================================================
void MainEngine::confBullet(CollisionTag_e collTag, uint32_t bulletEntity,
                            const pairFloat_t &point, float degreeAngle)
{
    assert(collTag == CollisionTag_e::BULLET_ENEMY_CT ||
           collTag == CollisionTag_e::BULLET_PLAYER_CT);
    GeneralCollisionComponent *genColl = m_ecsManager.getComponentManager().
            searchComponentByType<GeneralCollisionComponent>(bulletEntity,
            Components_e::GENERAL_COLLISION_COMPONENT);
    SegmentCollisionComponent *segmentColl = m_ecsManager.getComponentManager().
            searchComponentByType<SegmentCollisionComponent>(bulletEntity,
            Components_e::SEGMENT_COLLISION_COMPONENT);
    assert(genColl);
    assert(segmentColl);
    genColl->m_tag = collTag;
    genColl->m_shape = CollisionShape_e::SEGMENT_C;
    genColl->m_active = true;
    segmentColl->m_degreeOrientation = degreeAngle;
    segmentColl->m_points.first = point;
}

//===================================================================
void MainEngine::updateDisplayAmmoCount(PlayerConfComponent *playerComp)
{
    WriteComponent *writeConp = m_ecsManager.getComponentManager().
            searchComponentByType<WriteComponent>(playerComp->m_ammoWriteEntity,
                                                  Components_e::WRITE_COMPONENT);
    assert(writeConp);
    m_graphicEngine.updateAmmoCount(writeConp, playerComp);
}

//===================================================================
void MainEngine::setUnsetPaused()
{
    m_gamePaused = !m_gamePaused;
    if(m_gamePaused)
    {
        memTimerPausedValue();
    }
    else
    {
        applyTimerPausedValue();
    }
}

//===================================================================
void MainEngine::confSystems()
{
    m_graphicEngine.confSystems();
}

//===================================================================
void MainEngine::memTimerPausedValue()
{
    TimerComponent *timerComp;
    std::bitset<Components_e::TOTAL_COMPONENTS> bitset;
    bitset[Components_e::TIMER_COMPONENT] = true;
    std::vector<uint32_t> vectEntities = m_ecsManager.getEntityContainingComponents(bitset);
    assert(m_vectMemPausedTimer.empty());
    m_vectMemPausedTimer.reserve(vectEntities.size());
    for(uint32_t i = 0; i < vectEntities.size(); ++i)
    {
        timerComp = m_ecsManager.getComponentManager().
                searchComponentByType<TimerComponent>(vectEntities[i],
                                                      Components_e::TIMER_COMPONENT);
        assert(timerComp);
        time_t time = (std::chrono::system_clock::to_time_t(
                           std::chrono::system_clock::now()) -
                       std::chrono::system_clock::to_time_t(timerComp->m_clockA));
        m_vectMemPausedTimer.emplace_back(vectEntities[i], time);
    }
}

//===================================================================
void MainEngine::applyTimerPausedValue()
{
    TimerComponent *timerComp;
    for(uint32_t i = 0; i < m_vectMemPausedTimer.size(); ++i)
    {
        timerComp = m_ecsManager.getComponentManager().
                searchComponentByType<TimerComponent>(m_vectMemPausedTimer[i].first,
                                                      Components_e::TIMER_COMPONENT);
        assert(timerComp);
        timerComp->m_clockA = std::chrono::system_clock::from_time_t( std::chrono::system_clock::to_time_t(
                    std::chrono::system_clock::now()) - m_vectMemPausedTimer[i].second);
    }
    m_vectMemPausedTimer.clear();
}


//===================================================================
void MainEngine::loadGraphicPicture(const PictureData &picData, const FontData &fontData)
{
    m_graphicEngine.loadPictureData(picData, fontData);
}

//===================================================================
void MainEngine::loadGroundAndCeilingEntities(const GroundCeilingData &groundData,
                                              const GroundCeilingData &ceilingData)
{
    GroundCeilingData const *ptr = &groundData;
    for(uint32_t i = 0; i < 2; ++i)
    {
        std::bitset<Components_e::TOTAL_COMPONENTS> bitsetComponents;
        bitsetComponents[Components_e::POSITION_VERTEX_COMPONENT] = true;
        if(ptr->m_apparence == DisplayType_e::COLOR)
        {
            bitsetComponents[Components_e::COLOR_VERTEX_COMPONENT] = true;
        }
        else if(ptr->m_apparence == DisplayType_e::TEXTURE)
        {
            bitsetComponents[Components_e::SPRITE_TEXTURE_COMPONENT] = true;
        }
        else
        {
            bitsetComponents[Components_e::SPRITE_TEXTURE_COMPONENT] = true;
            bitsetComponents[Components_e::COLOR_VERTEX_COMPONENT] = true;
        }
        uint32_t entityNum = m_ecsManager.addEntity(bitsetComponents);
        if(i)
        {
            confGroundComponents(entityNum);
        }
        else
        {
            confCeilingComponents(entityNum);
        }
        ptr = &ceilingData;
    }
}

//===================================================================
void MainEngine::loadLevelEntities(const LevelManager &levelManager)
{
    loadGroundAndCeilingEntities(levelManager.getPictureData().getGroundData(),
                                 levelManager.getPictureData().getCeilingData());
    loadStaticElementEntities(levelManager);
    loadPlayerEntity(levelManager.getLevel(), loadWeaponsEntity(levelManager));
    Level::initLevelElementArray();
    loadWallEntities(levelManager);
    loadDoorEntities(levelManager);
    loadEnemiesEntities(levelManager);
}

//===================================================================
uint32_t MainEngine::loadWeaponsEntity(const LevelManager &levelManager)
{
    uint32_t weaponEntity = loadWeaponEntity();
    const std::vector<SpriteData> &vectSprite = levelManager.getPictureData().getSpriteData();
    const std::vector<pairUIPairFloat_t> &vectWeapons = levelManager.
            getLevel().getWeaponsData();
    MemSpriteDataComponent *memSprite = m_ecsManager.getComponentManager().
            searchComponentByType<MemSpriteDataComponent>(weaponEntity,
                                                          Components_e::MEM_SPRITE_DATA_COMPONENT);
    MemPositionsVertexComponents *memPosVertex = m_ecsManager.getComponentManager().
            searchComponentByType<MemPositionsVertexComponents>(weaponEntity,
                                                                Components_e::MEM_POSITIONS_VERTEX_COMPONENT);
    assert(memSprite);
    assert(memPosVertex);
    memSprite->m_vectSpriteData.reserve(static_cast<uint32_t>(WeaponsSpriteType_e::TOTAL_SPRITE));
    float posUp, posDown = -1.0f, posLeft, posRight, diffLateral;
    for(uint32_t i = 0; i < vectWeapons.size(); ++i)
    {
        memSprite->m_vectSpriteData.emplace_back(&vectSprite[vectWeapons[i].first]);
        posUp = -1.0f + vectWeapons[i].second.second;
        diffLateral = vectWeapons[i].second.first / 2.0f;
        posLeft = -diffLateral;
        posRight = diffLateral;
        memPosVertex->m_vectSpriteData.emplace_back(std::array<pairFloat_t, 4>{
                                                        {
                                                            {posLeft, posUp},
                                                            {posRight, posUp},
                                                            {posRight, posDown},
                                                            {posLeft, posDown}
                                                        }
                                                    });
    }
    return weaponEntity;
}

//===================================================================
void MainEngine::loadWallEntities(const LevelManager &levelManager)
{
    const std::vector<WallData> &wallData = levelManager.getLevel().getWallData();
    MemSpriteDataComponent *memSpriteComp;
    SpriteTextureComponent *spriteComp;
    const std::vector<SpriteData> &vectSprite = levelManager.getPictureData().getSpriteData();
    assert(!Level::getLevelCaseType().empty());
    for(uint32_t i = 0; i < wallData.size(); ++i)
    {
        const SpriteData &memSpriteData = levelManager.getPictureData().getSpriteData()[wallData[i].m_sprites[0]];
        for(uint32_t j = 0; j < wallData[i].m_TileGamePosition.size(); ++j)
        {
            uint32_t numEntity = createWallEntity();
            confBaseComponent(numEntity, memSpriteData, wallData[i].m_TileGamePosition[j],
                              CollisionShape_e::RECTANGLE_C);
            spriteComp = m_ecsManager.getComponentManager().
                    searchComponentByType<SpriteTextureComponent>(numEntity, Components_e::SPRITE_TEXTURE_COMPONENT);
            assert(spriteComp);
            Level::addElementCase(spriteComp, wallData[i].m_TileGamePosition[j], LevelCaseType_e::WALL_LC, numEntity);
            memSpriteComp = m_ecsManager.getComponentManager().
                    searchComponentByType<MemSpriteDataComponent>(numEntity,
                                                                  Components_e::MEM_SPRITE_DATA_COMPONENT);
            if(!memSpriteComp)
            {
                continue;
            }
            uint32_t vectSize = wallData[i].m_sprites.size();
            memSpriteComp->m_vectSpriteData.reserve(static_cast<uint32_t>(WallSpriteType_e::TOTAL_SPRITE));
            for(uint32_t j = 0; j < vectSize; ++j)
            {
                memSpriteComp->m_vectSpriteData.emplace_back(&vectSprite[wallData[i].m_sprites[j]]);
            }
        }
    }
}

//===================================================================
void MainEngine::loadDoorEntities(const LevelManager &levelManager)
{
    const std::vector<DoorData> &doorData = levelManager.getLevel().getDoorData();
    MemSpriteDataComponent *memSpriteComp;
    const std::vector<SpriteData> &vectSprite = levelManager.getPictureData().getSpriteData();
    for(uint32_t i = 0; i < doorData.size(); ++i)
    {
        const SpriteData &memSpriteData = levelManager.getPictureData().getSpriteData()[doorData[i].m_numSprite];
        for(uint32_t j = 0; j < doorData[i].m_TileGamePosition.size(); ++j)
        {
            uint32_t numEntity = createDoorEntity();
            confBaseComponent(numEntity, memSpriteData, doorData[i].m_TileGamePosition[j],
                              CollisionShape_e::RECTANGLE_C);
            GeneralCollisionComponent *tagComp = m_ecsManager.getComponentManager().
                    searchComponentByType<GeneralCollisionComponent>(numEntity, Components_e::GENERAL_COLLISION_COMPONENT);
            assert(tagComp);
            tagComp->m_tag = CollisionTag_e::DOOR_CT;
            MapCoordComponent *mapComp = m_ecsManager.getComponentManager().
                    searchComponentByType<MapCoordComponent>(numEntity, Components_e::MAP_COORD_COMPONENT);
            assert(mapComp);
            RectangleCollisionComponent *rectComp = m_ecsManager.getComponentManager().
                    searchComponentByType<RectangleCollisionComponent>(numEntity, Components_e::RECTANGLE_COLLISION_COMPONENT);
            assert(rectComp);
            DoorComponent *doorComp = m_ecsManager.getComponentManager().
                    searchComponentByType<DoorComponent>(numEntity, Components_e::DOOR_COMPONENT);
            assert(doorComp);
            if(doorData[i].m_vertical)
            {
                mapComp->m_absoluteMapPositionPX.first += DOOR_CASE_POS_PX;
                rectComp->m_size = {WIDTH_DOOR_SIZE_PX, LEVEL_TILE_SIZE_PX};
            }
            else
            {
                mapComp->m_absoluteMapPositionPX.second += DOOR_CASE_POS_PX;
                rectComp->m_size = {LEVEL_TILE_SIZE_PX, WIDTH_DOOR_SIZE_PX};
            }
            doorComp->m_vertical = doorData[i].m_vertical;
            memSpriteComp = m_ecsManager.getComponentManager().
                    searchComponentByType<MemSpriteDataComponent>(numEntity,
                                                                  Components_e::MEM_SPRITE_DATA_COMPONENT);

            SpriteTextureComponent *spriteComp = m_ecsManager.getComponentManager().
                    searchComponentByType<SpriteTextureComponent>(numEntity, Components_e::SPRITE_TEXTURE_COMPONENT);
            assert(spriteComp);
            Level::addElementCase(spriteComp, doorData[i].m_TileGamePosition[j], LevelCaseType_e::DOOR_LC, numEntity);

            if(!memSpriteComp)
            {
                continue;
            }
            memSpriteComp->m_vectSpriteData.emplace_back(&vectSprite[doorData[i].m_numSprite]);
        }
    }
}

//===================================================================
void MainEngine::loadEnemiesEntities(const LevelManager &levelManager)
{
    const std::vector<EnemyData> &enemiesData = levelManager.getLevel().getEnemiesData();
    for(uint32_t i = 0; i < enemiesData.size(); ++i)
    {
        const SpriteData &memSpriteData = levelManager.getPictureData().
                getSpriteData()[enemiesData[i].m_staticSprites[0]];
        for(uint32_t j = 0; j < enemiesData[i].m_TileGamePosition.size(); ++j)
        {
            uint32_t numEntity = createEnemyEntity();
            confBaseComponent(numEntity, memSpriteData,
                              enemiesData[i].m_TileGamePosition[j], CollisionShape_e::CIRCLE_C);
            loadEnemySprites(levelManager.getPictureData().getSpriteData(), enemiesData, numEntity);
        }
    }
}

//===================================================================
void MainEngine::loadEnemySprites(const std::vector<SpriteData> &vectSprite,
                                  const std::vector<EnemyData> &enemiesData, uint32_t numEntity)
{
    MemSpriteDataComponent *memSpriteComp = m_ecsManager.getComponentManager().
            searchComponentByType<MemSpriteDataComponent>(numEntity, Components_e::MEM_SPRITE_DATA_COMPONENT);
    assert(memSpriteComp);
    for(uint32_t i = 0; i < enemiesData.size(); ++i)
    {
        uint32_t vectSize = static_cast<uint32_t>(EnemySpriteType_e::TOTAL_SPRITE);
        memSpriteComp->m_vectSpriteData.reserve(vectSize);
        for(uint32_t j = 0; j < enemiesData[i].m_staticSprites.size(); ++j)
        {
            memSpriteComp->m_vectSpriteData.emplace_back(&vectSprite[enemiesData[i].m_staticSprites[j]]);
        }
        for(uint32_t j = 0; j < enemiesData[i].m_moveSprites.size(); ++j)
        {
            memSpriteComp->m_vectSpriteData.emplace_back(&vectSprite[enemiesData[i].m_moveSprites[j]]);
        }
        for(uint32_t j = 0; j < enemiesData[i].m_attackSprites.size(); ++j)
        {
            memSpriteComp->m_vectSpriteData.emplace_back(&vectSprite[enemiesData[i].m_attackSprites[j]]);
        }
        for(uint32_t j = 0; j < enemiesData[i].m_dyingSprites.size(); ++j)
        {
            memSpriteComp->m_vectSpriteData.emplace_back(&vectSprite[enemiesData[i].m_dyingSprites[j]]);
        }
        //OOOOOK TMP
        assert(memSpriteComp->m_vectSpriteData.size() > vectSize);
    }
}

//===================================================================
uint32_t MainEngine::loadWeaponEntity()
{
    std::bitset<Components_e::TOTAL_COMPONENTS> bitsetComponents;
    bitsetComponents[Components_e::POSITION_VERTEX_COMPONENT] = true;
    bitsetComponents[Components_e::SPRITE_TEXTURE_COMPONENT] = true;
    bitsetComponents[Components_e::MEM_SPRITE_DATA_COMPONENT] = true;
    bitsetComponents[Components_e::MEM_POSITIONS_VERTEX_COMPONENT] = true;
    bitsetComponents[Components_e::TIMER_COMPONENT] = true;
    return m_ecsManager.addEntity(bitsetComponents);
}

//===================================================================
uint32_t MainEngine::createWallEntity()
{
    std::bitset<Components_e::TOTAL_COMPONENTS> bitsetComponents;
    bitsetComponents[Components_e::POSITION_VERTEX_COMPONENT] = true;
    bitsetComponents[Components_e::SPRITE_TEXTURE_COMPONENT] = true;
    bitsetComponents[Components_e::MAP_COORD_COMPONENT] = true;
    bitsetComponents[Components_e::RECTANGLE_COLLISION_COMPONENT] = true;
    bitsetComponents[Components_e::GENERAL_COLLISION_COMPONENT] = true;
//    bitsetComponents[Components_e::MEM_SPRITE_DATA_COMPONENT] = true;
    bitsetComponents[Components_e::TIMER_COMPONENT] = true;
    return m_ecsManager.addEntity(bitsetComponents);
}

//===================================================================
uint32_t MainEngine::createDoorEntity()
{
    std::bitset<Components_e::TOTAL_COMPONENTS> bitsetComponents;
    bitsetComponents[Components_e::POSITION_VERTEX_COMPONENT] = true;
    bitsetComponents[Components_e::SPRITE_TEXTURE_COMPONENT] = true;
    bitsetComponents[Components_e::MAP_COORD_COMPONENT] = true;
    bitsetComponents[Components_e::RECTANGLE_COLLISION_COMPONENT] = true;
    bitsetComponents[Components_e::GENERAL_COLLISION_COMPONENT] = true;
    bitsetComponents[Components_e::DOOR_COMPONENT] = true;
    bitsetComponents[Components_e::TIMER_COMPONENT] = true;
    return m_ecsManager.addEntity(bitsetComponents);
}

//===================================================================
uint32_t MainEngine::createEnemyEntity()
{
    std::bitset<Components_e::TOTAL_COMPONENTS> bitsetComponents;
    bitsetComponents[Components_e::POSITION_VERTEX_COMPONENT] = true;
    bitsetComponents[Components_e::SPRITE_TEXTURE_COMPONENT] = true;
    bitsetComponents[Components_e::MAP_COORD_COMPONENT] = true;
    bitsetComponents[Components_e::CIRCLE_COLLISION_COMPONENT] = true;
    bitsetComponents[Components_e::GENERAL_COLLISION_COMPONENT] = true;
    bitsetComponents[Components_e::MOVEABLE_COMPONENT] = true;
    bitsetComponents[Components_e::MEM_SPRITE_DATA_COMPONENT] = true;
    bitsetComponents[Components_e::ENEMY_CONF_COMPONENT] = true;
    bitsetComponents[Components_e::TIMER_COMPONENT] = true;
    return m_ecsManager.addEntity(bitsetComponents);
}

//===================================================================
uint32_t MainEngine::createShotEntity()
{
    std::bitset<Components_e::TOTAL_COMPONENTS> bitsetComponents;
    bitsetComponents[Components_e::SEGMENT_COLLISION_COMPONENT] = true;
    bitsetComponents[Components_e::GENERAL_COLLISION_COMPONENT] = true;
    return m_ecsManager.addEntity(bitsetComponents);
}

//===================================================================
uint32_t MainEngine::createWriteEntity()
{
    std::bitset<Components_e::TOTAL_COMPONENTS> bitsetComponents;
    bitsetComponents[Components_e::POSITION_VERTEX_COMPONENT] = true;
    bitsetComponents[Components_e::WRITE_COMPONENT] = true;
    return m_ecsManager.addEntity(bitsetComponents);
}

//===================================================================
uint32_t MainEngine::createSimpleSpriteEntity()
{
    std::bitset<Components_e::TOTAL_COMPONENTS> bitsetComponents;
    bitsetComponents[Components_e::POSITION_VERTEX_COMPONENT] = true;
    bitsetComponents[Components_e::SPRITE_TEXTURE_COMPONENT] = true;
    return m_ecsManager.addEntity(bitsetComponents);
}

//===================================================================
uint32_t MainEngine::createStaticEntity()
{
    std::bitset<Components_e::TOTAL_COMPONENTS> bitsetComponents;
    bitsetComponents[Components_e::POSITION_VERTEX_COMPONENT] = true;
    bitsetComponents[Components_e::SPRITE_TEXTURE_COMPONENT] = true;
    bitsetComponents[Components_e::MAP_COORD_COMPONENT] = true;
    bitsetComponents[Components_e::STATIC_ELEMENT_COMPONENT] = true;
    bitsetComponents[Components_e::RECTANGLE_COLLISION_COMPONENT] = true;
    bitsetComponents[Components_e::GENERAL_COLLISION_COMPONENT] = true;
    return m_ecsManager.addEntity(bitsetComponents);
}

//===================================================================
void MainEngine::confBaseComponent(uint32_t entityNum,
                                   const SpriteData &memSpriteData,
                                   const pairUI_t& coordLevel,
                                   CollisionShape_e collisionShape)
{
    SpriteTextureComponent *spriteComp = m_ecsManager.getComponentManager().
            searchComponentByType<SpriteTextureComponent>(entityNum, Components_e::SPRITE_TEXTURE_COMPONENT);
    assert(spriteComp);
    spriteComp->m_spriteData = &memSpriteData;
    MapCoordComponent *mapComp = m_ecsManager.getComponentManager().
            searchComponentByType<MapCoordComponent>(entityNum, Components_e::MAP_COORD_COMPONENT);
    assert(mapComp);
    GeneralCollisionComponent *tagComp = m_ecsManager.getComponentManager().
            searchComponentByType<GeneralCollisionComponent>(entityNum, Components_e::GENERAL_COLLISION_COMPONENT);
    assert(tagComp);
    if(collisionShape == CollisionShape_e::RECTANGLE_C)
    {
        RectangleCollisionComponent *rectComp = m_ecsManager.getComponentManager().
                searchComponentByType<RectangleCollisionComponent>(entityNum, Components_e::RECTANGLE_COLLISION_COMPONENT);
        assert(rectComp);
        rectComp->m_size = {LEVEL_TILE_SIZE_PX, LEVEL_TILE_SIZE_PX};
        tagComp->m_tag = CollisionTag_e::WALL_CT;
    }
    else if(collisionShape == CollisionShape_e::CIRCLE_C)
    {
        CircleCollisionComponent *circleComp = m_ecsManager.getComponentManager().
                searchComponentByType<CircleCollisionComponent>(entityNum, Components_e::CIRCLE_COLLISION_COMPONENT);
        assert(circleComp);
        circleComp->m_ray = ENEMY_RAY;
        tagComp->m_tag = CollisionTag_e::ENEMY_CT;
        spriteComp->m_glFpsSize.first = 0.5f;
        spriteComp->m_glFpsSize.second = 0.8f;
    }
    mapComp->m_coord = coordLevel;
    mapComp->m_absoluteMapPositionPX = getAbsolutePosition(coordLevel);
    tagComp->m_shape = collisionShape;
}

//===================================================================
void MainEngine::confStaticComponent(uint32_t entityNum,
                                     const pairFloat_t& elementSize,
                                     bool traversable,
                                     LevelStaticElementType_e type)
{
    StaticElementComponent *staticComp = m_ecsManager.getComponentManager().
            searchComponentByType<StaticElementComponent>(entityNum, Components_e::STATIC_ELEMENT_COMPONENT);
    assert(staticComp);
    staticComp->m_inGameSpriteSize = elementSize;
    staticComp->m_traversable = traversable;
    staticComp->m_type = type;
}

//===================================================================
void MainEngine::loadPlayerEntity(const Level &level, uint32_t numWeaponEntity)
{
    std::bitset<Components_e::TOTAL_COMPONENTS> bitsetComponents;
    bitsetComponents[Components_e::POSITION_VERTEX_COMPONENT] = true;
    bitsetComponents[Components_e::MAP_COORD_COMPONENT] = true;
    bitsetComponents[Components_e::MOVEABLE_COMPONENT] = true;
    bitsetComponents[Components_e::COLOR_VERTEX_COMPONENT] = true;
    bitsetComponents[Components_e::INPUT_COMPONENT] = true;
    bitsetComponents[Components_e::CIRCLE_COLLISION_COMPONENT] = true;
    bitsetComponents[Components_e::GENERAL_COLLISION_COMPONENT] = true;
    bitsetComponents[Components_e::VISION_COMPONENT] = true;
    bitsetComponents[Components_e::PLAYER_CONF_COMPONENT] = true;
    uint32_t entityNum = m_ecsManager.addEntity(bitsetComponents);
    confPlayerEntity(entityNum, level, numWeaponEntity);
    //notify player entity number
    m_graphicEngine.getMapSystem().confPlayerComp(entityNum);
    m_physicalEngine.memPlayerEntity(entityNum);
}

//===================================================================
void MainEngine::confPlayerEntity(uint32_t entityNum, const Level &level, uint32_t numWeaponEntity)
{
    PositionVertexComponent *pos = m_ecsManager.getComponentManager().
            searchComponentByType<PositionVertexComponent>(entityNum,
                                                           Components_e::POSITION_VERTEX_COMPONENT);
    MapCoordComponent *map = m_ecsManager.getComponentManager().
            searchComponentByType<MapCoordComponent>(entityNum,
                                                     Components_e::MAP_COORD_COMPONENT);
    MoveableComponent *move = m_ecsManager.getComponentManager().
            searchComponentByType<MoveableComponent>(entityNum,
                                                     Components_e::MOVEABLE_COMPONENT);
    ColorVertexComponent *color = m_ecsManager.getComponentManager().
            searchComponentByType<ColorVertexComponent>(entityNum,
                                                     Components_e::COLOR_VERTEX_COMPONENT);
    CircleCollisionComponent *circleColl = m_ecsManager.getComponentManager().
            searchComponentByType<CircleCollisionComponent>(entityNum,
                                                     Components_e::CIRCLE_COLLISION_COMPONENT);
    GeneralCollisionComponent *tagColl = m_ecsManager.getComponentManager().
            searchComponentByType<GeneralCollisionComponent>(entityNum,
                                                     Components_e::GENERAL_COLLISION_COMPONENT);
    VisionComponent *vision = m_ecsManager.getComponentManager().
            searchComponentByType<VisionComponent>(entityNum,
                                                     Components_e::VISION_COMPONENT);
    PlayerConfComponent *playerConf = m_ecsManager.getComponentManager().
            searchComponentByType<PlayerConfComponent>(entityNum,
                                                     Components_e::PLAYER_CONF_COMPONENT);
    assert(pos);
    assert(map);
    assert(move);
    assert(color);
    assert(circleColl);
    assert(tagColl);
    assert(vision);
    assert(playerConf);
    map->m_coord = level.getPlayerDeparture();
    Direction_e playerDir = level.getPlayerDepartureDirection();
    switch(playerDir)
    {
    case Direction_e::NORTH:
        move->m_degreeOrientation = 90.0f;
        break;
    case Direction_e::EAST:
        move->m_degreeOrientation = 0.0f;
        break;
    case Direction_e::SOUTH:
        move->m_degreeOrientation = 270.0f;
        break;
    case Direction_e::WEST:
        move->m_degreeOrientation = 180.0f;
        break;
    }
    map->m_absoluteMapPositionPX = getAbsolutePosition(map->m_coord);
    updatePlayerOrientation(*move, *pos, *vision);

    color->m_vertex.reserve(3);
    color->m_vertex.emplace_back(0.9f, 0.00f, 0.00f);
    color->m_vertex.emplace_back(0.9f, 0.00f, 0.00f);
    color->m_vertex.emplace_back(0.9f, 0.00f, 0.00f);
    circleColl->m_ray = PLAYER_RAY;
    tagColl->m_tag = CollisionTag_e::PLAYER_CT;
    tagColl->m_shape = CollisionShape_e::CIRCLE_C;
    playerConf->m_weaponEntity = numWeaponEntity;
    playerConf->m_currentWeapon = WeaponsType_e::GUN;
    playerConf->m_previousWeapon = WeaponsType_e::GUN;
    //set standart weapon sprite
    StaticDisplaySystem *staticDisplay = m_ecsManager.getSystemManager().
            searchSystemByType<StaticDisplaySystem>(static_cast<uint32_t>(Systems_e::STATIC_DISPLAY_SYSTEM));
    assert(staticDisplay);
    staticDisplay->setWeaponSprite(numWeaponEntity, WeaponsSpriteType_e::GUN_STATIC);
    confWriteEntities(playerConf);
    confMenuCursorEntity(playerConf);
}

//===================================================================
void MainEngine::confWriteEntities(PlayerConfComponent *playerConf)
{
    uint32_t numAmmoWrite = createWriteEntity(),
            numLifeWrite = createWriteEntity(), numMenuWrite = createWriteEntity();
    //AMMO
    WriteComponent *writeConf = m_ecsManager.getComponentManager().
            searchComponentByType<WriteComponent>(numAmmoWrite, Components_e::WRITE_COMPONENT);
    assert(writeConf);
    writeConf->m_upLeftPositionGL = {-0.95f, -0.9f};
    writeConf->m_fontSize = STD_FONT_SIZE;
    m_graphicEngine.updateAmmoCount(writeConf, playerConf);
    //LIFE
    writeConf = m_ecsManager.getComponentManager().
            searchComponentByType<WriteComponent>(numLifeWrite, Components_e::WRITE_COMPONENT);
    assert(writeConf);
    writeConf->m_upLeftPositionGL = {-0.95f, -0.8f};
    writeConf->m_fontSize = STD_FONT_SIZE;
    m_graphicEngine.updatePlayerLife(writeConf, playerConf);
    //MENU
    writeConf = m_ecsManager.getComponentManager().
            searchComponentByType<WriteComponent>(numMenuWrite, Components_e::WRITE_COMPONENT);
    assert(writeConf);
    writeConf->m_upLeftPositionGL = m_menuCornerUpLeft;
    writeConf->m_fontSize = MENU_FONT_SIZE;
    m_graphicEngine.fillMenuWrite(writeConf);
    playerConf->m_menuEntity = numMenuWrite;
    playerConf->m_ammoWriteEntity = numAmmoWrite;
    playerConf->m_lifeWriteEntity = numLifeWrite;
}

//===================================================================
void MainEngine::confMenuCursorEntity(PlayerConfComponent *playerConf)
{
    uint32_t cursorEntity = createSimpleSpriteEntity();
    PositionVertexComponent *posCursor = m_ecsManager.getComponentManager().
            searchComponentByType<PositionVertexComponent>(cursorEntity,
                                                           Components_e::POSITION_VERTEX_COMPONENT);
    SpriteTextureComponent *spriteCursor = m_ecsManager.getComponentManager().
            searchComponentByType<SpriteTextureComponent>(cursorEntity,
                                                          Components_e::SPRITE_TEXTURE_COMPONENT);
    assert(posCursor);
    assert(spriteCursor);
    assert(m_memCursorSpriteData);
    playerConf->m_menuCursorEntity = cursorEntity;
    spriteCursor->m_spriteData = m_memCursorSpriteData;
    posCursor->m_vertex.reserve(4);
    float leftPos = m_menuCornerUpLeft.first - 0.25f,
            rightPos = m_menuCornerUpLeft.first - 0.05f,
            upPos = m_menuCornerUpLeft.second,
            downPos = m_menuCornerUpLeft.second - 0.25f;
    posCursor->m_vertex.emplace_back(pairFloat_t{leftPos, upPos});
    posCursor->m_vertex.emplace_back(pairFloat_t{rightPos, upPos});
    posCursor->m_vertex.emplace_back(pairFloat_t{rightPos, downPos});
    posCursor->m_vertex.emplace_back(pairFloat_t{leftPos, downPos});
}

//===================================================================
void MainEngine::loadStaticElementEntities(const LevelManager &levelManager)
{
    std::vector<StaticLevelElementData> const *staticData =
            &levelManager.getLevel().getGroundElementData();//0
    uint8_t cursorSpriteId = *levelManager.getPictureData().
            getIdentifier(levelManager.getCursorSpriteName());
    m_memCursorSpriteData = &levelManager.getPictureData().getSpriteData()[cursorSpriteId];
    for(uint32_t h = 0; h < 3; ++h)
    {
        for(uint32_t i = 0; i < staticData->size(); ++i)
        {
            const SpriteData &memSpriteData = levelManager.getPictureData().
                    getSpriteData()[staticData->operator[](i).m_numSprite];
            for(uint32_t j = 0; j < staticData->operator[](i).m_TileGamePosition.size(); ++j)
            {
                uint32_t entityNum = createStaticEntity();
                confBaseComponent(entityNum,
                                  memSpriteData,
                                  staticData->operator[](i).m_TileGamePosition[j],
                        CollisionShape_e::RECTANGLE_C);
                confStaticComponent(entityNum,
                                       staticData->operator[](i).m_inGameSpriteSize,
                                       staticData->operator[](i).m_traversable,
                                       static_cast<LevelStaticElementType_e>(h));
            }
        }
        if(!h)//h == 1
        {
            staticData = &levelManager.getLevel().getCeilingElementData();
        }
        else// h == 2
        {
            staticData = &levelManager.getLevel().getObjectElementData();
        }
    }
}

//===================================================================
void MainEngine::confGroundComponents(uint32_t entityNum)
{
    PositionVertexComponent *posComp = m_ecsManager.getComponentManager().
            searchComponentByType<PositionVertexComponent>(entityNum, Components_e::POSITION_VERTEX_COMPONENT);
    assert(posComp);
    posComp->m_vertex.reserve(4);
    posComp->m_vertex.emplace_back(-1.0f, 0.0f);
    posComp->m_vertex.emplace_back(1.0f, 0.0f);
    posComp->m_vertex.emplace_back(1.0f, -1.0f);
    posComp->m_vertex.emplace_back(-1.0f, -1.0f);

    ColorVertexComponent *colorComp = m_ecsManager.getComponentManager().
            searchComponentByType<ColorVertexComponent>(entityNum, Components_e::COLOR_VERTEX_COMPONENT);
    assert(colorComp);
    colorComp->m_vertex.reserve(4);
    colorComp->m_vertex.emplace_back(0.1f, 0.2f, 0.1f);
    colorComp->m_vertex.emplace_back(0.1f, 0.2f, 0.1f);
    colorComp->m_vertex.emplace_back(0.3f, 0.9f, 0.1f);
    colorComp->m_vertex.emplace_back(0.3f, 0.9f, 0.1f);

}

//===================================================================
void MainEngine::confCeilingComponents(uint32_t entityNum)
{
    PositionVertexComponent *posComp = m_ecsManager.getComponentManager().
            searchComponentByType<PositionVertexComponent>(entityNum, Components_e::POSITION_VERTEX_COMPONENT);
    assert(posComp);
    posComp->m_vertex.reserve(4);
    posComp->m_vertex.emplace_back(-1.0f, 1.0f);
    posComp->m_vertex.emplace_back(1.0f, 1.0f);
    posComp->m_vertex.emplace_back(1.0f, 0.0f);
    posComp->m_vertex.emplace_back(-1.0f, 0.0f);

    ColorVertexComponent *colorComp = m_ecsManager.getComponentManager().
            searchComponentByType<ColorVertexComponent>(entityNum, Components_e::COLOR_VERTEX_COMPONENT);
    assert(colorComp);
    colorComp->m_vertex.reserve(4);
    colorComp->m_vertex.emplace_back(0.1f, 0.2f, 0.3f);
    colorComp->m_vertex.emplace_back(0.1f, 0.2f, 0.3f);
    colorComp->m_vertex.emplace_back(0.3f, 0.9f, 0.8f);
    colorComp->m_vertex.emplace_back(0.3f, 0.9f, 0.8f);
}

//===================================================================
void MainEngine::linkSystemsToGraphicEngine()
{
    ColorDisplaySystem *color = m_ecsManager.getSystemManager().
            searchSystemByType<ColorDisplaySystem>(static_cast<uint32_t>(Systems_e::COLOR_DISPLAY_SYSTEM));
    MapDisplaySystem *map = m_ecsManager.getSystemManager().
            searchSystemByType<MapDisplaySystem>(static_cast<uint32_t>(Systems_e::MAP_DISPLAY_SYSTEM));
    FirstPersonDisplaySystem *first = m_ecsManager.getSystemManager().
            searchSystemByType<FirstPersonDisplaySystem>(static_cast<uint32_t>(Systems_e::FIRST_PERSON_DISPLAY_SYSTEM));
    VisionSystem *vision = m_ecsManager.getSystemManager().
            searchSystemByType<VisionSystem>(static_cast<uint32_t>(Systems_e::VISION_SYSTEM));
    StaticDisplaySystem *staticDisplay = m_ecsManager.getSystemManager().
            searchSystemByType<StaticDisplaySystem>(static_cast<uint32_t>(Systems_e::STATIC_DISPLAY_SYSTEM));
    m_graphicEngine.linkSystems(color, map, first, vision, staticDisplay);
}

//===================================================================
void MainEngine::linkSystemsToPhysicalEngine()
{
    InputSystem *input = m_ecsManager.getSystemManager().
            searchSystemByType<InputSystem>(static_cast<uint32_t>(Systems_e::INPUT_SYSTEM));
    CollisionSystem *coll = m_ecsManager.getSystemManager().
            searchSystemByType<CollisionSystem>(static_cast<uint32_t>(Systems_e::COLLISION_SYSTEM));
    DoorSystem *door = m_ecsManager.getSystemManager().
            searchSystemByType<DoorSystem>(static_cast<uint32_t>(Systems_e::DOOR_SYSTEM));
    IASystem *iaSystem = m_ecsManager.getSystemManager().
            searchSystemByType<IASystem>(static_cast<uint32_t>(Systems_e::IA_SYSTEM));
    input->setGLWindow(m_graphicEngine.getGLWindow());
    input->linkMainEngine(this);
    m_physicalEngine.linkSystems(input, coll, door, iaSystem);
}
