#include <ECS/Components/EnemyConfComponent.hpp>
#include <ECS/Components/MapCoordComponent.hpp>
#include <ECS/Components/MoveableComponent.hpp>
#include <ECS/Components/SegmentCollisionComponent.hpp>
#include <ECS/Components/GeneralCollisionComponent.hpp>
#include <ECS/Components/TimerComponent.hpp>
#include <ECS/Components/CircleCollisionComponent.hpp>
#include <ECS/Components/PlayerConfComponent.hpp>
#include <ECS/Components/ShotConfComponent.hpp>
#include <ECS/Components/RectangleCollisionComponent.hpp>
#include <ECS/Components/WeaponComponent.hpp>
#include <ECS/Components/MemSpriteDataComponent.hpp>
#include <ECS/Components/MoveableComponent.hpp>
#include <ECS/Components/SpriteTextureComponent.hpp>
#include <ECS/Components/GeneratorComponent.hpp>
#include <ECS/Systems/CollisionSystem.hpp>
#include <cassert>
#include <random>
#include <alias.hpp>
#include "IASystem.hpp"
#include "PhysicalEngine.hpp"
#include "CollisionUtils.hpp"
#include "MainEngine.hpp"

//===================================================================
IASystem::IASystem()
{
    std::srand(std::time(nullptr));
    addComponentsToSystem(Components_e::ENEMY_CONF_COMPONENT, 1);
}

//===================================================================
void IASystem::execSystem()
{
    PlayerConfComponent *playerConfComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    WeaponComponent *weaponComp = Ecsm_t::instance().getComponent<WeaponComponent, Components_e::WEAPON_COMPONENT>(playerConfComp->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::WEAPON)]);
    treatEject();
    for(uint32_t i = 0; i < m_refVehicleAmmo.size(); ++i)
    {
        treatVisibleShots(m_refVehicleAmmo[i].get(), false, true);
    }
    for(uint32_t i = 0; i < weaponComp->m_weaponsData.size(); ++i)
    {
        if(weaponComp->m_weaponsData[i].m_attackType == AttackType_e::VISIBLE_SHOTS)
        {
            assert(weaponComp->m_weaponsData[i].m_visibleShootEntities);
            treatVisibleShots(*weaponComp->m_weaponsData[i].m_visibleShootEntities);
        }
    }
    treatVisibleShots(*weaponComp->m_grenadeData.m_visibleShootEntities, true);
    float distancePlayer;
    MapCoordComponent *playerMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerEntity);
    for(std::set<uint32_t>::iterator it = m_usedEntities.begin(); it != m_usedEntities.end(); ++it)
    {
        //OOOOK A modifier
        EnemyConfComponent *enemyConfComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(*it);
        if(enemyConfComp->m_visibleShot)
        {
            treatVisibleShots(enemyConfComp->m_visibleAmmo);
        }
        if(enemyConfComp->m_behaviourMode == EnemyBehaviourMode_e::DEAD ||
                enemyConfComp->m_behaviourMode == EnemyBehaviourMode_e::DYING)
        {
            if(enemyConfComp->m_playDeathSound)
            {
                activeSound(*it, static_cast<uint32_t>(EnemySoundEffect_e::DEATH));
                enemyConfComp->m_playDeathSound = false;
            }
            continue;
        }
        MapCoordComponent *enemyMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(*it);
        distancePlayer = getDistance(playerMapComp->m_absoluteMapPositionPX,
                                     enemyMapComp->m_absoluteMapPositionPX);
        if(enemyConfComp->m_behaviourMode == EnemyBehaviourMode_e::ATTACK)
        {
            treatEnemyBehaviourAttack(*it, *enemyMapComp, *enemyConfComp, distancePlayer);
        }
    }
    treatGenerator();
}

//===================================================================
void IASystem::treatEject()
{
    std::set<uint32_t> setCacheComp;
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> arrEntities;
    arrEntities[Components_e::MOVEABLE_COMPONENT] = 1;
    setCacheComp.insert(Components_e::MOVEABLE_COMPONENT);
    std::optional<std::set<uint32_t>> vectMoveableEntities = Ecsm_t::instance().getEntitiesCustomComponents(setCacheComp, arrEntities);
    assert(vectMoveableEntities);

    for(std::set<uint32_t>::iterator it = vectMoveableEntities->begin(); it != vectMoveableEntities->end(); ++it)
    {
        MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(*it);
        if(moveComp->m_ejectData)
        {
            TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(*it);
            if(++timerComp->m_cycleCountD >= moveComp->m_ejectData->second)
            {
                moveComp->m_ejectData = std::nullopt;
                return;
            }
            MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(*it);
            moveElementFromAngle(moveComp->m_ejectData->first, getRadiantAngle(moveComp->m_currentDegreeMoveDirection),
                                 mapComp->m_absoluteMapPositionPX);
        }
    }
}

