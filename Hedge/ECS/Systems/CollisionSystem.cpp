#include "CollisionSystem.hpp"
#include "ECS/Systems/MapDisplaySystem.hpp"
#include "constants.hpp"
#include <ECS/Components/MapCoordComponent.hpp>
#include <ECS/Components/GeneralCollisionComponent.hpp>
#include <ECS/Components/CircleCollisionComponent.hpp>
#include <ECS/Components/RectangleCollisionComponent.hpp>
#include <ECS/Components/SegmentCollisionComponent.hpp>
#include <ECS/Components/MoveableComponent.hpp>
#include <ECS/Components/PlayerConfComponent.hpp>
#include <ECS/Components/EnemyConfComponent.hpp>
#include <ECS/Components/TimerComponent.hpp>
#include <ECS/Components/ShotConfComponent.hpp>
#include <ECS/Components/LogComponent.hpp>
#include <ECS/Components/AudioComponent.hpp>
#include <ECS/Components/WeaponComponent.hpp>
#include <ECS/Components/GravityComponent.hpp>
#include <ECS/Components/CheckpointComponent.hpp>
#include <ECS/Components/VehiculeComponent.hpp>
#include <ECS/Systems/ColorDisplaySystem.hpp>
#include <CollisionUtils.hpp>
#include <PhysicalEngine.hpp>
#include <MainEngine.hpp>
#include <cassert>
#include <Level.hpp>
#include <math.h>
#include <alias.hpp>

using multiMapTagIt_t = std::multimap<CollisionTag_e, CollisionTag_e>::const_iterator;

//===================================================================
CollisionSystem::CollisionSystem()
{
    setUsedComponents();
    initArrayTag();
}

//===================================================================
void CollisionSystem::setUsedComponents()
{
    addComponentsToSystem(Components_e::GENERAL_COLLISION_COMPONENT, 1);
}

//===================================================================
void CollisionSystem::execSystem()
{
    uint32_t i = 0;
    m_refMainEngine->unsetCurrentWallOnGround();
    GravityComponent *tagCompA = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(m_playerEntity);
    assert(tagCompA);
    if(m_memGround)
    {
        tagCompA->m_memOnGround = false;
    }
    m_memGround = tagCompA->m_memOnGround;
    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerComp);
    playerComp->m_insideWall = false;
    if(playerComp->m_associatedVehicle)
    {
        VehicleComponent *vehicleComp = Ecsm_t::instance().getComponent<VehicleComponent, Components_e::VEHICLE_COMPONENT>(*playerComp->m_associatedVehicle);
        assert(vehicleComp);
        vehicleComp->m_onLateralGround = false;
        vehicleComp->m_touchGround = false;
        vehicleComp->m_onStair = false;
        ++vehicleComp->m_stairCount;
    }
    for(std::set<uint32_t>::iterator it = m_usedEntities.begin(); it != m_usedEntities.end(); ++it, ++i)
    {
        m_memMoveableWallCrush = false;
        SegmentCollisionComponent *segmentCompA = nullptr;
        GeneralCollisionComponent *tagCompA = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(*it);
        assert(tagCompA);
        if(tagCompA->m_tagA == CollisionTag_e::PLAYER_CT)
        {
            PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
            if(playerComp->m_associatedVehicle)
            {
                treatCollisionVehiclePlayer(*playerComp->m_associatedVehicle);
                continue;
            }
        }
        //check if entity is moveable
        MoveableComponent *moveCompA = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(*it);
        if(!tagCompA->m_active || tagCompA->m_tagA == CollisionTag_e::WALL_CT || tagCompA->m_tagA == CollisionTag_e::OBJECT_CT)
        {
            continue;
        }
        if(tagCompA->m_tagA == CollisionTag_e::BULLET_ENEMY_CT || tagCompA->m_tagA == CollisionTag_e::BULLET_PLAYER_CT)
        {
            ShotConfComponent *shotComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(*it);
            if(shotComp && shotComp->m_destructPhase)
            {
                continue;
            }
        }
        m_memCrush.clear();
        m_mapMemCrushPos.clear();
        if(tagCompA->m_tagA == CollisionTag_e::ENEMY_CT)
        {
            if(checkEnemyRemoveCollisionMask(*it))
            {
                rmEnemyCollisionMaskEntity(*it);
            }
        }
        secondEntitiesLoop(*it, i, *tagCompA);
        if(tagCompA->m_tagA == CollisionTag_e::EXPLOSION_CT)
        {
            setDamageCircle(*it, false);
        }
        else if(tagCompA->m_tagA == CollisionTag_e::PLAYER_ACTION_CT)
        {
            tagCompA->m_active = false;
        }
        if(moveCompA && (tagCompA->m_tagA == CollisionTag_e::PLAYER_CT || tagCompA->m_tagA == CollisionTag_e::ENEMY_CT || tagCompA->m_tagA == CollisionTag_e::VEHICULE_CT))
        {
            treatGeneralCrushing(*it);
            treatLimitLevel(*it, tagCompA->m_tagA);
        }
        if(segmentCompA && m_memDistCurrentBulletColl.second > EPSILON_FLOAT)
        {
            if(m_memDistCurrentBulletColl.first)
            {
                m_vectMemShots.emplace_back(PairUI_t{*it, (*m_memDistCurrentBulletColl.first)});
            }
        }
        treatSegmentShots();
        m_vectMemShots.clear();
    }
}

//===================================================================
void CollisionSystem::updateZonesColl()
{
    m_zoneLevel = std::make_unique<ZoneLevelColl>(Level::getSize());
}

//===================================================================
void CollisionSystem::secondEntitiesLoop(uint32_t entityA, uint32_t currentIteration, GeneralCollisionComponent &tagCompA)
{
    if(tagCompA.m_tagA == CollisionTag_e::DETECT_MAP_CT ||
            tagCompA.m_tagA == CollisionTag_e::BULLET_PLAYER_CT || tagCompA.m_tagA == CollisionTag_e::BULLET_ENEMY_CT)
    {
        for(std::set<uint32_t>::iterator it = m_usedEntities.begin(); it != m_usedEntities.end(); ++it)
        {
            if(!iterationLoop(currentIteration, entityA, *it, tagCompA))
            {
                return;
            }
        }
        return;
    }
    bool firstItJumpDown = false;
    SetUi_t set = m_zoneLevel->getEntitiesFromZones(entityA);
    SetUi_t::iterator it = set.begin();
    if(tagCompA.m_tagA == CollisionTag_e::PLAYER_CT)
    {
        if(m_playerJumpDown)
        {
            m_memPlayerJumpDown = false;
        }
        PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(entityA);
        assert(playerComp);
        if(playerComp->m_jumpDown)
        {
            firstItJumpDown = true;
            m_playerJumpDown = true;
            m_memPlayerJumpDown = false;
            playerComp->m_jumpDown = false;
        }
    }
    for(; it != set.end(); ++it)
    {
        iterationLoop(currentIteration, entityA, *it, tagCompA);
    }
    if(tagCompA.m_tagA == CollisionTag_e::PLAYER_CT && (!firstItJumpDown && !m_memPlayerJumpDown))
    {
        PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(entityA);
        assert(playerComp);
        m_playerJumpDown = false;
    }
}

//===================================================================
bool CollisionSystem::iterationLoop(uint32_t currentIteration, uint32_t entityA, uint32_t entityB,
                                    GeneralCollisionComponent &tagCompA)
{
    if(entityA == entityB)
    {
        return true;
    }
    GeneralCollisionComponent *tagCompB = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(entityB);
    assert(tagCompB);
    if(!tagCompB->m_active)
    {
        return true;
    }
    if(!checkTag(tagCompA.m_tagA, tagCompB->m_tagA) && !checkTag(tagCompA.m_tagA, tagCompB->m_tagB))
    {
        return true;
    }
    if(!treatCollision(entityA, entityB, tagCompA, *tagCompB))
    {
        if(tagCompA.m_tagA == CollisionTag_e::BULLET_PLAYER_CT || tagCompA.m_tagA == CollisionTag_e::BULLET_ENEMY_CT)
        {
            secondEntitiesLoop(entityA, currentIteration, tagCompA);
        }
        return false;
    }
    return true;
}

//===================================================================
bool CollisionSystem::checkEnemyRemoveCollisionMask(uint32_t entityNum)
{
    EnemyConfComponent *enemyConfComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(entityNum);
    assert(enemyConfComp);
    if(enemyConfComp->m_displayMode == EnemyDisplayMode_e::DEAD)
    {
        MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(entityNum);
        assert(moveComp);
        if(!moveComp->m_ejectData)
        {
            return true;
        }
    }
    return false;
}

//===================================================================
void CollisionSystem::treatGeneralCrushing(uint32_t entityNum)
{
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNum);
    GeneralCollisionComponent *collComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(entityNum);
    assert(mapComp);
    assert(collComp);
    bool crush = false;
    for(uint32_t i = 0; i < m_memCrush.size(); ++i)
    {
        //QuickFix
        if(collComp->m_tagA == CollisionTag_e::ENEMY_CT)
        {
            mapComp->m_absoluteMapPositionPX.first += std::get<0>(m_memCrush[i]).first;
            mapComp->m_absoluteMapPositionPX.second += std::get<0>(m_memCrush[i]).second;
        }
        //3 == direction
        if(!crush && !std::get<1>(m_memCrush[i]))
        {
            for(uint32_t j = 0; j < i; ++j)
            {
                if((std::get<3>(m_memCrush[j]) || std::get<3>(m_memCrush[i])) &&
                        opposingDirection(std::get<2>(m_memCrush[j]), std::get<2>(m_memCrush[i])))
                {
                    crush = true;
                    treatCrushing(entityNum);
                    break;
                }
            }
        }
    }
    if(collComp->m_tagA != CollisionTag_e::PLAYER_CT)
    {
        return;
    }
    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(entityNum);
    assert(playerComp);
    if(!crush && !playerComp->m_insideWall)
    {
        playerComp->m_crush = false;
        playerComp->m_frozen = false;
    }
    if(playerComp->m_crush)
    {
        treatPlayerTakeDamage(1);
    }
}

