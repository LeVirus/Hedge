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
#include <ECS/Components/CheckpointComponent.hpp>
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
    for(std::set<uint32_t>::iterator it = m_usedEntities.begin(); it != m_usedEntities.end(); ++it, ++i)
    {
        SegmentCollisionComponent *segmentCompA = nullptr;
        GeneralCollisionComponent *tagCompA = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(*it);
        //check if entity is moveable
        MoveableComponent *moveCompA = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(*it);
        if(!tagCompA->m_active || tagCompA->m_tagA == CollisionTag_e::WALL_CT || tagCompA->m_tagA == CollisionTag_e::OBJECT_CT)
        {
            continue;
        }
        m_memCrush.clear();
        if(tagCompA->m_shape == CollisionShape_e::SEGMENT_C &&
                (tagCompA->m_tagA == CollisionTag_e::BULLET_PLAYER_CT || tagCompA->m_tagA == CollisionTag_e::BULLET_ENEMY_CT))
        {
            m_memDistCurrentBulletColl.second = EPSILON_FLOAT;

            segmentCompA = Ecsm_t::instance().getComponent<SegmentCollisionComponent, Components_e::SEGMENT_COLLISION_COMPONENT>(*it);
            addEntityToZone(segmentCompA->m_impactEntity, *getLevelCoord(segmentCompA->m_points.second));
            tagCompA->m_active = false;
        }
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
        if(moveCompA && (tagCompA->m_tagA == CollisionTag_e::PLAYER_CT || tagCompA->m_tagA == CollisionTag_e::ENEMY_CT))
        {
            treatGeneralCrushing(*it);
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
void CollisionSystem::secondEntitiesLoop(uint32_t entityA, uint32_t currentIteration, GeneralCollisionComponent &tagCompA, bool shotExplosionEject)
{
    if(tagCompA.m_tagA == CollisionTag_e::DETECT_MAP_CT ||
            tagCompA.m_tagA == CollisionTag_e::BULLET_PLAYER_CT || tagCompA.m_tagA == CollisionTag_e::BULLET_ENEMY_CT)
    {
        for(std::set<uint32_t>::iterator it = m_usedEntities.begin(); it != m_usedEntities.end(); ++it)
        {
            if(!iterationLoop(currentIteration, entityA, *it, tagCompA, shotExplosionEject))
            {
                return;
            }
        }
        return;
    }
    SetUi_t set = m_zoneLevel->getEntitiesFromZones(entityA);
    SetUi_t::iterator it = set.begin();
    for(; it != set.end(); ++it)
    {
        if(!iterationLoop(currentIteration, entityA, *it, tagCompA, shotExplosionEject))
        {
            return;
        }
    }
}

//===================================================================
bool CollisionSystem::iterationLoop(uint32_t currentIteration, uint32_t entityA, uint32_t entityB,
                                    GeneralCollisionComponent &tagCompA, bool shotExplosionEject)
{
    if(currentIteration == entityB)
    {
        return true;
    }
    GeneralCollisionComponent *tagCompB = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(entityB);
    if(!tagCompB->m_active)
    {
        return true;
    }
    if(!checkTag(tagCompA.m_tagA, tagCompB->m_tagA) && !checkTag(tagCompA.m_tagA, tagCompB->m_tagB))
    {
        return true;
    }
    if(!treatCollision(entityA, entityB, tagCompA, *tagCompB, shotExplosionEject))
    {
        if(tagCompA.m_tagA == CollisionTag_e::BULLET_PLAYER_CT || tagCompA.m_tagA == CollisionTag_e::BULLET_ENEMY_CT)
        {
            secondEntitiesLoop(entityA, currentIteration, tagCompA, true);
        }
        return false;
    }
    return true;
}

//===================================================================
bool CollisionSystem::checkEnemyRemoveCollisionMask(uint32_t entityNum)
{
    EnemyConfComponent *enemyConfComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(entityNum);
    if(enemyConfComp->m_displayMode == EnemyDisplayMode_e::DEAD)
    {
        MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(entityNum);
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
    if(!crush && !playerComp->m_insideWall)
    {
        playerComp->m_crush = false;
        playerComp->m_frozen = false;
    }
    if(playerComp->m_crush)
    {
        playerComp->takeDamage(1);
    }
}

//===================================================================
void CollisionSystem::treatEnemyTakeDamage(uint32_t enemyEntityNum, uint32_t damage)
{
    EnemyConfComponent *enemyConfCompB = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(enemyEntityNum);
    TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(enemyEntityNum);
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
    //if enemy dead
    if(!enemyConfCompB->takeDamage(damage))
    {
        if(!playerComp->m_enemiesKilled)
        {
            playerComp->m_enemiesKilled = 1;
        }
        else
        {
            ++(*playerComp->m_enemiesKilled);
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
void CollisionSystem::confDropedObject(uint32_t objectEntity, uint32_t enemyEntity)
{

    GeneralCollisionComponent *genComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(objectEntity);
    MapCoordComponent *objectMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(objectEntity);
    MapCoordComponent *enemyMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(enemyEntity);
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
        if(tagCompTarget->m_tagA == CollisionTag_e::WALL_CT)
        {
            continue;
        }
        GeneralCollisionComponent *tagCompBullet = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(m_vectMemShots[i].first);
        if(tagCompBullet->m_tagA == CollisionTag_e::BULLET_PLAYER_CT)
        {
            ShotConfComponent *shotConfComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(m_vectMemShots[i].first);
            if(tagCompTarget->m_tagA == CollisionTag_e::ENEMY_CT)
            {
                treatEnemyTakeDamage(m_vectMemShots[i].second, shotConfComp->m_damage);
            }
            tagCompBullet->m_active = false;
        }
        else if(tagCompBullet->m_tagA == CollisionTag_e::BULLET_ENEMY_CT)
        {
            ShotConfComponent *shotConfComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(m_vectMemShots[i].first);
            if(tagCompTarget->m_tagA == CollisionTag_e::PLAYER_CT)
            {
                PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
                playerComp->takeDamage(shotConfComp->m_damage);
            }
        }
    }
}

//===================================================================
void CollisionSystem::rmEnemyCollisionMaskEntity(uint32_t numEntity)
{
    GeneralCollisionComponent *tagComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(numEntity);
    tagComp->m_tagA = CollisionTag_e::DEAD_CORPSE_CT;
}

//===================================================================
void CollisionSystem::initArrayTag()
{
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::WALL_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::ENEMY_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::OBJECT_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::STATIC_SET_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::LOG_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::CHECKPOINT_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_CT, CollisionTag_e::SECRET_CT});

    m_tagArray.insert({CollisionTag_e::DETECT_MAP_CT, CollisionTag_e::WALL_CT});
    m_tagArray.insert({CollisionTag_e::DETECT_MAP_CT, CollisionTag_e::STATIC_SET_CT});
    m_tagArray.insert({CollisionTag_e::DETECT_MAP_CT, CollisionTag_e::EXIT_CT});
    m_tagArray.insert({CollisionTag_e::DETECT_MAP_CT, CollisionTag_e::LOG_CT});

    m_tagArray.insert({CollisionTag_e::PLAYER_ACTION_CT, CollisionTag_e::EXIT_CT});
    m_tagArray.insert({CollisionTag_e::PLAYER_ACTION_CT, CollisionTag_e::LOG_CT});
    m_tagArray.insert({CollisionTag_e::HIT_PLAYER_CT, CollisionTag_e::ENEMY_CT});

    m_tagArray.insert({CollisionTag_e::EXPLOSION_CT, CollisionTag_e::PLAYER_CT});
    m_tagArray.insert({CollisionTag_e::EXPLOSION_CT, CollisionTag_e::ENEMY_CT});

    m_tagArray.insert({CollisionTag_e::ENEMY_CT, CollisionTag_e::PLAYER_CT});
    m_tagArray.insert({CollisionTag_e::ENEMY_CT, CollisionTag_e::WALL_CT});
    m_tagArray.insert({CollisionTag_e::ENEMY_CT, CollisionTag_e::STATIC_SET_CT});
    m_tagArray.insert({CollisionTag_e::ENEMY_CT, CollisionTag_e::LOG_CT});

    m_tagArray.insert({CollisionTag_e::WALL_CT, CollisionTag_e::PLAYER_CT});
    m_tagArray.insert({CollisionTag_e::WALL_CT, CollisionTag_e::ENEMY_CT});

    m_tagArray.insert({CollisionTag_e::BULLET_ENEMY_CT, CollisionTag_e::PLAYER_CT});
    m_tagArray.insert({CollisionTag_e::BULLET_ENEMY_CT, CollisionTag_e::WALL_CT});

    m_tagArray.insert({CollisionTag_e::BULLET_PLAYER_CT, CollisionTag_e::ENEMY_CT});
    m_tagArray.insert({CollisionTag_e::BULLET_PLAYER_CT, CollisionTag_e::WALL_CT});

    m_tagArray.insert({CollisionTag_e::IMPACT_CT, CollisionTag_e::WALL_CT});
    m_tagArray.insert({CollisionTag_e::IMPACT_CT, CollisionTag_e::ENEMY_CT});

    m_tagArray.insert({CollisionTag_e::DEAD_CORPSE_CT, CollisionTag_e::WALL_CT});
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
                                     GeneralCollisionComponent &tagCompB, bool shotExplosionEject)
{
    if(tagCompA.m_shape == CollisionShape_e::RECTANGLE_C)
    {
        MapCoordComponent *mapCompA = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNumA);
        MapCoordComponent *mapCompB = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNumB);
        CollisionArgs args = {entityNumA, entityNumB, tagCompA, tagCompB, *mapCompA, *mapCompB};
        checkCollisionFirstRect(args);
    }
    else if(tagCompA.m_shape == CollisionShape_e::CIRCLE_C)
    {
        MapCoordComponent *mapCompA = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNumA);
        MapCoordComponent *mapCompB = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNumB);
        CollisionArgs args = {entityNumA, entityNumB, tagCompA, tagCompB, *mapCompA, *mapCompB};
        return treatCollisionFirstCircle(args, shotExplosionEject);
    }
    else if(tagCompA.m_shape == CollisionShape_e::SEGMENT_C)
    {
        MapCoordComponent *mapCompA = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(entityNumA);
        assert(tagCompA.m_tagA == CollisionTag_e::BULLET_PLAYER_CT || tagCompA.m_tagA == CollisionTag_e::BULLET_ENEMY_CT);
        checkCollisionFirstSegment(entityNumA, entityNumB, tagCompB, *mapCompA);
    }
    return true;
}