//===================================================================
void IASystem::updateGeneratorEntities()
{
    m_vectGeneratorEntities->clear();
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> arrayComp;
    std::set<uint32_t> set;
    arrayComp.fill(0);
    arrayComp[Components_e::GENERATOR_COMPONENT] = 1;
    arrayComp[Components_e::TIMER_COMPONENT] = 1;

    set.insert(Components_e::GENERATOR_COMPONENT);
    set.insert(Components_e::TIMER_COMPONENT);

    m_vectGeneratorEntities = Ecsm_t::instance().getEntitiesCustomComponents(set, arrayComp);
}

//===================================================================
void IASystem::treatGenerator()
{
    for(std::set<uint32_t>::iterator it = m_vectGeneratorEntities->begin(); it != m_vectGeneratorEntities->end(); ++it)
    {
        GeneratorComponent *generatorComp = Ecsm_t::instance().getComponent<GeneratorComponent, Components_e::GENERATOR_COMPONENT>(*it);
        assert(generatorComp);
        TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(*it);
        assert(timerComp);
        if(++timerComp->m_cycleCountA > generatorComp->m_cycles)
        {
            MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(*it);
            assert(mapComp);
            timerComp->m_cycleCountA = 0;
            float angle;
            switch(generatorComp->m_dir)
            {
            case Direction_e::SOUTH:
                angle = 270.0f;
                break;
            case Direction_e::NORTH:
                angle = 90.0f;
                break;
            case Direction_e::WEST:
                angle = 180.0f;
                break;
            case Direction_e::EAST:
                angle = 0.0f;
                break;
            }
            if(generatorComp->m_genEnemies)
            {
                confEnemiesGenerator(*it, mapComp->m_absoluteMapPositionPX);
            }
            else
            {
                confVisibleShoot(generatorComp->m_vectElementGen, mapComp->m_absoluteMapPositionPX, angle, CollisionTag_e::BULLET_ENEMY_CT);
            }
        }
        if(!generatorComp->m_genEnemies)
        {
            treatVisibleShots(generatorComp->m_vectElementGen);
        }
    }
}

//===================================================================
void IASystem::treatVisibleShots(const std::vector<uint32_t> &stdAmmo, bool grenade, bool tank)
{
    for(uint32_t i = 0; i < stdAmmo.size(); ++i)
    {
        GeneralCollisionComponent *genColl = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(stdAmmo[i]);
        if(!genColl->m_active)
        {
            continue;
        }
        ShotConfComponent *shotComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(stdAmmo[i]);
        if(shotComp->m_destructPhase)
        {
            continue;
        }
        TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(stdAmmo[i]);
        if(++timerComp->m_cycleCountA > m_intervalVisibleShotLifeTime)
        {
            genColl->m_active = false;
            timerComp->m_cycleCountA = 0;
            continue;
        }
        MapCoordComponent *ammoMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(stdAmmo[i]);
        MoveableComponent *ammoMoveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(stdAmmo[i]);
        if(grenade)
        {
            PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
            WeaponComponent *weaponComp = Ecsm_t::instance().getComponent<WeaponComponent, Components_e::WEAPON_COMPONENT>(playerComp->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::WEAPON)]);
            assert(weaponComp);
            if(++timerComp->m_cycleCountB < weaponComp->m_grenadeData.m_cycleTime)
            {
                moveElementFromAngle(ammoMoveComp->m_velocity, getRadiantAngle(ammoMoveComp->m_degreeOrientation), ammoMapComp->m_absoluteMapPositionPX);
                m_mainEngine->addEntityToZone(stdAmmo[i], *getLevelCoord(ammoMapComp->m_absoluteMapPositionPX));
            }
        }
        else
        {
            SegmentCollisionComponent *segmentComp = Ecsm_t::instance().getComponent<SegmentCollisionComponent, Components_e::SEGMENT_COLLISION_COMPONENT>(stdAmmo[i]);
            segmentComp->m_points.first = ammoMapComp->m_absoluteMapPositionPX;
            moveElementFromAngle(ammoMoveComp->m_velocity, getRadiantAngle(ammoMoveComp->m_degreeOrientation), ammoMapComp->m_absoluteMapPositionPX);
            m_mainEngine->addEntityToZone(stdAmmo[i], *getLevelCoord(ammoMapComp->m_absoluteMapPositionPX));
            segmentComp->m_points.second = ammoMapComp->m_absoluteMapPositionPX;
            //if tank check collision down of shot picture
            if(tank)
            {
                SegmentCollisionComponent *segmentCompB = Ecsm_t::instance().getComponent<SegmentCollisionComponent, Components_e::SEGMENT_COLLISION_COMPONENT>(stdAmmo[i], 1);
                assert(segmentCompB);
                segmentCompB->m_points.first = segmentComp->m_points.first;
                segmentCompB->m_points.second = segmentComp->m_points.second;
                //on lateral ground
                if(ammoMoveComp->m_degreeOrientation == 0.0f || ammoMoveComp->m_degreeOrientation == 180.0f)
                {
                    segmentCompB->m_points.first.second += 20;
                    segmentCompB->m_points.second.second += 20;
                }
                else
                {
                    segmentCompB->m_points.first.second += 50;
                    segmentCompB->m_points.second.second += 50;
                }
            }
        }
    }
}