//===================================================================
void CollisionSystem::treatLimitLevel(uint32_t entityNum, CollisionTag_e tag)
{
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNum);
    assert(mapComp);
    RectangleCollisionComponent *rectComp = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(entityNum);
    assert(rectComp);
    PairUI_t limitLevel = Level::getSize();
    if(tag == CollisionTag_e::PLAYER_CT)
    {
        uint32_t minLevelX = Ecsm_t::instance().getSystem<MapDisplaySystem>(static_cast<uint32_t>(Systems_e::MAP_DISPLAY_SYSTEM))->getMinLevelLock();
        if(mapComp->m_absoluteMapPositionPX.first < minLevelX)
        {
            mapComp->m_absoluteMapPositionPX.first = minLevelX;
        }
    }
    if(mapComp->m_absoluteMapPositionPX.first < 0.0f)
    {
        mapComp->m_absoluteMapPositionPX.first = 0.0f;
    }
    else if(mapComp->m_absoluteMapPositionPX.first + rectComp->m_size.first > (limitLevel.first * LEVEL_TILE_SIZE_PX))
    {
        mapComp->m_absoluteMapPositionPX.first = limitLevel.first * LEVEL_TILE_SIZE_PX - rectComp->m_size.first ;
    }
    if(mapComp->m_absoluteMapPositionPX.second < 0.0f)
    {
        mapComp->m_absoluteMapPositionPX.second = 0.0f;
    }
    else if(mapComp->m_absoluteMapPositionPX.second > ((limitLevel.second + 1) * LEVEL_TILE_SIZE_PX))
    {
        if(tag == CollisionTag_e::PLAYER_CT)
        {
            PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(entityNum);
            playerComp->m_life = 0;
        }
        else if(tag == CollisionTag_e::ENEMY_CT)
        {
            EnemyConfComponent *enemyComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(entityNum);
            enemyComp->m_life = 0;
            enemyComp->m_behaviourMode = EnemyBehaviourMode_e::DYING;
        }
    }
}

//===================================================================
void CollisionSystem::treatEnemyTakeDamage(uint32_t enemyEntityNum, uint32_t damage, std::optional<uint32_t> vehicleDamageMax)
{
    EnemyConfComponent *enemyConfCompB = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(enemyEntityNum);
    TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(enemyEntityNum);
    assert(enemyConfCompB);
    assert(timerComp);
    if(vehicleDamageMax && enemyConfCompB->m_life > *vehicleDamageMax)
    {
        return;
    }
    if(enemyConfCompB->m_behaviourMode == EnemyBehaviourMode_e::DYING)
    {
        return;
    }
    enemyConfCompB->m_touched = true;
    enemyConfCompB->m_behaviourMode = EnemyBehaviourMode_e::ATTACK;
    if(enemyConfCompB->m_frozenOnAttack)
    {
        timerComp->m_cycleCountC = 0;
        timerComp->m_cycleCountB = 0;
        enemyConfCompB->m_attackPhase = EnemyAttackPhase_e::SHOOTED;
    }
    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerComp);
    //if enemy dead
    if(!enemyConfCompB->takeDamage(damage))
    {
        GeneralCollisionComponent *tagComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(enemyEntityNum);
        tagComp->m_tagA = CollisionTag_e::GHOST_CT;
        GravityComponent *gravComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(enemyEntityNum);
        if(gravComp)
        {
            gravComp->m_freeze = true;
        }
        enemyConfCompB->m_behaviourMode = EnemyBehaviourMode_e::DYING;
        enemyConfCompB->m_touched = false;
        enemyConfCompB->m_playDeathSound = true;
        if(enemyConfCompB->m_dropedObjectEntity)
        {
            confDropedObject(*enemyConfCompB->m_dropedObjectEntity, enemyEntityNum);
        }
    }
}

//===================================================================
void CollisionSystem::treatPlayerTakeDamage(uint32_t damage)
{
    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerComp);
    if(playerComp->m_invulnerable)
    {
        return;
    }
    if(playerComp->m_associatedVehicle)
    {
        ShotConfComponent *shotComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(*playerComp->m_associatedVehicle);
        assert(shotComp);
        VehicleComponent *vehicleComp = Ecsm_t::instance().getComponent<VehicleComponent, Components_e::VEHICLE_COMPONENT>(*playerComp->m_associatedVehicle);
        assert(vehicleComp);
        //Vehicle destroyed
        if(damage >= vehicleComp->m_HP)
        {
            AudioComponent *audioComp = Ecsm_t::instance().getComponent<AudioComponent, Components_e::AUDIO_COMPONENT>(*playerComp->m_associatedVehicle);
            assert(audioComp);
            audioComp->m_soundElements[3]->m_toPlay = true;
            shotComp->m_destructPhase = true;
            vehicleComp->m_vehicleMemPlayerAssociated = false;
            playerComp->m_associatedVehicle = std::nullopt;
            GravityComponent *gravComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(*playerComp->m_associatedVehicle);
            assert(gravComp);
            GravityComponent *gravPlayerComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(m_playerEntity);
            assert(gravPlayerComp);
            gravComp->m_freeze = false;
            gravComp->m_exitVehicle = true;
        }
        else
        {
            vehicleComp->m_HP -= damage;
        }
    }
    else
    {
        playerComp->takeDamage(damage);
        //invulnerability frames
        TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(m_playerEntity);
        assert(timerComp);
        timerComp->m_cycleCountE = 0;
        playerComp->m_invulnerable = true;
    }
}

//===================================================================
void CollisionSystem::confDropedObject(uint32_t objectEntity, uint32_t enemyEntity)
{
    GeneralCollisionComponent *genComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(objectEntity);
    MapCoordComponent *objectMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(objectEntity);
    MapCoordComponent *enemyMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(enemyEntity);
    assert(genComp);
    assert(objectMapComp);
    assert(enemyMapComp);
    genComp->m_active = true;
    objectMapComp->m_coord = enemyMapComp->m_coord;
    addEntityToZone(objectEntity, objectMapComp->m_coord);
    objectMapComp->m_absoluteMapPositionPX = enemyMapComp->m_absoluteMapPositionPX;
}

//===================================================================
void CollisionSystem::treatSegmentShots()
{
    for(uint32_t i = 0; i < m_vectMemShots.size(); ++i)
    {
        GeneralCollisionComponent *tagCompTarget = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(m_vectMemShots[i].second);
        assert(tagCompTarget);
        if(tagCompTarget->m_tagA == CollisionTag_e::WALL_CT)
        {
            continue;
        }
        GeneralCollisionComponent *tagCompBullet = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(m_vectMemShots[i].first);
        assert(tagCompBullet);
        if(tagCompBullet->m_tagA == CollisionTag_e::BULLET_PLAYER_CT)
        {
            ShotConfComponent *shotConfComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(m_vectMemShots[i].first);
            assert(shotConfComp);
            if(tagCompTarget->m_tagA == CollisionTag_e::ENEMY_CT)
            {
                treatEnemyTakeDamage(m_vectMemShots[i].second, shotConfComp->m_damage);
            }
        }
        else if(tagCompBullet->m_tagA == CollisionTag_e::BULLET_ENEMY_CT)
        {
            ShotConfComponent *shotConfComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(m_vectMemShots[i].first);
            assert(shotConfComp);
            if(tagCompTarget->m_tagA == CollisionTag_e::PLAYER_CT)
            {
                treatPlayerTakeDamage(shotConfComp->m_damage);
            }
            else if(tagCompTarget->m_tagA == CollisionTag_e::VEHICULE_CT)
            {
                PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
                assert(playerComp);
                if(playerComp->m_associatedVehicle && playerComp->m_memEntityAssociated == m_vectMemShots[i].second)
                {
                    treatPlayerTakeDamage(shotConfComp->m_damage);
                }
            }
        }
    }
}

//===================================================================
void CollisionSystem::rmEnemyCollisionMaskEntity(uint32_t numEntity)
{
    GeneralCollisionComponent *tagComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(numEntity);
    assert(tagComp);
    tagComp->m_tagA = CollisionTag_e::DEAD_CORPSE_CT;
}

//===================================================================
void CollisionSystem::initArrayTag()
{
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::WALL_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::ELECTRIC_WALL_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::TRAVERSABLE_WALL_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::ENEMY_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::OBJECT_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::STATIC_SET_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::LOG_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::CHECKPOINT_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::BOSS_ZONE_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::EXIT_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::VEHICULE_CT});

    m_tagArray.insert({CollisionTag_e::VEHICULE_CT, CollisionTag_e::WALL_CT});
    m_tagArray.insert({CollisionTag_e::VEHICULE_CT, CollisionTag_e::TRAVERSABLE_WALL_CT});
    m_tagArray.insert({CollisionTag_e::VEHICULE_CT, CollisionTag_e::ELECTRIC_WALL_CT});
    m_tagArray.insert({CollisionTag_e::VEHICULE_CT, CollisionTag_e::ENEMY_CT});
    m_tagArray.insert({CollisionTag_e::VEHICULE_CT, CollisionTag_e::STATIC_SET_CT});

    m_tagArray.insert({CollisionTag_e::DETECT_MAP_CT, CollisionTag_e::WALL_CT});
    m_tagArray.insert({CollisionTag_e::DETECT_MAP_CT, CollisionTag_e::ELECTRIC_WALL_CT});
    m_tagArray.insert({CollisionTag_e::DETECT_MAP_CT, CollisionTag_e::TRAVERSABLE_WALL_CT});
    m_tagArray.insert({CollisionTag_e::DETECT_MAP_CT, CollisionTag_e::STATIC_SET_CT});
    m_tagArray.insert({CollisionTag_e::DETECT_MAP_CT, CollisionTag_e::EXIT_CT});
    m_tagArray.insert({CollisionTag_e::DETECT_MAP_CT, CollisionTag_e::LOG_CT});

    m_tagArray.insert({CollisionTag_e::PLAYER_ACTION_CT, CollisionTag_e::EXIT_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_ACTION_CT, CollisionTag_e::LOG_CT});
    m_tagArray.insert({CollisionTag_e::HIT_PLAYER_CT, CollisionTag_e::ENEMY_CT});

    m_tagArray.insert({CollisionTag_e::EXPLOSION_CT, CollisionTag_e::PLAYER_CT});
    m_tagArray.insert({CollisionTag_e::EXPLOSION_CT, CollisionTag_e::ENEMY_CT});

    m_tagArray.insert({CollisionTag_e::ENEMY_CT, CollisionTag_e::PLAYER_CT});
    // m_tagArray.insert({CollisionTag_e::ENEMY_CT, CollisionTag_e::VEHICULE_CT});
    m_tagArray.insert({CollisionTag_e::ENEMY_CT, CollisionTag_e::WALL_CT});
    m_tagArray.insert({CollisionTag_e::ENEMY_CT, CollisionTag_e::ELECTRIC_WALL_CT});
    m_tagArray.insert({CollisionTag_e::ENEMY_CT, CollisionTag_e::TRAVERSABLE_WALL_CT});
    m_tagArray.insert({CollisionTag_e::ENEMY_CT, CollisionTag_e::STATIC_SET_CT});

    m_tagArray.insert({CollisionTag_e::WALL_CT, CollisionTag_e::PLAYER_CT});
    m_tagArray.insert({CollisionTag_e::WALL_CT, CollisionTag_e::ENEMY_CT});
    m_tagArray.insert({CollisionTag_e::ELECTRIC_WALL_CT, CollisionTag_e::PLAYER_CT});
    m_tagArray.insert({CollisionTag_e::ELECTRIC_WALL_CT, CollisionTag_e::ENEMY_CT});

    m_tagArray.insert({CollisionTag_e::TRAVERSABLE_WALL_CT, CollisionTag_e::PLAYER_CT});
    m_tagArray.insert({CollisionTag_e::TRAVERSABLE_WALL_CT, CollisionTag_e::ENEMY_CT});

    m_tagArray.insert({CollisionTag_e::BULLET_ENEMY_CT, CollisionTag_e::PLAYER_CT});
    m_tagArray.insert({CollisionTag_e::BULLET_ENEMY_CT, CollisionTag_e::WALL_CT});
    m_tagArray.insert({CollisionTag_e::BULLET_ENEMY_CT, CollisionTag_e::ELECTRIC_WALL_CT});

    m_tagArray.insert({CollisionTag_e::BULLET_PLAYER_CT, CollisionTag_e::ENEMY_CT});
    m_tagArray.insert({CollisionTag_e::BULLET_PLAYER_CT, CollisionTag_e::WALL_CT});
    m_tagArray.insert({CollisionTag_e::BULLET_PLAYER_CT, CollisionTag_e::ELECTRIC_WALL_CT});


    m_tagArray.insert({CollisionTag_e::IMPACT_CT, CollisionTag_e::WALL_CT});
    m_tagArray.insert({CollisionTag_e::IMPACT_CT, CollisionTag_e::ENEMY_CT});
    m_tagArray.insert({CollisionTag_e::IMPACT_CT, CollisionTag_e::ELECTRIC_WALL_CT});


    m_tagArray.insert({CollisionTag_e::DEAD_CORPSE_CT, CollisionTag_e::WALL_CT});
    m_tagArray.insert({CollisionTag_e::DEAD_CORPSE_CT, CollisionTag_e::ELECTRIC_WALL_CT});
}