//Detect map only
//===================================================================
void CollisionSystem::checkCollisionFirstRect(CollisionArgs &args)
{
    MapDisplaySystem *mapSystem = Ecsm_t::instance().getSystem<MapDisplaySystem>(static_cast<uint32_t>(Systems_e::MAP_DISPLAY_SYSTEM));
    assert(mapSystem);
    if(mapSystem->entityAlreadyDiscovered(args.entityNumB))
    {
        return;
    }
    bool collision = false;
    RectangleCollisionComponent *rectCompA = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(args.entityNumA);
    switch(args.tagCompB.m_shape)
    {
    case CollisionShape_e::RECTANGLE_C:
    {
        RectangleCollisionComponent *rectCompB = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(args.entityNumB);
        collision = checkRectRectCollision(args.mapCompA.m_absoluteMapPositionPX, rectCompA->m_size,
                               args.mapCompB.m_absoluteMapPositionPX, rectCompB->m_size);
    }
        break;
    case CollisionShape_e::CIRCLE_C:
    {
        CircleCollisionComponent *circleCompB = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(args.entityNumB);
        collision = checkCircleRectCollision(args.mapCompB.m_absoluteMapPositionPX, circleCompB->m_ray,
                                 args.mapCompA.m_absoluteMapPositionPX, rectCompA->m_size);
    }
        break;
    case CollisionShape_e::SEGMENT_C:
        break;
    }
    if(collision)
    {
        mapSystem->addDiscoveredEntity(args.entityNumB, *getLevelCoord(args.mapCompB.m_absoluteMapPositionPX));
    }
}