//===================================================================
void IASystem::activeSound(uint32_t entityNum, uint32_t soundNum)
{
    AudioComponent *audioComp = Ecsm_t::instance().getComponent<AudioComponent, Components_e::AUDIO_COMPONENT>(entityNum);
    audioComp->m_soundElements[soundNum]->m_toPlay = true;
}

//===================================================================
void IASystem::updateEnemyDirection(EnemyConfComponent &enemyConfComp, MoveableComponent &moveComp,
                                    MapCoordComponent &enemyMapComp)
{
    MapCoordComponent *playerMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerEntity);
    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    PairFloat_t point;
    //Check if vehicle associated
    RectangleCollisionComponent *rectComp = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(
        playerComp->m_associatedVehicle ? *playerComp->m_associatedVehicle : m_playerEntity);
    point = {playerMapComp->m_absoluteMapPositionPX.first + rectComp->m_size.first / 2, playerMapComp->m_absoluteMapPositionPX.second + rectComp->m_size.second / 2};
    moveComp.m_degreeOrientation = getTrigoAngle(enemyMapComp.m_absoluteMapPositionPX, point);
    if(enemyConfComp.m_type == TypeEnemy_e::FLYING)
    {
        moveComp.m_currentDegreeMoveDirection = std::abs(std::rand()) % 360;
        // moveComp.m_degreeOrientation = moveComp.m_currentDegreeMoveDirection;
        return;
    }
    if(enemyConfComp.m_attackPhase == EnemyAttackPhase_e::MOVE_TO_TARGET_LEFT)
    {
        moveComp.m_degreeOrientation += 90.0f;
    }
    else if(enemyConfComp.m_attackPhase == EnemyAttackPhase_e::MOVE_TO_TARGET_RIGHT)
    {
        moveComp.m_degreeOrientation -= 90.0f;
    }
    moveComp.m_currentDegreeMoveDirection = moveComp.m_degreeOrientation;
}