//===================================================================
bool CollisionSystem::checkTag(CollisionTag_e entityTagA, CollisionTag_e entityTagB)
{
    for(multiMapTagIt_t it = m_tagArray.find(entityTagA); it != m_tagArray.end() ; ++it)
    {
        if(it->first == entityTagA && it->second == entityTagB)
        {
            return true;
        }
    }
    return false;
}

//===================================================================
bool CollisionSystem::treatCollision(uint32_t entityNumA, uint32_t entityNumB, GeneralCollisionComponent &tagCompA,
                                     GeneralCollisionComponent &tagCompB)
{
    if(tagCompA.m_shape == CollisionShape_e::RECTANGLE_C)
    {
        MapCoordComponent *mapCompA = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNumA);
        MapCoordComponent *mapCompB = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNumB);
        assert(mapCompA);
        assert(mapCompB);
        CollisionArgs args = {entityNumA, entityNumB, tagCompA, tagCompB, *mapCompA, *mapCompB};
        checkCollisionFirstRect(args);
    }
    else if(tagCompA.m_shape == CollisionShape_e::CIRCLE_C)
    {
        MapCoordComponent *mapCompA = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNumA);
        MapCoordComponent *mapCompB = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNumB);
        assert(mapCompA);
        assert(mapCompB);
        CollisionArgs args = {entityNumA, entityNumB, tagCompA, tagCompB, *mapCompA, *mapCompB};
        return treatCollisionFirstCircle(args);
    }
    else if(tagCompA.m_shape == CollisionShape_e::SEGMENT_C)
    {
        MapCoordComponent *mapCompA = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNumA);
        MapCoordComponent *mapCompB = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNumB);
        assert(tagCompA.m_tagA == CollisionTag_e::BULLET_PLAYER_CT || tagCompA.m_tagA == CollisionTag_e::BULLET_ENEMY_CT);
        CollisionArgs args = {entityNumA, entityNumB, tagCompA, tagCompB, *mapCompA, *mapCompB};
        checkCollisionFirstSegment(args, entityNumA, entityNumB, tagCompB, *mapCompA);
    }
    return true;
}

//===================================================================
void CollisionSystem::checkCollisionFirstRect(CollisionArgs &args)
{
    MapDisplaySystem *mapSystem = Ecsm_t::instance().getSystem<MapDisplaySystem>(static_cast<uint32_t>(Systems_e::MAP_DISPLAY_SYSTEM));
    assert(mapSystem);
    if(args.tagCompA.m_tagA == CollisionTag_e::DETECT_MAP_CT && mapSystem->entityAlreadyDiscovered(args.entityNumB))
    {
        return;
    }
    RectangleCollisionComponent *rectCompA = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(args.entityNumA);
    assert(rectCompA);
    switch(args.tagCompB.m_shape)
    {
    case CollisionShape_e::RECTANGLE_C:
    {
        RectangleCollisionComponent *rectCompB = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(args.entityNumB);
        assert(rectCompB);
        PairFloat_t mapPosA = args.mapCompA.m_absoluteMapPositionPX;
        if(args.tagCompA.m_tagA == CollisionTag_e::VEHICULE_CT)
        {
            if(args.tagCompB.m_tagA == CollisionTag_e::WALL_CT || args.tagCompB.m_tagA == CollisionTag_e::TRAVERSABLE_WALL_CT || args.tagCompB.m_tagA == CollisionTag_e::ELECTRIC_WALL_CT)
            {
                VehicleComponent *vehicleCompA = Ecsm_t::instance().getComponent<VehicleComponent, Components_e::VEHICLE_COMPONENT>(args.entityNumA);
                assert(vehicleCompA);
                MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(args.entityNumA, 1);
                assert(mapComp);
                mapComp->m_absoluteMapPositionPX = {args.mapCompA.m_absoluteMapPositionPX.first, args.mapCompA.m_absoluteMapPositionPX.second + rectCompA->m_size.second / 3};
                //check if second shape touch a wall
                if(checkRectRectCollision(mapComp->m_absoluteMapPositionPX, rectCompA->m_size, args.mapCompB.m_absoluteMapPositionPX, rectCompB->m_size))
                {
                    vehicleCompA->m_onLateralGround = true;
                    vehicleCompA->m_touchGround = true;
                    if(vehicleCompA->m_onStair)
                    {
                        return;
                    }
                }
            }
        }
        if(!checkRectRectCollision(mapPosA, rectCompA->m_size, args.mapCompB.m_absoluteMapPositionPX, rectCompB->m_size))
        {
            return;
        }
        if(args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT && treatCollisionPlayer(args))
        {
            return;
        }
        if((args.tagCompA.m_tagA == CollisionTag_e::ENEMY_CT || args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT || args.tagCompA.m_tagA == CollisionTag_e::VEHICULE_CT))
        {
            if(!(args.tagCompA.m_tagA == CollisionTag_e::ENEMY_CT && (args.tagCompB.m_tagA == CollisionTag_e::PLAYER_CT || args.tagCompB.m_tagA == CollisionTag_e::VEHICULE_CT)))
            {
                if(args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT && args.tagCompB.m_tagA == CollisionTag_e::ENEMY_CT)
                {
                    EnemyConfComponent *enemyComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(args.entityNumB);
                    //if loop behaviour
                    if(static_cast<uint32_t>(enemyComp->m_type) > static_cast<uint32_t>(TypeEnemy_e::STATIC))
                    {
                        treatPlayerTakeDamage(*enemyComp->m_meleeAttackDamage);
                    }
                }
                else if(args.tagCompA.m_tagA == CollisionTag_e::VEHICULE_CT && args.tagCompB.m_tagA == CollisionTag_e::ENEMY_CT)
                {
                    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
                    if(!playerComp->m_associatedVehicle || *playerComp->m_associatedVehicle != args.entityNumA)
                    {
                        return;
                    }
                }
                if(args.tagCompB.m_tagA != CollisionTag_e::PLAYER_CT)
                {
                    collisionRectRectEject(args);
                }
            }
            if(args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT)
            {
                PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
                assert(playerComp);
                if(args.tagCompB.m_tagA == CollisionTag_e::ELECTRIC_WALL_CT)
                {
                    WallMultiSpriteComponent *wallMultiComp = Ecsm_t::instance().getComponent<WallMultiSpriteComponent, Components_e::WALL_MULTI_SPRITE_CONF_COMPONENT>(args.entityNumB);
                    assert(wallMultiComp);
                    treatPlayerTakeDamage(wallMultiComp->m_damage);
                }
            }
            else if(args.tagCompA.m_tagA == CollisionTag_e::VEHICULE_CT && args.tagCompB.m_tagA == CollisionTag_e::ENEMY_CT)
            {
                ShotConfComponent *shotConfComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(args.entityNumA);
                assert(shotConfComp);
                VehicleComponent *vehicleComp = Ecsm_t::instance().getComponent<VehicleComponent, Components_e::VEHICLE_COMPONENT>(args.entityNumA);
                assert(vehicleComp);
                if(vehicleComp->m_vehicleMemPlayerAssociated)
                {
                    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
                    assert(playerComp);
                    if(playerComp->m_inMovement)
                    {
                        treatEnemyTakeDamage(args.entityNumB, vehicleComp->m_damageColl, vehicleComp->m_minHealthDamage);
                    }
                }
            }
        }
    }
        break;
    case CollisionShape_e::CIRCLE_C:
    {
        CircleCollisionComponent *circleCompB = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(args.entityNumB);
        assert(circleCompB);
        if(!checkCircleRectCollision(args.mapCompB.m_absoluteMapPositionPX, circleCompB->m_ray,
                                     args.mapCompA.m_absoluteMapPositionPX, rectCompA->m_size))
        {
            return;
        }
        if(args.tagCompB.m_tagA == CollisionTag_e::OBJECT_CT)
        {
            treatPlayerPickObject(args);
        }
        else if(args.tagCompB.m_tagA == CollisionTag_e::EXIT_CT)
        {
            m_refMainEngine->activeEndLevel();
        }
        else if(args.tagCompB.m_tagA == CollisionTag_e::LOG_CT)
        {
            LogComponent *logComp = Ecsm_t::instance().getComponent<LogComponent, Components_e::LOG_COMPONENT>(args.entityNumB);
            assert(logComp);
            if(!logComp->m_activated)
            {
                PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
                assert(playerComp);
                playerComp->m_lockLogRelease = true;
                logComp->m_activated = true;
                playerComp->m_infoWriteData = {true, {logComp->m_message, args.entityNumB}};
                TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(playerComp->m_memEntityAssociated);
                assert(timerComp);
                timerComp->m_cycleCountA = 0;
                timerComp->m_timeIntervalOptional = 4.0 / FPS_VALUE;
                Level::setDialogMode(true);
            }
        }
    }
        break;
    case CollisionShape_e::SEGMENT_C:
        break;
    case CollisionShape_e::TRIANGLE_STAIR_DOWN:
    {
        TriangleStairCollisionComponent *triangleCompB = Ecsm_t::instance().getComponent<TriangleStairCollisionComponent, Components_e::TRIANGLE_STAIR_COLLISION_COMPONENT>(args.entityNumB);
        assert(triangleCompB);
        if(checkRectRectCollision(args.mapCompA.m_absoluteMapPositionPX, rectCompA->m_size, args.mapCompB.m_absoluteMapPositionPX, triangleCompB->m_size))
        {
            if(args.tagCompA.m_tagA == CollisionTag_e::ENEMY_CT || args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT || args.tagCompA.m_tagA == CollisionTag_e::VEHICULE_CT)
            {
                collisionRectTriangleEject(args, true);
            }
        }
    }
    break;
    case CollisionShape_e::TRIANGLE_STAIR_UP:
    {
        TriangleStairCollisionComponent *triangleCompB = Ecsm_t::instance().getComponent<TriangleStairCollisionComponent, Components_e::TRIANGLE_STAIR_COLLISION_COMPONENT>(args.entityNumB);
        assert(triangleCompB);
        if(checkRectRectCollision(args.mapCompA.m_absoluteMapPositionPX, rectCompA->m_size, args.mapCompB.m_absoluteMapPositionPX, triangleCompB->m_size))
        {
            if(args.tagCompA.m_tagA == CollisionTag_e::ENEMY_CT || args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT || args.tagCompA.m_tagA == CollisionTag_e::VEHICULE_CT)
            {
                collisionRectTriangleEject(args, false);
            }
        }
    }
    break;
    }
}