//===================================================================
void CollisionSystem::writePlayerInfo(const std::string &info)
{
    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(playerComp->m_memEntityAssociated);
    timerComp->m_cycleCountA = 0;
    playerComp->m_infoWriteData = {true, info};
}

//===================================================================
bool CollisionSystem::treatCollisionFirstCircle(CollisionArgs &args, bool shotExplosionEject)
{
    if(args.tagCompA.m_tagA == CollisionTag_e::PLAYER_ACTION_CT ||
            args.tagCompA.m_tagA == CollisionTag_e::HIT_PLAYER_CT)
    {
        args.tagCompA.m_active = false;
    }
    CircleCollisionComponent *circleCompA = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(args.entityNumA);
    bool collision = false;
    switch(args.tagCompB.m_shape)
    {
    case CollisionShape_e::RECTANGLE_C:
    {
        RectangleCollisionComponent *rectCompB = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(args.entityNumB);
        collision = checkCircleRectCollision(args.mapCompA.m_absoluteMapPositionPX, circleCompA->m_ray,
                                             args.mapCompB.m_absoluteMapPositionPX, rectCompB->m_size);
        if(collision)
        {
            if(args.tagCompA.m_tagA == CollisionTag_e::PLAYER_CT)
            {
                if(treatCollisionPlayer(args, *circleCompA, *rectCompB))
                {
                    return true;
                }
            }
            else if(args.tagCompA.m_tagA == CollisionTag_e::ENEMY_CT)
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
        collision = checkCircleCircleCollision(args.mapCompA.m_absoluteMapPositionPX, circleCompA->m_ray,
                                               args.mapCompB.m_absoluteMapPositionPX, circleCompB->m_ray);
        if(collision)
        {
            if(args.tagCompA.m_tagA == CollisionTag_e::PLAYER_ACTION_CT)
            {
                treatActionPlayerCircle(args);
            }
            else if(args.tagCompA.m_tagA == CollisionTag_e::HIT_PLAYER_CT)
            {
                ShotConfComponent *shotConfComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(args.entityNumA);
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
    case CollisionShape_e::SEGMENT_C:
    {
    }
        break;
    }
    //TREAT VISIBLE SHOT
    if((args.tagCompA.m_tagA == CollisionTag_e::BULLET_ENEMY_CT) ||
            (args.tagCompA.m_tagA == CollisionTag_e::BULLET_PLAYER_CT))
    {
        ShotConfComponent *shotConfComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(args.entityNumA);
        bool limitX = (args.mapCompA.m_absoluteMapPositionPX.first < LEVEL_THIRD_TILE_SIZE_PX),
            limitY = (args.mapCompA.m_absoluteMapPositionPX.second < LEVEL_THIRD_TILE_SIZE_PX);
        //limit level case
        if(!shotConfComp->m_destructPhase && (limitX || limitY))
        {
            if(limitX)
            {
                args.mapCompA.m_absoluteMapPositionPX.first = LEVEL_THIRD_TILE_SIZE_PX;
            }
            if(limitY)
            {
                args.mapCompA.m_absoluteMapPositionPX.second = LEVEL_THIRD_TILE_SIZE_PX;
            }
            shotConfComp->m_destructPhase = true;
            if(shotConfComp->m_damageCircleRayData)
            {
                setDamageCircle(*shotConfComp->m_damageCircleRayData, true, args.entityNumA);
            }
            return true;
        }
        if(collision)
        {
            if(args.tagCompB.m_shape == CollisionShape_e::RECTANGLE_C)
            {
                if(shotExplosionEject)
                {
                    RectangleCollisionComponent *rectCompB = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(args.entityNumB);
                    collisionCircleRectEject(args, circleCompA->m_ray, *rectCompB, shotExplosionEject);
                }
                else if(!shotConfComp->m_ejectMode)
                {
                    shotConfComp->m_ejectMode = true;
                    std::swap(circleCompA->m_ray, shotConfComp->m_ejectExplosionRay);
                    return false;
                }
            }
            if(shotConfComp->m_destructPhase)
            {
                return true;
            }
            if(shotConfComp->m_damageCircleRayData)
            {
                setDamageCircle(*shotConfComp->m_damageCircleRayData, true, args.entityNumA);
            }
            activeSound(args.entityNumA);
            shotConfComp->m_destructPhase = true;
            shotConfComp->m_spriteShotNum = 0;
            if(shotConfComp->m_damageCircleRayData)
            {
                return true;
            }
            if(args.tagCompA.m_tagA == CollisionTag_e::BULLET_PLAYER_CT && args.tagCompB.m_tagA == CollisionTag_e::ENEMY_CT)
            {
                treatEnemyTakeDamage(args.entityNumB, shotConfComp->m_damage);
            }
            else if(args.tagCompA.m_tagA == CollisionTag_e::BULLET_ENEMY_CT && args.tagCompB.m_tagA == CollisionTag_e::PLAYER_CT)
            {
                PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
                playerComp->takeDamage(shotConfComp->m_damage);
            }
        }
    }
    return true;
}

//===================================================================
bool CollisionSystem::treatCollisionPlayer(CollisionArgs &args, CircleCollisionComponent &circleCompA, RectangleCollisionComponent &rectCompB)
{
   if(args.tagCompB.m_tagA == CollisionTag_e::CHECKPOINT_CT)
    {
        PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
        CheckpointComponent *checkpointComp = Ecsm_t::instance().getComponent<CheckpointComponent, Components_e::CHECKPOINT_COMPONENT>(args.entityNumB);
        if(!playerComp->m_currentCheckpoint || checkpointComp->m_checkpointNumber > playerComp->m_currentCheckpoint->first)
        {
            MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(args.entityNumB);
            playerComp->m_checkpointReached = mapComp->m_coord;
            playerComp->m_currentCheckpoint = {checkpointComp->m_checkpointNumber, checkpointComp->m_direction};
            writePlayerInfo("Checkpoint Reached");
        }
        m_vectEntitiesToDelete.push_back(args.entityNumB);
        return true;
    }
    else if(args.tagCompB.m_tagA == CollisionTag_e::SECRET_CT)
    {
        PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
        writePlayerInfo("Secret Found");
        if(!playerComp->m_secretsFound)
        {
            playerComp->m_secretsFound = 1;
        }
        else
        {
            ++(*playerComp->m_secretsFound);
        }
        m_vectEntitiesToDelete.push_back(args.entityNumB);
        return true;
    }
    collisionCircleRectEject(args, circleCompA.m_ray, rectCompB);
    return false;
}

//===================================================================
void CollisionSystem::setDamageCircle(uint32_t damageEntity, bool active, uint32_t baseEntity)
{
    GeneralCollisionComponent *genDam = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(damageEntity);
    genDam->m_active = active;
    if(active)
    {
        MapCoordComponent *mapCompDam = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(damageEntity);
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(baseEntity);
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
    audioComp->m_soundElements[0]->m_toPlay = true;
}

//===================================================================
void CollisionSystem::treatActionPlayerCircle(CollisionArgs &args)
{
    if(args.tagCompB.m_tagA == CollisionTag_e::EXIT_CT)
    {
        m_refMainEngine->activeEndLevel();
    }
    else if(args.tagCompB.m_tagA == CollisionTag_e::LOG_CT)
    {
        PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
        LogComponent *logComp = Ecsm_t::instance().getComponent<LogComponent, Components_e::LOG_COMPONENT>(args.entityNumB);
        playerComp->m_infoWriteData = {true, logComp->m_message};
        TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(playerComp->m_memEntityAssociated);
        timerComp->m_cycleCountA = 0;
        timerComp->m_timeIntervalOptional = 4.0 / FPS_VALUE;
    }
}

//===================================================================
void CollisionSystem::treatPlayerPickObject(CollisionArgs &args)
{
    ObjectConfComponent *objectComp = Ecsm_t::instance().getComponent<ObjectConfComponent, Components_e::OBJECT_CONF_COMPONENT>(args.entityNumB);
    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    WeaponComponent *weaponComp = Ecsm_t::instance().getComponent<WeaponComponent, Components_e::WEAPON_COMPONENT>(playerComp->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::WEAPON)]);
    std::string info;
    switch (objectComp->m_type)
    {
    case ObjectType_e::AMMO_WEAPON:
    {
        if(!pickUpAmmo(*objectComp->m_weaponID, *weaponComp, objectComp->m_containing))
        {
            return;
        }
        info = weaponComp->m_weaponsData[*objectComp->m_weaponID].m_weaponName + " Ammo";
    }
        break;
    case ObjectType_e::WEAPON:
    {
        if(!pickUpWeapon(*objectComp->m_weaponID, *weaponComp, objectComp->m_containing))
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
    case ObjectType_e::TOTAL:
        assert(false);
        break;
    }
    removeEntityToZone(args.entityNumB);
    playerComp->m_infoWriteData = {true, info};
    TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(playerComp->m_memEntityAssociated);
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
        PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
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
bool pickUpAmmo(uint32_t numWeapon, WeaponComponent &weaponComp, uint32_t objectContaining)
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
            setPlayerWeapon(weaponComp, numWeapon);
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
bool pickUpWeapon(uint32_t numWeapon, WeaponComponent &weaponComp, uint32_t objectContaining)
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
            setPlayerWeapon(weaponComp, numWeapon);
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
void CollisionSystem::checkCollisionFirstSegment(uint32_t numEntityA, uint32_t numEntityB,
                                                 GeneralCollisionComponent &tagCompB,
                                                 MapCoordComponent &mapCompB)
{
    SegmentCollisionComponent *segmentCompA = Ecsm_t::instance().getComponent<SegmentCollisionComponent, Components_e::SEGMENT_COLLISION_COMPONENT>(numEntityA);
    switch(tagCompB.m_shape)
    {
    case CollisionShape_e::RECTANGLE_C:
    case CollisionShape_e::SEGMENT_C:
    {
    }
        break;
    case CollisionShape_e::CIRCLE_C:
    {
        CircleCollisionComponent *circleCompB = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(numEntityB);
        if(checkCircleSegmentCollision(mapCompB.m_absoluteMapPositionPX, circleCompB->m_ray,
                                       segmentCompA->m_points.first,
                                       segmentCompA->m_points.second))
        {
            //Fix impact displayed behind element
            float distance = getDistance(segmentCompA->m_points.first,
                                         mapCompB.m_absoluteMapPositionPX) - 5.0f;
            if(m_memDistCurrentBulletColl.second <= EPSILON_FLOAT ||
                    distance < m_memDistCurrentBulletColl.second)
            {
                m_memDistCurrentBulletColl = {numEntityB, distance};
            }
        }
    }
        break;
    }
}

//===================================================================
void CollisionSystem::collisionCircleRectEject(CollisionArgs &args, float circleRay,
                                               const RectangleCollisionComponent &rectCollB, bool visibleShotFirstEject)
{
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(args.entityNumA);
    MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(args.entityNumA);
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
    collisionEject(*mapComp, diffX, diffY, limitEjectY, limitEjectX, crushMode);
    addEntityToZone(args.entityNumA, *getLevelCoord(mapComp->m_absoluteMapPositionPX));
}

//===================================================================
float CollisionSystem::getVerticalCircleRectEject(const EjectYArgs& args, bool &limitEject, bool visibleShot)
{
    float adj, diffYA = EPSILON_FLOAT, diffYB;
    if(std::abs(std::sin(args.radiantAngle)) < 0.01f &&
            (args.angleMode || args.circlePosY < args.elementPosY || args.circlePosY > args.elementSecondPosY))
    {
        float distUpPoint = std::abs(args.circlePosY - args.elementPosY),
                distDownPoint = std::abs(args.circlePosY - args.elementSecondPosY);
        limitEject = true;
        if(distUpPoint < distDownPoint)
        {
            --diffYA;
        }
        else
        {
            ++diffYA;
        }
        if(limitEject)
        {
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
float CollisionSystem::getHorizontalCircleRectEject(const EjectXArgs &args, bool &limitEject, bool visibleShot)
{
    float adj, diffXA = EPSILON_FLOAT, diffXB;
    if(std::abs(std::cos(args.radiantAngle)) < 0.01f && (args.angleMode || args.circlePosX < args.elementPosX ||
             args.circlePosX > args.elementSecondPosX))
    {
        float distLeftPoint = std::abs(args.circlePosX - args.elementPosX),
                distRightPoint = std::abs(args.circlePosX - args.elementSecondPosX);
        limitEject = true;
        if(distLeftPoint < distRightPoint)
        {
            --diffXA;
        }
        else
        {
            ++diffXA;
        }
        if(limitEject)
        {
            return diffXA;
        }
    }
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
void CollisionSystem::collisionEject(MapCoordComponent &mapComp, float diffX, float diffY, bool limitEjectY, bool limitEjectX, bool crushCase)
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
    if(!limitEjectX && (limitEjectY || std::abs(diffY) < std::abs(diffX)))
    {
        if(crushCase)
        {
            std::get<0>(m_memCrush.back()).second = diffY;
        }
        mapComp.m_absoluteMapPositionPX.second += diffY;
    }
    if(!limitEjectY && (limitEjectX || std::abs(diffY) > std::abs(diffX)))
    {
        if(crushCase)
        {
            std::get<0>(m_memCrush.back()).first = diffX;
        }
        mapComp.m_absoluteMapPositionPX.first += diffX;
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