//===================================================================
void IASystem::treatEnemyBehaviourAttack(uint32_t enemyEntity, MapCoordComponent &enemyMapComp, EnemyConfComponent &enemyConfComp, float distancePlayer)
{
    TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(enemyEntity);
    MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(enemyEntity);
    bool wave = (enemyConfComp.m_type == TypeEnemy_e::LOOP_WAVE_LEFT || enemyConfComp.m_type == TypeEnemy_e::LOOP_WAVE_RIGHT), loop = false;
    if(enemyConfComp.m_type == TypeEnemy_e::STATIC)
    {
        if(++timerComp->m_cycleCountB >= timerComp->m_timeIntervalOptional)
        {
            treatStaticEnemy(enemyConfComp, *moveComp, enemyEntity, distancePlayer);
            timerComp->m_cycleCountB = 0;            
            enemyConfComp.m_currentSprite = enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::ATTACK_LEFT)->second.first;
        }
        return;
    }
    else if(wave || enemyConfComp.m_type == TypeEnemy_e::LOOP_GROUND_HORIZONTAL_LEFT || enemyConfComp.m_type == TypeEnemy_e::LOOP_GROUND_HORIZONTAL_RIGHT ||
               enemyConfComp.m_type == TypeEnemy_e::LOOP_HORIZONTAL)
    {
        loop = true;
        bool right = (moveComp->m_degreeOrientation <= 0.1f);
        if(checkEnemyPlayerDistance(m_playerEntity, enemyEntity, distancePlayer))
        {
            enemyMapComp.m_absoluteMapPositionPX.first += right ? moveComp->m_velocity : -moveComp->m_velocity;
        }
        else
        {
            PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
            MapCoordComponent *playerMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerEntity);
            if(!playerComp->m_invulnerable)
            {
                CollisionSystem *collSystem = Ecsm_t::instance().getSystem<CollisionSystem>(static_cast<uint32_t>(Systems_e::COLLISION_SYSTEM));
                EnemyConfComponent *enemyComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(enemyEntity);
                collSystem->treatPlayerTakeDamage(*enemyComp->m_meleeAttackDamage);
            }
            if(right && enemyMapComp.m_absoluteMapPositionPX.first > playerMapComp->m_absoluteMapPositionPX.first)
            {
                enemyMapComp.m_absoluteMapPositionPX.first += moveComp->m_velocity;
            }
            else if(!right && enemyMapComp.m_absoluteMapPositionPX.first < playerMapComp->m_absoluteMapPositionPX.first)
            {
                enemyMapComp.m_absoluteMapPositionPX.first -= moveComp->m_velocity;
            }
        }
        EnemyConfComponent *enemyComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(enemyEntity);
        if(wave)
        {
            enemyMapComp.m_absoluteMapPositionPX.second += enemyComp->m_waveUp ? -moveComp->m_velocity : moveComp->m_velocity;
            if(++timerComp->m_cycleCountE >= 50)
            {
                timerComp->m_cycleCountE = 0;
                enemyComp->m_waveUp = !enemyComp->m_waveUp;
            }
        }
        if(enemyComp->m_attackPhase == EnemyAttackPhase_e::SHOOTED || enemyComp->m_attackPhase == EnemyAttackPhase_e::SHOOT)
        {
            enemyComp->m_attackPhase = right ? EnemyAttackPhase_e::MOVE_TO_TARGET_RIGHT : EnemyAttackPhase_e::MOVE_TO_TARGET_LEFT;
        }
        if(enemyComp->m_meleeOnly)
        {
            return;
        }
    }
    else if(enemyConfComp.m_type == TypeEnemy_e::LOOP_VERTICAL)
    {
        loop = true;
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(enemyEntity);
        bool up = (moveComp->m_degreeOrientation <= 90.1f);
        mapComp->m_absoluteMapPositionPX.second += up ? -moveComp->m_velocity : moveComp->m_velocity;
        EnemyConfComponent *enemyComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(enemyEntity);
        if(enemyComp->m_attackPhase == EnemyAttackPhase_e::SHOOTED || enemyComp->m_attackPhase == EnemyAttackPhase_e::SHOOT)
        {
            enemyComp->m_attackPhase = up ? EnemyAttackPhase_e::MOVE_TO_TARGET_RIGHT : EnemyAttackPhase_e::MOVE_TO_TARGET_LEFT;
        }
        if(enemyComp->m_meleeOnly)
        {
            return;
        }
    }
    if(!enemyConfComp.m_stuck)
    {
        enemyConfComp.m_previousMove = {EnemyAttackPhase_e::TOTAL, EnemyAttackPhase_e::TOTAL};
    }
    //CHANGING PHASE
    if(enemyConfComp.m_stuck || ++timerComp->m_cycleCountB >= timerComp->m_timeIntervalOptional)
    {
        timerComp->m_cycleCountB = 0;
        if(enemyConfComp.m_countTillLastAttack > 3 && (!enemyConfComp.m_meleeOnly || distancePlayer < 32.0f))
        {
            enemyConfComp.m_attackPhase = EnemyAttackPhase_e::SHOOT;
            enemyConfComp.m_stuck = false;
            updateEnemyDirection(enemyConfComp, *moveComp, enemyMapComp);
            enemyShoot(enemyConfComp, *moveComp, enemyMapComp, distancePlayer);
            activeSound(enemyEntity, static_cast<uint32_t>(EnemySoundEffect_e::ATTACK));
        }
        else
        {
            if(loop)
            {
                ++enemyConfComp.m_countTillLastAttack;
                return;
            }
            enemyConfComp.m_attackPhase = EnemyAttackPhase_e::MOVE_TO_TARGET_LEFT;
        }
        enemyConfComp.m_countTillLastAttack = (enemyConfComp.m_attackPhase == EnemyAttackPhase_e::SHOOT) ? 0 : ++enemyConfComp.m_countTillLastAttack;
        if(loop)
        {
            return;
        }
        while(enemyConfComp.m_stuck)
        {
            if((enemyConfComp.m_attackPhase != std::get<0>(enemyConfComp.m_previousMove) &&
                enemyConfComp.m_attackPhase != std::get<1>(enemyConfComp.m_previousMove)))
            {
                enemyConfComp.m_stuck = false;
            }
            else
            {
                if(enemyConfComp.m_attackPhase == EnemyAttackPhase_e::MOVE_TO_TARGET_RIGHT)
                {
                    enemyConfComp.m_attackPhase = EnemyAttackPhase_e::MOVE_TO_TARGET_LEFT;
                }
                else
                {
                    enemyConfComp.m_attackPhase = EnemyAttackPhase_e::MOVE_TO_TARGET_RIGHT;
                }
            }
        }
        std::swap(std::get<0>(enemyConfComp.m_previousMove), std::get<1>(enemyConfComp.m_previousMove));
        std::get<0>(enemyConfComp.m_previousMove) = enemyConfComp.m_attackPhase;

        std::swap(enemyConfComp.m_previousMove[2], enemyConfComp.m_previousMove[1]);
        std::swap(enemyConfComp.m_previousMove[1], enemyConfComp.m_previousMove[0]);
        enemyConfComp.m_previousMove[0] = enemyConfComp.m_attackPhase;
        timerComp->m_cycleCountB = 0;
    }
    //CONTINUING PHASE
    else if(enemyConfComp.m_attackPhase != EnemyAttackPhase_e::SHOOT)
    {
        if(loop || !checkEnemyPlayerDistance(m_playerEntity, enemyEntity))
        {
            return;
        }
        if(enemyConfComp.m_attackPhase != EnemyAttackPhase_e::SHOOTED)
        {
            MapCoordComponent *playerMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerEntity);
            MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(enemyEntity);
            treatEnemyMove(playerMapComp, *mapComp, moveComp->m_velocity, enemyConfComp, enemyEntity);
            mapComp->m_coord = *getLevelCoord(mapComp->m_absoluteMapPositionPX);
            m_mainEngine->addEntityToZone(enemyEntity, mapComp->m_coord);
        }
    }
}