//===================================================================
void CollisionSystem::writePlayerInfo(const std::string &info)
{
    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(playerComp->m_memEntityAssociated);
    assert(playerComp);
    assert(timerComp);
    timerComp->m_cycleCountA = 0;
    playerComp->m_infoWriteData = {true, {info, 0}};
}

//===================================================================
bool CollisionSystem::treatCollisionFirstCircle(CollisionArgs &args)
{
    if(args.tagCompA.m_tagA == CollisionTag_e::PLAYER_ACTION_CT || args.tagCompA.m_tagA == CollisionTag_e::HIT_PLAYER_CT)
    {
        args.tagCompA.m_active = false;
    }
    CircleCollisionComponent *circleCompA = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(args.entityNumA);
    assert(circleCompA);
    bool collision = false;
    switch(args.tagCompB.m_shape)
    {
    case CollisionShape_e::RECTANGLE_C:
    {
        RectangleCollisionComponent *rectCompB = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(args.entityNumB);
        assert(rectCompB);
        collision = checkCircleRectCollision(args.mapCompA.m_absoluteMapPositionPX, circleCompA->m_ray,
                                             args.mapCompB.m_absoluteMapPositionPX, rectCompB->m_size);
        if(collision)
        {
            //GRENADE
            if(args.tagCompA.m_tagA == CollisionTag_e::BULLET_PLAYER_CT)
            {
                ShotConfComponent *shotConfComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(args.entityNumA);
                assert(shotConfComp);
                if(!shotConfComp->m_damageCircleRayData)
                {
                    if(args.tagCompB.m_tagA == CollisionTag_e::ENEMY_CT)
                    {
                        treatEnemyTakeDamage(args.entityNumB, shotConfComp->m_damage);
                    }
                }
            }
            if(args.tagCompA.m_tagA == CollisionTag_e::ENEMY_CT)
            {
                bool checkStuck = (args.tagCompB.m_tagA == CollisionTag_e::CHECKPOINT_CT ||
                                   args.tagCompB.m_tagA == CollisionTag_e::LOG_CT || args.tagCompB.m_tagA == CollisionTag_e::STATIC_SET_CT);
                PairFloat_t previousPos;
                if(checkStuck)
                {
                    previousPos = args.mapCompA.m_absoluteMapPositionPX;
                }
                collisionCircleRectEject(args, circleCompA->m_ray, *rectCompB);
            }
            else if(args.tagCompA.m_tagA == CollisionTag_e::HIT_PLAYER_CT)
            {
                ShotConfComponent *shotConfComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(args.entityNumA);
                assert(shotConfComp);
                if(args.tagCompB.m_tagA == CollisionTag_e::ENEMY_CT)
                {
                    activeSound(args.entityNumA);
                    treatEnemyTakeDamage(args.entityNumB, shotConfComp->m_damage);
                    return false;
                }
            }
            else if(args.tagCompA.m_tagA == CollisionTag_e::IMPACT_CT)
            {
                collisionCircleRectEject(args, circleCompA->m_ray, *rectCompB);
            }
        }
    }
        break;
    case CollisionShape_e::CIRCLE_C:
    {
        CircleCollisionComponent *circleCompB = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(args.entityNumB);
        assert(circleCompB);
        collision = checkCircleCircleCollision(args.mapCompA.m_absoluteMapPositionPX, circleCompA->m_ray,
                                               args.mapCompB.m_absoluteMapPositionPX, circleCompB->m_ray);
        if(collision)
        {
            if(args.tagCompA.m_tagA == CollisionTag_e::HIT_PLAYER_CT)
            {
                ShotConfComponent *shotConfComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(args.entityNumA);
                assert(shotConfComp);
                if(args.tagCompB.m_tagA == CollisionTag_e::ENEMY_CT)
                {
                    activeSound(args.entityNumA);
                    treatEnemyTakeDamage(args.entityNumB, shotConfComp->m_damage);
                    return false;
                }
            }
            else if(args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT)
            {
                if(args.tagCompB.m_tagA == CollisionTag_e::OBJECT_CT)
                {
                    treatPlayerPickObject(args);
                }
            }
            if((args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT || args.tagCompA.m_tagA == CollisionTag_e::ENEMY_CT ||
                     args.tagCompA.m_tagA == CollisionTag_e::IMPACT_CT) &&
                    (args.tagCompB.m_tagA == CollisionTag_e::LOG_CT || args.tagCompB.m_tagA == CollisionTag_e::WALL_CT ||
                     args.tagCompB.m_tagA == CollisionTag_e::PLAYER_CT ||
                     args.tagCompB.m_tagA == CollisionTag_e::ENEMY_CT || args.tagCompB.m_tagA == CollisionTag_e::STATIC_SET_CT))
            {
                if(args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT &&
                        args.tagCompB.m_tagA == CollisionTag_e::ENEMY_CT &&
                        circleCompB->m_ray > 10.0f)
                {
                    if(m_memCrush.empty())
                    {
                        collisionCircleCircleEject(args, *circleCompA, *circleCompB);
                    }
                }
                else
                {
                    collisionCircleCircleEject(args, *circleCompA, *circleCompB);
                }
            }
        }
    }
        break;
    case CollisionShape_e::TRIANGLE_STAIR_DOWN:
    case CollisionShape_e::TRIANGLE_STAIR_UP:
    case CollisionShape_e::SEGMENT_C:
    {
    }
        break;
    }
    //TREAT VISIBLE SHOT
    if((args.tagCompA.m_tagA == CollisionTag_e::BULLET_ENEMY_CT) ||
        (args.tagCompA.m_tagA == CollisionTag_e::BULLET_PLAYER_CT))
    {
        treatVisibleShot(args, collision);
    }
    return true;
}

//===================================================================
void CollisionSystem::treatVisibleShot(CollisionArgs &args, bool collision)
{
    ShotConfComponent *shotConfComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(args.entityNumA);
    assert(shotConfComp);
    if(shotConfComp->m_destructPhase)
    {
        return;
    }
    bool limitX = (args.mapCompA.m_absoluteMapPositionPX.first <= 0.0f),
        limitY = (args.mapCompA.m_absoluteMapPositionPX.second <= 0.0f);
    bool limit = false;
    PairUI_t levelSize = Level::getSize();
    float maxLimitX = levelSize.first * LEVEL_TILE_SIZE_PX - LEVEL_TWO_THIRD_TILE_SIZE_PX,
            maxLimitY = levelSize.second * LEVEL_TILE_SIZE_PX - LEVEL_TWO_THIRD_TILE_SIZE_PX;


    //limit level case
    if(args.mapCompA.m_absoluteMapPositionPX.first >= maxLimitX)
    {
        args.mapCompA.m_absoluteMapPositionPX.first = maxLimitX;
        limit = true;
    }
    else if(limitX)
    {
        args.mapCompA.m_absoluteMapPositionPX.first = 0.0f;
        limit = true;
    }
    //limit case
    if(args.mapCompA.m_absoluteMapPositionPX.second >= maxLimitY)
    {
        args.mapCompA.m_absoluteMapPositionPX.second = maxLimitY;
        limit = true;
    }
    else if(limitY)
    {
        args.mapCompA.m_absoluteMapPositionPX.second = 0.0f;
        limit = true;
    }
    if(limit)
    {
        shotConfComp->m_destructPhase = true;
        return;
    }

    if(shotConfComp->m_damageCircleRayData)
    {
        setDamageCircle(*shotConfComp->m_damageCircleRayData, true, args.entityNumA);
    }
    if(collision)
    {
        if(shotConfComp->m_damageCircleRayData)
        {
            setDamageCircle(*shotConfComp->m_damageCircleRayData, true, args.entityNumA);
        }
        activeSound(args.entityNumA);
        shotConfComp->m_destructPhase = true;
        shotConfComp->m_spriteShotNum = 0;
        GravityComponent *gravComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(args.entityNumA);
        if(gravComp)
        {
            gravComp->m_freeze = true;
        }
    }
}

//===================================================================
bool CollisionSystem::treatCollisionPlayer(CollisionArgs &args)
{
   if(args.tagCompB.m_tagA == CollisionTag_e::CHECKPOINT_CT)
    {
        PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
        CheckpointComponent *checkpointComp = Ecsm_t::instance().getComponent<CheckpointComponent, Components_e::CHECKPOINT_COMPONENT>(args.entityNumB);
        assert(playerComp);
        assert(checkpointComp);
        if(!playerComp->m_currentCheckpoint || checkpointComp->m_checkpointNumber > playerComp->m_currentCheckpoint->first)
        {
            MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(args.entityNumB);
            assert(mapComp);
            playerComp->m_checkpointReached = mapComp->m_coord;
            playerComp->m_currentCheckpoint = {checkpointComp->m_checkpointNumber, checkpointComp->m_direction};
            writePlayerInfo("Checkpoint Reached");
        }
        m_vectEntitiesToDelete.push_back(args.entityNumB);
        return true;
    }
    else if(args.tagCompB.m_tagA == CollisionTag_e::BOSS_ZONE_CT)
    {
        PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
        assert(playerComp);
        Level::setScrollingLock(true);
        m_refMainEngine->playBossMusic();
        writePlayerInfo("Warning");
        m_vectEntitiesToDelete.push_back(args.entityNumB);
        return true;
    }
    else
    {
        return treatCollisionVehiclePlayer(args.entityNumB);
    }
}