//===================================================================
bool checkEnemyPlayerDistance(uint32_t playerEntity, uint32_t enemyEntity, float distance)
{
    MapCoordComponent *mapPlayerComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(playerEntity);
    RectangleCollisionComponent *playerComp = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(playerEntity);
    MapCoordComponent *enemyMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(enemyEntity);
    RectangleCollisionComponent *rectComp = Ecsm_t::instance().getComponent<RectangleCollisionComponent, Components_e::RECTANGLE_COLLISION_COMPONENT>(enemyEntity);
    float dist = (rectComp->m_size.first + playerComp->m_size.first) / 2 + 5,
        lateralDist = std::abs((mapPlayerComp->m_absoluteMapPositionPX.first + playerComp->m_size.first / 2) - (enemyMapComp->m_absoluteMapPositionPX.first + rectComp->m_size.first / 2));
    if(distance != 0 && distance > dist)
    {
        return true;
    }
    // return true if enemy don't touch player
    return lateralDist > dist;
}

//===================================================================
void IASystem::treatStaticEnemy(EnemyConfComponent &enemyConfComp, MoveableComponent &moveComp, uint32_t enemyEntity, float distancePlayer)
{
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(enemyEntity);
    assert(mapComp);
    if(enemyConfComp.m_shootingStaticType == StaticEnemyShootBehaviour_e::AIM_PLAYER)
    {
        MapCoordComponent *playerComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerEntity);
        assert(mapComp);
        moveComp.m_degreeOrientation = getTrigoAngle(mapComp->m_absoluteMapPositionPX, playerComp->m_absoluteMapPositionPX);
    }
    enemyShoot(enemyConfComp, moveComp, *mapComp, distancePlayer);
}

//===================================================================
void IASystem::treatEnemyMove(MapCoordComponent *playerMapComp, MapCoordComponent &mapComp, float velocity, EnemyConfComponent &enemyConfComp, uint32_t enemyEntity)
{
    if(enemyConfComp.m_type == TypeEnemy_e::FLYING)
    {
        MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(enemyEntity);
        assert(moveComp);
        moveElementFromAngle(moveComp->m_velocity, getRadiantAngle(moveComp->m_currentDegreeMoveDirection), mapComp.m_absoluteMapPositionPX);
        // moveComp.m_degreeOrientation = moveComp.m_currentDegreeMoveDirection;
        return;
    }
    if(mapComp.m_absoluteMapPositionPX.first < playerMapComp->m_absoluteMapPositionPX.first)
    {
        mapComp.m_absoluteMapPositionPX.first += velocity;
        enemyConfComp.m_attackPhase = EnemyAttackPhase_e::MOVE_TO_TARGET_RIGHT;
    }
    else
    {
        mapComp.m_absoluteMapPositionPX.first -= velocity;
        enemyConfComp.m_attackPhase = EnemyAttackPhase_e::MOVE_TO_TARGET_LEFT;
    }
}

//===================================================================
void IASystem::enemyShoot(EnemyConfComponent &enemyConfComp, MoveableComponent &moveComp,
                          MapCoordComponent &enemyMapComp, float distancePlayer)
{
    if(enemyConfComp.m_meleeAttackDamage && distancePlayer < 32.0f)
    {                
        Ecsm_t::instance().getSystem<CollisionSystem>(static_cast<uint32_t>(Systems_e::COLLISION_SYSTEM))->treatPlayerTakeDamage(*enemyConfComp.m_meleeAttackDamage);
    }
    else if(enemyConfComp.m_visibleShot)
    {
        confVisibleShoot(enemyConfComp.m_visibleAmmo, enemyMapComp.m_absoluteMapPositionPX,
                         moveComp.m_degreeOrientation, CollisionTag_e::BULLET_ENEMY_CT);
    }
}

//===================================================================
void IASystem::memPlayerDatas(uint32_t playerEntity)
{
    m_playerEntity = playerEntity;
}

//===================================================================
void IASystem::confVisibleShoot(std::vector<uint32_t> &visibleShots, const PairFloat_t &point, float degreeAngle, CollisionTag_e tag, bool tank)
{
    uint32_t currentShot = 0;
    assert(!visibleShots.empty());
    GeneralCollisionComponent *genComp;
    for(; currentShot < visibleShots.size(); ++currentShot)
    {
        genComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(visibleShots[currentShot]);
        if(!genComp->m_active)
        {
            break;
        }
        //if all shoot active create a new one
        else if(currentShot == (visibleShots.size() - 1))
        {
            visibleShots.push_back(m_mainEngine->createAmmoEntity(tag, false, tank));
            confNewVisibleShot(visibleShots);
            ++currentShot;
            genComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(visibleShots[currentShot]);
            break;
        }
    }
    PairFloat_t currentPoint = point;
    if(std::cos(getRadiantAngle(degreeAngle)) < EPSILON_FLOAT)
    {
        currentPoint.first += 10;
    }
    m_mainEngine->addEntityToZone(visibleShots[currentShot], *getLevelCoord(point));
    ShotConfComponent *targetShotConfComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(visibleShots[currentShot]);
    assert(targetShotConfComp);
    if(targetShotConfComp->m_ejectMode)
    {
        targetShotConfComp->m_ejectMode = false;
        CircleCollisionComponent *circleTargetComp = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(visibleShots[currentShot]);
        std::swap(targetShotConfComp->m_ejectExplosionRay, circleTargetComp->m_ray);
    }
    targetShotConfComp->m_destructPhase = false;
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(visibleShots[currentShot]);
    MoveableComponent *ammoMoveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(visibleShots[currentShot]);
    TimerComponent *ammoTimeComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(visibleShots[currentShot]);
    genComp->m_active = true;
    ammoTimeComp->m_cycleCountA = 0;
    std::optional<PairUI_t> coord = getLevelCoord(currentPoint);
    assert(coord);
    mapComp->m_coord = *coord;
    SegmentCollisionComponent *segmentComp = Ecsm_t::instance().getComponent<SegmentCollisionComponent, Components_e::SEGMENT_COLLISION_COMPONENT>(visibleShots[currentShot]);
    assert(segmentComp);
    mapComp->m_absoluteMapPositionPX = currentPoint;
    mapComp->m_absoluteMapPositionPX = getShootPoint(currentPoint);
    segmentComp->m_points.first = currentPoint;
    confShotAnim(mapComp->m_absoluteMapPositionPX);
    m_mainEngine->addEntityToZone(visibleShots[currentShot], mapComp->m_coord);
    moveElementFromAngle(LEVEL_HALF_TILE_SIZE_PX, getRadiantAngle(degreeAngle), mapComp->m_absoluteMapPositionPX);
    segmentComp->m_points.second = mapComp->m_absoluteMapPositionPX;
    ammoMoveComp->m_degreeOrientation = degreeAngle;
    ammoMoveComp->m_currentDegreeMoveDirection = degreeAngle;
    if(tank)
    {
        SegmentCollisionComponent *segmentCompB = Ecsm_t::instance().getComponent<SegmentCollisionComponent, Components_e::SEGMENT_COLLISION_COMPONENT>(visibleShots[currentShot], 1);
        assert(segmentCompB);
        segmentCompB->m_points.first = segmentComp->m_points.first;
        segmentCompB->m_points.second = segmentComp->m_points.second;
        //on lateral ground
        if(ammoMoveComp->m_degreeOrientation == 0.0f || ammoMoveComp->m_degreeOrientation == 180.0f)
        {
            segmentCompB->m_points.first.second += 20;
            segmentCompB->m_points.second.second += 20;
        }
        else
        {
            segmentCompB->m_points.first.second += 50;
            segmentCompB->m_points.second.second += 50;
        }
    }
}