//===================================================================
bool CollisionSystem::treatCollisionVehiclePlayer(uint32_t vehicleEntity)
{
    GeneralCollisionComponent *vehicleColl = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(vehicleEntity);
    if(vehicleColl->m_tagA == CollisionTag_e::VEHICULE_CT)
    {
        GravityComponent *gravComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(m_playerEntity);
        assert(gravComp);
        if(gravComp->m_exitVehicle)
        {
            PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
            assert(playerComp);
            GravityComponent *vehicleGravComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(vehicleEntity);
            assert(vehicleGravComp);
            VehicleComponent *vehicleComp = Ecsm_t::instance().getComponent<VehicleComponent, Components_e::VEHICLE_COMPONENT>(vehicleEntity);
            assert(vehicleComp);
            if(vehicleComp->m_currentShootAnimation)
            {
                m_refMainEngine->updateExitVehicleSprites(vehicleEntity, *vehicleComp);
            }
            playerComp->m_associatedVehicle = std::nullopt;
            vehicleComp->m_vehicleMemPlayerAssociated = false;
            vehicleGravComp->m_freeze = false;
            gravComp->m_exitVehicle = false;
            playerComp->m_vehicleEject = true;
        }
        else if(gravComp->m_jump)
        {
            PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
            assert(playerComp);
            if(!playerComp->m_vehicleEject && !playerComp->m_associatedVehicle)
            {
                VehicleComponent *vehicleComp = Ecsm_t::instance().getComponent<VehicleComponent, Components_e::VEHICLE_COMPONENT>(vehicleEntity);
                assert(vehicleComp);
                GravityComponent *vehicleGravComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(vehicleEntity);
                assert(vehicleGravComp);
                ShotConfComponent *shotComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(vehicleEntity);
                assert(shotComp);
                playerComp->m_associatedVehicle = vehicleEntity;
                vehicleComp->m_vehicleMemPlayerAssociated = true;
                //Stop jumping if enter vehicle
                gravComp->m_jump = false;
                playerComp->m_vehicleEject = true;
            }
        }
        return true;
    }
    return false;
}

//===================================================================
void CollisionSystem::setDamageCircle(uint32_t damageEntity, bool active, uint32_t baseEntity)
{
    GeneralCollisionComponent *genDam = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(damageEntity);
    assert(genDam);
    genDam->m_active = active;
    if(active)
    {
        MapCoordComponent *mapCompDam = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(damageEntity);
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(baseEntity);
        assert(mapCompDam);
        assert(mapComp);
        mapCompDam->m_absoluteMapPositionPX = mapComp->m_absoluteMapPositionPX;
        std::optional<PairUI_t> coord = getLevelCoord(mapCompDam->m_absoluteMapPositionPX);
        assert(coord);
        addEntityToZone(damageEntity, *getLevelCoord(mapCompDam->m_absoluteMapPositionPX));
    }
    else
    {
        removeEntityToZone(damageEntity);
    }
}

//===================================================================
void CollisionSystem::activeSound(uint32_t entityNum)
{
    AudioComponent *audioComp = Ecsm_t::instance().getComponent<AudioComponent, Components_e::AUDIO_COMPONENT>(entityNum);
    if(audioComp)
    {
        audioComp->m_soundElements[0]->m_toPlay = true;
    }
}

//===================================================================
void CollisionSystem::treatPlayerPickObject(CollisionArgs &args)
{
    ObjectConfComponent *objectComp = Ecsm_t::instance().getComponent<ObjectConfComponent, Components_e::OBJECT_CONF_COMPONENT>(args.entityNumB);
    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    WeaponComponent *weaponComp = Ecsm_t::instance().getComponent<WeaponComponent, Components_e::WEAPON_COMPONENT>(playerComp->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::WEAPON)]);
    assert(objectComp);
    assert(playerComp);
    assert(weaponComp);
    std::string info;
    switch (objectComp->m_type)
    {
    case ObjectType_e::AMMO_WEAPON:
    {
        TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(m_playerEntity);
        assert(timerComp);
        if(!pickUpAmmo(*objectComp->m_weaponID, *weaponComp, *timerComp, objectComp->m_containing))
        {
            return;
        }
        info = weaponComp->m_weaponsData[*objectComp->m_weaponID].m_weaponName + " Ammo";
    }
        break;
    case ObjectType_e::WEAPON:
    {
        TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(m_playerEntity);
        assert(timerComp);
        if(!pickUpWeapon(*objectComp->m_weaponID, *weaponComp, *timerComp, objectComp->m_containing))
        {
            return;
        }
        info = weaponComp->m_weaponsData[*objectComp->m_weaponID].m_weaponName;
    }
        break;
    case ObjectType_e::HEAL:
    {
        if(playerComp->m_life == 100)
        {
            return;
        }
        playerComp->m_life += objectComp->m_containing;
        if(playerComp->m_life > 100)
        {
            playerComp->m_life = 100;
        }
        info = "Heal";
        break;
    }
    case ObjectType_e::CARD:
    {
        playerComp->m_card.insert(*objectComp->m_cardID);
        info = objectComp->m_cardName;
        break;
    }
    case ObjectType_e::GRENADE:
    {
        info = "Grenade";
        if(weaponComp->m_grenadeData.m_ammunationsCount < weaponComp->m_grenadeData.m_maxAmmunations)
        {
            ++weaponComp->m_grenadeData.m_ammunationsCount;
        }
        break;
    }
    case ObjectType_e::TOTAL:
        assert(false);
        break;
    }
    removeEntityToZone(args.entityNumB);
    playerComp->m_infoWriteData = {true, {info, 0}};
    TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(playerComp->m_memEntityAssociated);
    assert(timerComp);
    timerComp->m_cycleCountA = 0;
    playerComp->m_pickItem = true;
    activeSound(args.entityNumA);
    m_vectEntitiesToDelete.push_back(args.entityNumB);
}

//===================================================================
void CollisionSystem::treatCrushing(uint32_t entityNum)
{
    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    if(playerComp)
    {
        playerComp->m_crush = true;
        playerComp->m_frozen = true;
    }
    else
    {        
        //check if component exist
        if(Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(entityNum))
        {
            treatEnemyTakeDamage(entityNum, 1);
        }
    }
}

//===================================================================
bool pickUpAmmo(uint32_t numWeapon, WeaponComponent &weaponComp, TimerComponent &timerComp, uint32_t objectContaining)
{
    WeaponData &objectWeapon = weaponComp.m_weaponsData[numWeapon];
    if(objectWeapon.m_ammunationsCount == objectWeapon.m_maxAmmunations)
    {
        return false;
    }
    if(objectWeapon.m_posses && objectWeapon.m_ammunationsCount == 0)
    {
        if(weaponComp.m_currentWeapon < numWeapon)
        {
            setPlayerWeapon(weaponComp, timerComp, numWeapon);
        }
    }
    objectWeapon.m_ammunationsCount += objectContaining;
    if(objectWeapon.m_ammunationsCount > objectWeapon.m_maxAmmunations)
    {
        objectWeapon.m_ammunationsCount = objectWeapon.m_maxAmmunations;
    }
    return true;
}

//===================================================================
bool pickUpWeapon(uint32_t numWeapon, WeaponComponent &weaponComp, TimerComponent &timerComp, uint32_t objectContaining)
{
    WeaponData &objectWeapon = weaponComp.m_weaponsData[numWeapon];
    if(objectWeapon.m_posses &&
            objectWeapon.m_ammunationsCount == objectWeapon.m_maxAmmunations)
    {
        return false;
    }
    if(!objectWeapon.m_posses)
    {
        objectWeapon.m_posses = true;
        if(weaponComp.m_currentWeapon < numWeapon)
        {
            setPlayerWeapon(weaponComp, timerComp, numWeapon);
        }
    }
    objectWeapon.m_ammunationsCount += objectContaining;
    if(objectWeapon.m_ammunationsCount > objectWeapon.m_maxAmmunations)
    {
        objectWeapon.m_ammunationsCount = objectWeapon.m_maxAmmunations;
    }
    return true;
}

//===================================================================
bool CollisionSystem::checkCollisionFirstSegment(CollisionArgs &args, uint32_t numEntityA, uint32_t numEntityB, GeneralCollisionComponent &tagCompB, MapCoordComponent &mapCompB)
{
    bool collision = false;
    SegmentCollisionComponent *segmentCompA = Ecsm_t::instance().getComponent<SegmentCollisionComponent, Components_e::SEGMENT_COLLISION_COMPONENT>(numEntityA);
    assert(segmentCompA);
    switch(tagCompB.m_shape)
    {
    case CollisionShape_e::TRIANGLE_STAIR_DOWN:
    case CollisionShape_e::TRIANGLE_STAIR_UP:
    {
        TriangleStairCollisionComponent *triangleComp = Ecsm_t::instance().getComponent<TriangleStairCollisionComponent, Components_e::TRIANGLE_STAIR_COLLISION_COMPONENT>(numEntityB);
        assert(triangleComp);
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(numEntityB);
        assert(mapComp);
        SegmentCollisionComponent *segmentComp = Ecsm_t::instance().getComponent<SegmentCollisionComponent, Components_e::SEGMENT_COLLISION_COMPONENT>(numEntityA);
        assert(segmentComp);
        if(checkSegmentRectCollision(segmentComp->m_points.first, segmentComp->m_points.second, mapComp->m_absoluteMapPositionPX, triangleComp->m_size))
        {
            destroyShot(numEntityA);
            collision = true;
        }
    }
    break;
    case CollisionShape_e::SEGMENT_C:
    {
    }
        break;
    case CollisionShape_e::CIRCLE_C:
    {
        CircleCollisionComponent *circleCompB = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(numEntityB);
        assert(circleCompB);
        if(checkCircleSegmentCollision(mapCompB.m_absoluteMapPositionPX, circleCompB->m_ray, segmentCompA->m_points.first, segmentCompA->m_points.second))
        {
            destroyShot(numEntityA);
            collision = true;
        }
    }
        break;
    case CollisionShape_e::RECTANGLE_C:
    {
        SegmentCollisionComponent *segmentComp = Ecsm_t::instance().getComponent<SegmentCollisionComponent, Components_e::SEGMENT_COLLISION_COMPONENT>(numEntityA);
        assert(segmentComp);
        collision = treatSegmentRectColl(args, numEntityA, numEntityB, segmentComp);
        if(!collision)
        {
            SegmentCollisionComponent *segmentCompB = Ecsm_t::instance().getComponent<SegmentCollisionComponent, Components_e::SEGMENT_COLLISION_COMPONENT>(numEntityA, 1);
            if(segmentCompB)
            {
                collision = treatSegmentRectColl(args, numEntityA, numEntityB, segmentCompB);
            }
        }
    }
    break;
    }
    if((args.tagCompA.m_tagA == CollisionTag_e::BULLET_ENEMY_CT) ||
        (args.tagCompA.m_tagA == CollisionTag_e::BULLET_PLAYER_CT))
    {
        treatVisibleShot(args, collision);
    }
    return false;
}

//===================================================================
bool CollisionSystem::treatSegmentRectColl(CollisionArgs &args, uint32_t numEntityA, uint32_t numEntityB, SegmentCollisionComponent *segmentComp)
{
    RectangleCollisionComponent *rectComp = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(numEntityB);
    assert(rectComp);
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(numEntityB);
    assert(mapComp);
    if(checkSegmentRectCollision(segmentComp->m_points.first, segmentComp->m_points.second, mapComp->m_absoluteMapPositionPX, rectComp->m_size))
    {
        GeneralCollisionComponent *collComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(numEntityA);
        assert(collComp);
        if(collComp->m_tagA == CollisionTag_e::BULLET_ENEMY_CT || collComp->m_tagA == CollisionTag_e::BULLET_PLAYER_CT)
        {
            ShotConfComponent *shotComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(numEntityA);
            assert(shotComp);
            if(args.tagCompB.m_tagA == CollisionTag_e::PLAYER_CT)
            {
                treatPlayerTakeDamage(shotComp->m_damage);
            }
            else if(args.tagCompB.m_tagA == CollisionTag_e::ENEMY_CT)
            {
                EnemyConfComponent *enemyComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(numEntityB);
                assert(enemyComp);
                treatEnemyTakeDamage(args.entityNumB, shotComp->m_damage);
                enemyComp->takeDamage(shotComp->m_damage);
            }
        }
        return true;
    }
    return false;
}

//===================================================================
void destroyShot(uint32_t entity)
{
    GeneralCollisionComponent *collComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(entity);
    assert(collComp);
    if(collComp->m_tagA == CollisionTag_e::BULLET_ENEMY_CT || collComp->m_tagA == CollisionTag_e::BULLET_PLAYER_CT)
    {
        ShotConfComponent *shotComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(entity);
        assert(shotComp);
        shotComp->m_destructPhase = true;
    }
}

//===================================================================
void CollisionSystem::collisionCircleRectEject(CollisionArgs &args, float circleRay,
                                               const RectangleCollisionComponent &rectCollB, bool visibleShotFirstEject)
{
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(args.entityNumA);
    MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(args.entityNumA);
    assert(mapComp);
    assert(moveComp);
    float radiantEjectedAngle = getRadiantAngle(moveComp->m_currentDegreeMoveDirection);
    float circlePosX = args.mapCompA.m_absoluteMapPositionPX.first;
    float circlePosY = args.mapCompA.m_absoluteMapPositionPX.second;
    float elementPosX = args.mapCompB.m_absoluteMapPositionPX.first;
    float elementPosY = args.mapCompB.m_absoluteMapPositionPX.second;
    float elementSecondPosX = elementPosX + rectCollB.m_size.first;
    float elementSecondPosY = elementPosY + rectCollB.m_size.second;
    bool angleBehavior = false, limitEjectY = false, limitEjectX = false, crushMode = false;
    //collision on angle of rect
    if((circlePosX < elementPosX || circlePosX > elementSecondPosX) &&
            (circlePosY < elementPosY || circlePosY > elementSecondPosY))
    {
        angleBehavior = true;
    }
    float pointElementX = (circlePosX < elementPosX) ? elementPosX : elementSecondPosX;
    float pointElementY = (circlePosY < elementPosY) ? elementPosY : elementSecondPosY;
    float diffY, diffX = EPSILON_FLOAT;
    bool visibleShot = (args.tagCompA.m_tagA == CollisionTag_e::BULLET_ENEMY_CT || args.tagCompA.m_tagA == CollisionTag_e::BULLET_PLAYER_CT);
    diffY = getVerticalCircleRectEject({circlePosX, circlePosY, pointElementX, elementPosY,
                                        elementSecondPosY, circleRay, radiantEjectedAngle, angleBehavior}, limitEjectY, visibleShot);
    diffX = getHorizontalCircleRectEject({circlePosX, circlePosY, pointElementY, elementPosX, elementSecondPosX,
                                          circleRay, radiantEjectedAngle, angleBehavior}, limitEjectX, visibleShot);
    if(!visibleShotFirstEject && args.tagCompA.m_tagA != CollisionTag_e::PLAYER_CT &&
            std::min(std::abs(diffX), std::abs(diffY)) > LEVEL_THIRD_TILE_SIZE_PX)
    {
        return;
    }
    if(args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT || args.tagCompA.m_tagA == CollisionTag_e::ENEMY_CT)
    {
        crushMode = args.tagCompB.m_tagA == CollisionTag_e::WALL_CT;
    }
    //if player touch ground
    if(args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT || args.tagCompA.m_tagA == CollisionTag_e::ENEMY_CT)
    {
        GravityComponent *gravityComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(args.entityNumA);
        assert(gravityComp);
        if(diffY < 0)
        {
            gravityComp->m_onGround = true;
            gravityComp->m_memOnGround = true;
            gravityComp->m_jump = false;
            if(gravityComp->m_fall)
            {
                //cancel gravity
                mapComp->m_absoluteMapPositionPX.second -= gravityComp->m_gravityCohef;
                gravityComp->m_fall = false;
                diffY = std::numeric_limits<float>::epsilon();
            }
        }
        else
        {
            gravityComp->m_memOnGround = false;
        }
    }
    collisionEject(*mapComp, diffX, diffY, crushMode);
    addEntityToZone(args.entityNumA, *getLevelCoord(mapComp->m_absoluteMapPositionPX));
}

//===================================================================
void CollisionSystem::collisionRectRectEject(CollisionArgs &args)
{
    bool lockY = false;
    PlayerConfComponent *playerComp = nullptr;
    if(args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT)
    {
        playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(args.entityNumA);
        if(playerComp->m_associatedVehicle)
        {
            return;
        }
        if(args.tagCompB.m_tagA == CollisionTag_e::ENEMY_CT)
        {
            lockY = true;
        }
    }
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(args.entityNumA);
    RectangleCollisionComponent *rectCollA = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(args.entityNumA);
    RectangleCollisionComponent *rectCollB = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(args.entityNumB);
    assert(rectCollA);
    assert(rectCollB);
    assert(mapComp);
    float elementAPosX = args.mapCompA.m_absoluteMapPositionPX.first;
    float elementAPosY = args.mapCompA.m_absoluteMapPositionPX.second;
    float elementASecondPosX = elementAPosX + rectCollA->m_size.first;
    float elementASecondPosY = elementAPosY + rectCollA->m_size.second;
    float elementBPosX = args.mapCompB.m_absoluteMapPositionPX.first;
    float elementBPosY = args.mapCompB.m_absoluteMapPositionPX.second;
    float elementBSecondPosX = elementBPosX + rectCollB->m_size.first;
    float elementBSecondPosY = elementBPosY + rectCollB->m_size.second;
    bool limitEjectY = false, limitEjectX = false, crushMode = false;
    float diffY = 10000, diffX = EPSILON_FLOAT;
    if(!lockY)
    {
        //eject Y
        diffY = getRectRectEject({elementAPosX, elementAPosY, elementASecondPosY,
                                  elementBPosX, elementBPosY, elementBSecondPosY}, limitEjectY);
    }
    //eject X
    diffX = getRectRectEject({elementAPosY, elementAPosX, elementASecondPosX,
                              elementBPosY, elementBPosX, elementBSecondPosX}, limitEjectY);

    MoveableWallConfComponent *moveB = Ecsm_t::instance().getComponent<MoveableWallConfComponent, Components_e::MOVEABLE_WALL_CONF_COMPONENT>(args.entityNumB);
    //if moveable wall and go up
    if(moveB)
    {
        m_memMoveableWallCrush = true;
        if(moveB->m_directionMove[moveB->m_currentPhase].first == Direction_e::NORTH)
        {
            MoveableComponent *moveableB = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(args.entityNumB);
            if(std::abs(diffX) < std::abs(diffY) && std::abs(diffY) == moveableB->m_velocity)
            {
                return;
            }
        }
    }
    if(args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT || args.tagCompA.m_tagA == CollisionTag_e::ENEMY_CT)
    {
        crushMode = args.tagCompB.m_tagA == CollisionTag_e::WALL_CT;
    }
    //if player touch ground
    if(args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT || args.tagCompA.m_tagA == CollisionTag_e::VEHICULE_CT)
    {
        GravityComponent *gravityComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(args.entityNumA);
        assert(gravityComp);
        GeneralCollisionComponent *CollCompB = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(args.entityNumB);
        assert(CollCompB);
        if(CollCompB->m_wallTraversable)
        {
            if(elementASecondPosY < elementBSecondPosY)
            {
                m_memPlayerJumpDown = true;
            }
            if(m_playerJumpDown || diffY >= 0 || std::abs(diffY) > LEVEL_TILE_SIZE_PX)
            {
                // m_memPlayerJumpDown = true;
                return;
            }
        }
        //if y change is lower than Y
        bool YChange = (std::abs(diffY) < std::abs(diffX));
        if(YChange && diffY < 0)
        {
            gravityComp->m_onGround = true;
            gravityComp->m_memOnGround = true;
            if(std::abs(diffY) > std::abs(diffX))
            {
                gravityComp->m_fall = true;
                gravityComp->m_onGround = false;
                gravityComp->m_memOnGround = false;
            }
            if(gravityComp->m_onGround)
            {
                if(args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT)
                {
                    m_refMainEngine->memPlayerCurrentWallOnGround(args.entityNumB);
                }
                else
                {
                    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
                    if(playerComp->m_associatedVehicle && args.entityNumA == *playerComp->m_associatedVehicle)
                    {
                        m_refMainEngine->memPlayerCurrentWallOnGround(args.entityNumB);
                    }
                }
            }
        }
        else
        {
            if(gravityComp->m_memOnGround)
            {
                gravityComp->m_fall = false;
            }
            else
            {
                gravityComp->m_memOnGround = false;
                gravityComp->m_onGround = false;
                gravityComp->m_fall = true;
            }
        }
    }
    else if(args.tagCompA.m_tagA == CollisionTag_e::ENEMY_CT)
    {
        GravityComponent *gravityComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(args.entityNumA);
        if(gravityComp)
        {
            if(diffY < 0)
            {
                gravityComp->m_onGround = true;
                gravityComp->m_memOnGround = true;
                gravityComp->m_jump = false;
                if(gravityComp->m_fall)
                {
                    //cancel gravity
                    mapComp->m_absoluteMapPositionPX.second -= gravityComp->m_gravityCohef;
                    gravityComp->m_fall = false;
                    diffY = std::numeric_limits<float>::epsilon();
                }
            }
            else
            {
                gravityComp->m_memOnGround = false;
            }
        }
        if(std::abs(diffX) < std::abs(diffY))
        {
            EnemyConfComponent *enemyComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(args.entityNumA);
            if(enemyComp->m_type == TypeEnemy_e::LOOP_GROUND_HORIZONTAL_LEFT || enemyComp->m_type == TypeEnemy_e::LOOP_GROUND_HORIZONTAL_RIGHT
                || enemyComp->m_type == TypeEnemy_e::LOOP_HORIZONTAL || enemyComp->m_type == TypeEnemy_e::LOOP_WAVE_LEFT ||
                enemyComp->m_type == TypeEnemy_e::LOOP_WAVE_RIGHT)
            {
                bool left = (diffX < 0.0f);
                MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(args.entityNumA);
                moveComp->m_degreeOrientation = left ? 180.0f : 0.0f;
                enemyComp->m_attackPhase = left ? EnemyAttackPhase_e::MOVE_TO_TARGET_LEFT : EnemyAttackPhase_e::MOVE_TO_TARGET_RIGHT;
            }
        }
        else
        {
            EnemyConfComponent *enemyComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(args.entityNumA);
            if(enemyComp->m_type == TypeEnemy_e::LOOP_VERTICAL)
            {
                bool up = (diffY < 0.0f);
                MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(args.entityNumA);
                moveComp->m_degreeOrientation = up ? 90.0f : 270.0f;
            }
        }
    }
    if(crushMode)
    {
        m_mapMemCrushPos.insert({args.entityNumA, mapComp->m_absoluteMapPositionPX});
    }
    collisionEject(*mapComp, diffX, diffY, crushMode);
    if(m_memMoveableWallCrush && crushMode)
    {
        treatCrushCase(mapComp, playerComp, {diffX, diffY}, args.entityNumA);
    }
    addEntityToZone(args.entityNumA, *getLevelCoord(mapComp->m_absoluteMapPositionPX));
}