//===================================================================
PairFloat_t IASystem::getShootPoint(const PairFloat_t &basePoint)
{
    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    switch (playerComp->getCurrentSpriteType())
    {
    case PlayerSpriteElementType_e::AIM_UP_LOOK_LEFT:
    case PlayerSpriteElementType_e::SHOOT_UP_LOOK_LEFT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::AIM_UP_LOOK_RIGHT:
    case PlayerSpriteElementType_e::SHOOT_UP_LOOK_RIGHT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::SHOOT_UP_RIGHT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::SHOOT_RIGHT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::SHOOT_DOWN_RIGHT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::SHOOT_UP_LEFT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::SHOOT_LEFT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::SHOOT_DOWN_LEFT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::RUN_SHOOT_UP_RIGHT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::RUN_SHOOT_RIGHT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::RUN_SHOOT_DOWN_RIGHT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::RUN_SHOOT_UP_LEFT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::RUN_SHOOT_LEFT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::RUN_SHOOT_DOWN_LEFT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::JUMP_SHOOT_UP_LOOK_RIGHT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::JUMP_SHOOT_UP_RIGHT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::JUMP_SHOOT_DOWN_RIGHT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::JUMP_SHOOT_DOWN_LOOK_RIGHT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::JUMP_SHOOT_UP_LOOK_LEFT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::JUMP_SHOOT_UP_LEFT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::JUMP_SHOOT_LEFT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::JUMP_SHOOT_DOWN_LEFT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    case PlayerSpriteElementType_e::JUMP_SHOOT_DOWN_LOOK_LEFT:
        return {basePoint.first + 2.0f, basePoint.second - 10.0f};
    default:
        break;
    }
    return basePoint;
}

//===================================================================
void IASystem::confShotAnim(const PairFloat_t &currentPoint)
{
    PlayerConfComponent *playerComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    uint32_t shotAnimEntity = playerComp->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::SHOT_ANIM)];
    TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(shotAnimEntity);
    assert(timerComp);
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(shotAnimEntity);
    assert(spriteComp);
    MemSpriteDataComponent *memSpriteComp = Ecsm_t::instance().getComponent<MemSpriteDataComponent, Components_e::MEM_SPRITE_DATA_COMPONENT>(shotAnimEntity);
    GeneralCollisionComponent *genCollComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(shotAnimEntity);
    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(shotAnimEntity);
    mapComp->m_absoluteMapPositionPX = currentPoint;
    assert(genCollComp);
    assert(memSpriteComp);
    memSpriteComp->m_current = 0;
    spriteComp->m_spriteData = memSpriteComp->m_vectSpriteData[0];
    timerComp->m_cycleCountA = 0;
    genCollComp->m_active = true;
}

//===================================================================
void IASystem::confEnemiesGenerator(uint32_t generatorEntity, const PairFloat_t &point)
{
    GeneratorComponent *generatorComp = Ecsm_t::instance().getComponent<GeneratorComponent, Components_e::GENERATOR_COMPONENT>(generatorEntity);
    assert(generatorComp);
    for(uint32_t i = 0; i < generatorComp->m_vectElementGen.size(); ++i)
    {
        GeneralCollisionComponent *collComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(generatorComp->m_vectElementGen[i]);
        assert(collComp);
        if(collComp->m_active)
        {
            continue;
        }
        collComp->m_active = true;
        collComp->m_tagA = CollisionTag_e::ENEMY_CT;
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(generatorComp->m_vectElementGen[i]);
        assert(mapComp);
        mapComp->m_absoluteMapPositionPX = point;
        mapComp->m_coord = *getLevelCoord(mapComp->m_absoluteMapPositionPX);
        m_mainEngine->addEntityToZone(generatorComp->m_vectElementGen[i], mapComp->m_coord);
        EnemyConfComponent *enemyComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(generatorComp->m_vectElementGen[i]);
        assert(enemyComp);
        enemyComp->m_life = generatorComp->m_memEnemyLife;
        enemyComp->m_displayMode = EnemyDisplayMode_e::NORMAL;
        enemyComp->m_behaviourMode = EnemyBehaviourMode_e::PASSIVE;
        enemyComp->m_currentSprite = enemyComp->m_mapSpriteAssociate.find(EnemySpriteType_e::STATIC_LEFT)->second.first;
        GravityComponent *gravComp = Ecsm_t::instance().getComponent<GravityComponent, Components_e::GRAVITY_COMPONENT>(generatorComp->m_vectElementGen[i]);
        assert(gravComp);
        gravComp->m_freeze = false;
        break;
    }
}

//===================================================================
void IASystem::confNewVisibleShot(const std::vector<uint32_t> &visibleShots)
{
    assert(visibleShots.size() > 1);
    uint32_t targetIndex = visibleShots.size() - 1, baseIndex = targetIndex - 1;
    MemSpriteDataComponent *baseMemSpriteComp = Ecsm_t::instance().getComponent<MemSpriteDataComponent, Components_e::MEM_SPRITE_DATA_COMPONENT>(visibleShots[baseIndex]);
    SpriteTextureComponent *baseSpriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(visibleShots[baseIndex]);
    SpriteTextureComponent *targetSpriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(visibleShots[targetIndex]);
    MemSpriteDataComponent *targetMemSpriteComp = Ecsm_t::instance().getComponent<MemSpriteDataComponent, Components_e::MEM_SPRITE_DATA_COMPONENT>(visibleShots[targetIndex]);
    ShotConfComponent *baseShotConfComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(visibleShots[baseIndex]);
    ShotConfComponent *targetShotConfComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(visibleShots[targetIndex]);
    MoveableComponent *baseMoveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(visibleShots[baseIndex]);
    MoveableComponent *targetMoveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(visibleShots[targetIndex]);

    AudioComponent *audioCompTarget = Ecsm_t::instance().getComponent<AudioComponent, Components_e::AUDIO_COMPONENT>(visibleShots[targetIndex]);
    AudioComponent *audioCompBase = Ecsm_t::instance().getComponent<AudioComponent, Components_e::AUDIO_COMPONENT>(visibleShots[baseIndex]);
    audioCompTarget->m_soundElements.push_back(SoundElement());
    audioCompTarget->m_soundElements[0]->m_toPlay = true;
    audioCompTarget->m_soundElements[0]->m_bufferALID = audioCompBase->m_soundElements[0]->m_bufferALID;    
    audioCompTarget->m_soundElements[0]->m_sourceALID =  Ecsm_t::instance().getSystem<SoundSystem>(static_cast<uint32_t>(Systems_e::SOUND_SYSTEM))->createSource(audioCompBase->m_soundElements[0]->m_bufferALID);
    targetMemSpriteComp->m_vectSpriteData = baseMemSpriteComp->m_vectSpriteData;
    targetSpriteComp->m_spriteData = targetMemSpriteComp->m_vectSpriteData[0];
    targetSpriteComp->m_displaySize = baseSpriteComp->m_displaySize;
    targetMoveComp->m_velocity = baseMoveComp->m_velocity;
    targetShotConfComp->m_damage = baseShotConfComp->m_damage;
    targetShotConfComp->m_spriteTotal = baseShotConfComp->m_spriteTotal;
    float maxWidth = EPSILON_FLOAT;
    targetShotConfComp->m_ejectExplosionRay = maxWidth * LEVEL_HALF_TILE_SIZE_PX;
}