//===================================================================
void CollisionSystem::treatCrushCase(MapCoordComponent *mapComp, PlayerConfComponent * playerComp, const PairFloat_t &pairDiff, uint32_t entity)
{
    std::get<2>(m_memCrush.back()) = (pairDiff.second > 0.0f) ? Direction_e::SOUTH : Direction_e::NORTH;
    Direction_e dirA = std::get<2>(m_memCrush.back()), dirB;
    for(uint32_t i = 0; i < m_memCrush.size() - 1; ++i)
    {
        dirB = std::get<2>(m_memCrush[i]);
        //if 2 wall crush reset init pos
        if((dirA == Direction_e::NORTH && dirB == Direction_e::SOUTH) || (dirB == Direction_e::NORTH && dirA == Direction_e::SOUTH))
        {
            mapComp->m_absoluteMapPositionPX.second = m_mapMemCrushPos[entity].second;
            if(playerComp)
            {
                playerComp->m_frozen = true;
                GravityComponent *gravComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(entity);
                gravComp->m_jump = false;
                playerComp->m_insideWall = true;
            }
        }
        else if((dirA == Direction_e::EAST && dirB == Direction_e::WEST) || (dirB == Direction_e::WEST && dirA == Direction_e::EAST))
        {
            mapComp->m_absoluteMapPositionPX.first = m_mapMemCrushPos[entity].first;
            if(playerComp)
            {
                playerComp->m_frozen = true;
                GravityComponent *gravComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(entity);
                gravComp->m_jump = false;
                playerComp->m_insideWall = true;
            }
        }
    }
}

//===================================================================
void CollisionSystem::collisionRectTriangleEject(CollisionArgs &args, bool down)
{
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(args.entityNumA);
    RectangleCollisionComponent *rectCollA = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(args.entityNumA);
    TriangleStairCollisionComponent *triangleCollB = Ecsm_t::instance().getComponent<TriangleStairCollisionComponent, Components_e::TRIANGLE_STAIR_COLLISION_COMPONENT>(args.entityNumB);
    assert(rectCollA);
    assert(triangleCollB);
    assert(mapComp);
    float elementAPosX = args.mapCompA.m_absoluteMapPositionPX.first;
    float elementAPosY = args.mapCompA.m_absoluteMapPositionPX.second;
    float elementASecondPosX = elementAPosX+ rectCollA->m_size.first;
    float elementASecondPosY = elementAPosY+ rectCollA->m_size.second;
    float elementBPosX = args.mapCompB.m_absoluteMapPositionPX.first;
    float elementBPosY = args.mapCompB.m_absoluteMapPositionPX.second;
    float elementBSecondPosX = elementBPosX + triangleCollB->m_size.first;
    float elementBSecondPosY = elementBPosY + triangleCollB->m_size.second;
    bool limitEjectY = false, limitEjectX = false, crushMode = false;
    float diffY, diffX = EPSILON_FLOAT;
    //eject Y
    diffY = getRectRectEject({elementAPosX, elementAPosY, elementASecondPosY,
                              elementBPosX, elementBPosY, elementBSecondPosY}, limitEjectY);
    //eject X
    diffX = getRectRectEject({elementAPosY, elementAPosX, elementASecondPosX,
                              elementBPosY, elementBPosX, elementBSecondPosX}, limitEjectY);
    GravityComponent *gravityComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(args.entityNumA);
    assert(gravityComp);
    //if player touch ground
    if(args.tagCompA.m_tagA == CollisionTag_e::ENEMY_CT || args.tagCompA.m_tagA == CollisionTag_e::VEHICULE_CT || (args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT && !gravityComp->m_jump))
    {
        //if y change is lower than Y
        bool YChange = (std::abs(diffY) < std::abs(diffX));
        if(YChange && diffY < 0)
        {
            gravityComp->m_onGround = true;
            gravityComp->m_memOnGround = true;
            if(gravityComp->m_fall)
            {
                if(std::abs(std::abs(diffY) - std::abs(diffX)) >= 1.0f)
                {
                    //cancel gravity
                    mapComp->m_absoluteMapPositionPX.second -= gravityComp->m_gravityCohef;
                    gravityComp->m_fall = false;
                    diffY = std::numeric_limits<float>::epsilon();
                }
            }
            //if player go down
            if(args.tagCompA.m_tagA == CollisionTag_e::VEHICULE_CT && std::abs(diffX) < 31.0f)
            {
                PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
                assert(playerComp);
                if(args.entityNumA != *playerComp->m_associatedVehicle)
                {
                    return;
                }
                VehicleComponent *vehicleComp = Ecsm_t::instance().getComponent<VehicleComponent, Components_e::VEHICLE_COMPONENT>(args.entityNumA);
                assert(vehicleComp);
                //if onstair ==> sprite already treated for this frame
                if((!vehicleComp->m_vehicleShoot && ((std::abs(diffY) > EPSILON_FLOAT && (!vehicleComp->m_onStair || std::abs(diffX) == 30)) ||
                                                      (!triangleCollB->m_upStair && std::abs(diffX) == 30 && std::abs(diffY) == EPSILON_FLOAT))) ||
                    (vehicleComp->m_vehicleShoot && ((!triangleCollB->m_upStair && diffX > EPSILON_FLOAT) || (triangleCollB->m_upStair && diffX < EPSILON_FLOAT))))
                {
                    updateVehicleSpriteType(down);
                }
            }
            if(down && elementAPosX > elementBPosX)
            {
                mapComp->m_absoluteMapPositionPX.second = (elementBPosY - rectCollA->m_size.second) + std::fmod(elementAPosX, LEVEL_TILE_SIZE_PX);
                addEntityToZone(args.entityNumA, *getLevelCoord(mapComp->m_absoluteMapPositionPX));
                return;
            }
            else if(!down && elementASecondPosX < elementBSecondPosX)
            {
                mapComp->m_absoluteMapPositionPX.second = (elementBSecondPosY - rectCollA->m_size.second) - std::fmod(elementASecondPosX, LEVEL_TILE_SIZE_PX);
                addEntityToZone(args.entityNumA, *getLevelCoord(mapComp->m_absoluteMapPositionPX));
                return;
            }
            if(gravityComp->m_onGround)
            {
                if(args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT)
                {
                    m_refMainEngine->memPlayerCurrentWallOnGround(args.entityNumB);
                }
                else
                {
                    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
                    if(playerComp->m_associatedVehicle && args.entityNumA == *playerComp->m_associatedVehicle)
                    {
                        m_refMainEngine->memPlayerCurrentWallOnGround(args.entityNumB);
                    }
                }
            }
            // if(gravityComp->m_onGround && args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT)
            // {
            //     m_refMainEngine->memPlayerCurrentWallOnGround(args.entityNumB);
            // }
        }
        else if(!YChange && ((down && diffX > 0.0f) || (!down && diffX < 0.0f)))
        {
            if(gravityComp->m_memOnGround)
            {
                gravityComp->m_fall = false;
                gravityComp->m_onGround = true;
            }
            else
            {
                gravityComp->m_memOnGround = false;
                gravityComp->m_onGround = false;
                gravityComp->m_fall = true;
            }
            if(args.tagCompA.m_tagA == CollisionTag_e::VEHICULE_CT && std::abs(diffX) < 31.0f)
            {
                VehicleComponent *vehicleComp = Ecsm_t::instance().getComponent<VehicleComponent, Components_e::VEHICLE_COMPONENT>(args.entityNumA);
                assert(vehicleComp);
                PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
                assert(playerComp);
                if(args.entityNumA != *playerComp->m_associatedVehicle)
                {
                    return;
                }
                updateVehicleSpriteType(down);
            }
            if(down && diffX > 0.0f && elementAPosX > elementBPosX)
            {
                gravityComp->m_onGround = true;
                gravityComp->m_memOnGround = true;
                mapComp->m_absoluteMapPositionPX.second = (elementBPosY - rectCollA->m_size.second) + std::fmod(elementAPosX, LEVEL_TILE_SIZE_PX);
                addEntityToZone(args.entityNumA, *getLevelCoord(mapComp->m_absoluteMapPositionPX));
                return;
            }
            else if(!down && elementASecondPosX < elementBSecondPosX)
            {
                gravityComp->m_onGround = true;
                gravityComp->m_memOnGround = true;
                mapComp->m_absoluteMapPositionPX.second = (elementBSecondPosY - rectCollA->m_size.second) - std::fmod(elementASecondPosX, LEVEL_TILE_SIZE_PX);
                addEntityToZone(args.entityNumA, *getLevelCoord(mapComp->m_absoluteMapPositionPX));
                return;
            }

        }
    }
    if(args.tagCompA.m_tagA == CollisionTag_e::ENEMY_CT)
    {
        GravityComponent *gravityComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(args.entityNumA);
        assert(gravityComp);
        if(diffY < 0)
        {
            gravityComp->m_onGround = true;
            gravityComp->m_memOnGround = true;
            gravityComp->m_jump = false;
            if(gravityComp->m_fall)
            {
                //cancel gravity
                mapComp->m_absoluteMapPositionPX.second -= gravityComp->m_gravityCohef;
                gravityComp->m_fall = false;
                diffY = std::numeric_limits<float>::epsilon();
            }
        }
        else
        {
            gravityComp->m_memOnGround = false;
        }
    }
    collisionEject(*mapComp, diffX, diffY, crushMode);
    addEntityToZone(args.entityNumA, *getLevelCoord(mapComp->m_absoluteMapPositionPX));
}

//===================================================================
void CollisionSystem::updateVehicleSpriteType(bool stairDown)
{
    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    assert(playerComp);
    if(playerComp->m_associatedVehicle)
    {
        VehicleComponent *vehicleComp = Ecsm_t::instance().getComponent<VehicleComponent, Components_e::VEHICLE_COMPONENT>(*playerComp->m_associatedVehicle);
        assert(vehicleComp);
        if(vehicleComp->m_touchGround)
        {
            return;
        }
        vehicleComp->m_onStair = true;
        vehicleComp->m_stairCount = 0;
        bool currentShoot = vehicleComp->m_currentShootAnimation ? true: false;
        VehicleSpriteType_e previous;
        if(stairDown)
        {
            if(playerComp->m_currentDirectionRight)
            {
                previous = currentShoot ? VehicleSpriteType_e::SHOOT_STAIR_DOWN_RIGHT : VehicleSpriteType_e::STAIR_DOWN_RIGHT;
            }
            else
            {
                previous = currentShoot ? VehicleSpriteType_e::SHOOT_STAIR_UP_LEFT : VehicleSpriteType_e::STAIR_UP_LEFT;
            }
        }
        else
        {
            if(playerComp->m_currentDirectionRight)
            {
                previous = currentShoot ? VehicleSpriteType_e::SHOOT_STAIR_UP_RIGHT : VehicleSpriteType_e::STAIR_UP_RIGHT;
            }
            else
            {
                previous = currentShoot ? VehicleSpriteType_e::SHOOT_STAIR_DOWN_LEFT : VehicleSpriteType_e::STAIR_DOWN_LEFT;
            }
        }
        if(previous == vehicleComp->m_currentSpritesType)
        {
            return;
        }
        vehicleComp->m_currentSpritesType = previous;
        if(currentShoot)
        {
            MapVehicleSprite_t::const_iterator it = vehicleComp->m_mapSpriteAssociate.find(vehicleComp->m_currentSpritesType);
            vehicleComp->m_currentSprite = it->second.first + vehicleComp->m_shootCount;
        }
    }
}

//===================================================================
float CollisionSystem::getVerticalCircleRectEject(const EjectCircleYArgs& args, bool &limitEject, bool visibleShot)
{
    float adj, diffYA = EPSILON_FLOAT, diffYB;
    if(std::abs(std::sin(args.radiantAngle)) < 0.01f &&
        (args.angleMode || args.circlePosY < args.elementPosY || args.circlePosY > args.elementSecondPosY))
    {
        float distUpPoint = std::abs(args.circlePosY - args.elementPosY),
            distDownPoint = std::abs(args.circlePosY - args.elementSecondPosY),
            diff = distUpPoint - distDownPoint;
        if(std::abs(diff) < 4.0f)
        {
            limitEject = true;
            if(diff < 0.0f)
            {
                --diffYA;
            }
            else
            {
                ++diffYA;
            }
            return diffYA;
        }
    }
    if(args.angleMode)
    {
        adj = std::abs(args.circlePosX - args.elementPosX);
        //args.elementPosX == 150 ??
        diffYA = getRectTriangleSide(adj, args.ray);
        //EJECT UP
        if(args.circlePosY < args.elementPosY ||
                std::abs(args.circlePosY - args.elementPosY) < std::abs(args.circlePosY - args.elementSecondPosY))
        {
            diffYA -= (args.elementPosY - args.circlePosY);
            if(diffYA > EPSILON_FLOAT)
            {
                diffYA = -diffYA;
            }
        }
        //EJECT DOWN
        else
        {
            diffYA -= (args.circlePosY - args.elementSecondPosY);
            if(diffYA < EPSILON_FLOAT)
            {
                diffYA = -diffYA;
            }
        }
    }
    else
    {
        diffYA = args.elementPosY - (args.circlePosY + args.ray);
        diffYB = args.elementSecondPosY - (args.circlePosY - args.ray);
        if(visibleShot)
        {
            diffYA = (std::sin(args.radiantAngle) < EPSILON_FLOAT) ? diffYA : diffYB;
        }
        else
        {
            diffYA = (std::abs(diffYA) < std::abs(diffYB)) ? diffYA : diffYB;
        }
    }
    return diffYA;
}

//===================================================================
float CollisionSystem::getHorizontalCircleRectEject(const EjectCircleXArgs &args, bool &limitEject, bool visibleShot)
{
    float adj, diffXA = EPSILON_FLOAT, diffXB;
    if(args.angleMode)
    {
        adj = std::abs(args.circlePosY - args.elementPosY);
        diffXA = getRectTriangleSide(adj, args.ray);
        //EJECT LEFT
        if(args.circlePosX < args.elementPosX ||
                std::abs(args.circlePosX - args.elementPosX) < std::abs(args.circlePosX - args.elementSecondPosX))
        {
            diffXA -= std::abs(args.elementPosX - args.circlePosX);
            if(diffXA > EPSILON_FLOAT)
            {
                diffXA = -diffXA;
            }
        }
        //EJECT RIGHT
        else
        {
            diffXA -= (args.circlePosX - args.elementSecondPosX);
            if(diffXA < EPSILON_FLOAT)
            {
                diffXA = -diffXA;
            }
        }
    }
    else
    {
        diffXA = args.elementPosX - (args.circlePosX + args.ray);
        diffXB = args.elementSecondPosX - (args.circlePosX - args.ray);
        if(visibleShot)
        {
            diffXA = (std::cos(args.radiantAngle) > EPSILON_FLOAT) ? diffXA : diffXB;
        }
        else
        {
            diffXA = (std::abs(diffXA) < std::abs(diffXB)) ? diffXA : diffXB;
        }
    }
    return diffXA;
}

//===================================================================
float CollisionSystem::getRectRectEject(const EjectRectRectArgs &args, bool &limitEject)
{
    float diffYA = EPSILON_FLOAT;
    //EJECT UP
    if(args.elementASecondPosY < args.elementBSecondPosY && args.elementASecondPosY > args.elementBPosY)
    {
        diffYA -= (args.elementASecondPosY - args.elementBPosY);
    }
    //EJECT DOWN
    else if(args.elementAPosY < args.elementBSecondPosY)
    {
        diffYA += (args.elementBSecondPosY - args.elementAPosY);
    }
    // assert(diffYA > 1.0f);
    return diffYA;
}

//===================================================================
void CollisionSystem::collisionEject(MapCoordComponent &mapComp, float diffX, float diffY, bool crushCase)
{
    float minEject = std::min(std::abs(diffY), std::abs(diffX));
    if(minEject >= LEVEL_TILE_SIZE_PX)
    {
        return;
    }
    if(crushCase)
    {
        m_memCrush.push_back({{EPSILON_FLOAT, EPSILON_FLOAT}, false, {}, {}});
    }
    if(std::abs(diffY) < std::abs(diffX))
    {
        if(crushCase)
        {
            std::get<0>(m_memCrush.back()).second = diffY;
        }
        if(std::abs(diffY) < 15.0f)
        {
            mapComp.m_absoluteMapPositionPX.second += diffY;
        }
    }
    else
    {
        if(crushCase)
        {
            std::get<0>(m_memCrush.back()).first = diffX;
        }
        if(std::abs(diffX) < 15.0f)
        {
            mapComp.m_absoluteMapPositionPX.first += diffX;
        }
    }
}

//===================================================================
void CollisionSystem::collisionCircleCircleEject(CollisionArgs &args,
                                                 const CircleCollisionComponent &circleCollA,
                                                 const CircleCollisionComponent &circleCollB)
{
    float circleAPosX = args.mapCompA.m_absoluteMapPositionPX.first;
    float circleAPosY = args.mapCompA.m_absoluteMapPositionPX.second;
    float circleBPosX = args.mapCompB.m_absoluteMapPositionPX.first;
    float circleBPosY = args.mapCompB.m_absoluteMapPositionPX.second;
    float distanceX = std::abs(circleAPosX - circleBPosX);
    float distanceY = std::abs(circleAPosY - circleBPosY);
    float hyp = circleCollA.m_ray + circleCollB.m_ray;
    float diffX = getRectTriangleSide(distanceY, hyp);
    float diffY = getRectTriangleSide(distanceX, hyp);
    diffX -= distanceX;
    diffY -= distanceY;
    if(circleAPosX < circleBPosX)
    {
        diffX = -diffX;
    }
    if(circleAPosY < circleBPosY)
    {
        diffY = -diffY;
    }
    collisionEject(args.mapCompA, diffX, diffY);
}

//===================================================================
Direction_e getDirection(float diffX, float diffY)
{
    bool vert = (std::abs(diffX) > std::abs(diffY));
    return vert ? ((diffY < EPSILON_FLOAT) ? Direction_e::NORTH : Direction_e::SOUTH) :
                  ((diffX < EPSILON_FLOAT) ? Direction_e::WEST : Direction_e::EAST);
}

//===================================================================
bool opposingDirection(Direction_e dirA, Direction_e dirB)
{
    std::bitset<4> bitset;
    bitset[static_cast<uint32_t>(dirA)] = true;
    bitset[static_cast<uint32_t>(dirB)] = true;
    return (bitset[static_cast<uint32_t>(Direction_e::EAST)] && bitset[static_cast<uint32_t>(Direction_e::WEST)]) ||
            (bitset[static_cast<uint32_t>(Direction_e::NORTH)] && bitset[static_cast<uint32_t>(Direction_e::SOUTH)]);
}
