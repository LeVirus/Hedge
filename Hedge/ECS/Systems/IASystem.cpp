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
#include <cassert>
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
void IASystem::execSystem()
{
    PlayerConfComponent *playerConfComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
    WeaponComponent *weaponComp = Ecsm_t::instance().getComponent<WeaponComponent, Components_e::WEAPON_COMPONENT>(playerConfComp->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::WEAPON)]);
    treatEject();
    for(uint32_t i = 0; i < weaponComp->m_weaponsData.size(); ++i)
    {
        if(weaponComp->m_weaponsData[i].m_attackType == AttackType_e::VISIBLE_SHOTS)
        {
            assert(weaponComp->m_weaponsData[i].m_visibleShootEntities);
            treatVisibleShots(*weaponComp->m_weaponsData[i].m_visibleShootEntities);
        }
    }
    float distancePlayer;
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
        MapCoordComponent *playerMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(m_playerEntity);
        MapCoordComponent *enemyMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(*it);
        distancePlayer = getDistance(playerMapComp->m_absoluteMapPositionPX,
                                     enemyMapComp->m_absoluteMapPositionPX);
        if(enemyConfComp->m_behaviourMode != EnemyBehaviourMode_e::ATTACK)
        {
            TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(*it);
        }
        if(enemyConfComp->m_behaviourMode == EnemyBehaviourMode_e::ATTACK)
        {
            treatEnemyBehaviourAttack(*it, *enemyMapComp, *enemyConfComp, distancePlayer);
        }
    }
}

//===================================================================
void IASystem::treatVisibleShots(const std::vector<uint32_t> &stdAmmo)
{
    for(uint32_t i = 0; i < stdAmmo.size(); ++i)
    {
        treatVisibleShot(stdAmmo[i]);
    }
}

//===================================================================
void IASystem::treatVisibleShot(uint32_t numEntity)
{
    GeneralCollisionComponent *genColl = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(numEntity);
    if(!genColl->m_active)
    {
        return;
    }
    ShotConfComponent *shotComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(numEntity);
    if(shotComp->m_destructPhase)
    {
        return;
    }
    TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(numEntity);
    if(++timerComp->m_cycleCountA > m_intervalVisibleShotLifeTime)
    {
        genColl->m_active = false;
        timerComp->m_cycleCountA = 0;
        return;
    }
    MapCoordComponent *ammoMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(numEntity);
    MoveableComponent *ammoMoveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(numEntity);
    assert(genColl->m_shape == CollisionShape_e::CIRCLE_C);
    moveElementFromAngle(ammoMoveComp->m_velocity, getRadiantAngle(ammoMoveComp->m_degreeOrientation),
                         ammoMapComp->m_absoluteMapPositionPX);
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
    moveComp.m_degreeOrientation = getTrigoAngle(enemyMapComp.m_absoluteMapPositionPX, playerMapComp->m_absoluteMapPositionPX);
    if(enemyConfComp.m_attackPhase == EnemyAttackPhase_e::MOVE_TO_TARGET_DIAG_RIGHT)
    {
        moveComp.m_degreeOrientation -= 30.0f;
    }
    else if(enemyConfComp.m_attackPhase == EnemyAttackPhase_e::MOVE_TO_TARGET_DIAG_LEFT)
    {
        moveComp.m_degreeOrientation += 30.0f;
    }
    else if(enemyConfComp.m_attackPhase == EnemyAttackPhase_e::MOVE_TO_TARGET_LEFT)
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
    MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(enemyEntity);
    TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(enemyEntity);
    if(!enemyConfComp.m_stuck)
    {
        enemyConfComp.m_previousMove = {EnemyAttackPhase_e::TOTAL, EnemyAttackPhase_e::TOTAL};
    }
    if(enemyConfComp.m_stuck || ++timerComp->m_cycleCountB >= m_intervalEnemyBehaviour)
    {
        if(enemyConfComp.m_countTillLastAttack > 3 && (!enemyConfComp.m_meleeOnly || distancePlayer < 32.0f))
        {
            enemyConfComp.m_attackPhase = EnemyAttackPhase_e::SHOOT;
            enemyConfComp.m_stuck = false;
        }
        else
        {
            uint32_t modulo = (enemyConfComp.m_meleeOnly || enemyConfComp.m_countTillLastAttack < 2) ? static_cast<uint32_t>(EnemyAttackPhase_e::SHOOT) :
                                                                           static_cast<uint32_t>(EnemyAttackPhase_e::SHOOT) + 1;
            enemyConfComp.m_attackPhase = static_cast<EnemyAttackPhase_e>(std::rand() / ((RAND_MAX + 1u) / modulo));
        }
        enemyConfComp.m_countTillLastAttack =
                (enemyConfComp.m_attackPhase == EnemyAttackPhase_e::SHOOT) ? 0 : ++enemyConfComp.m_countTillLastAttack;

        while(enemyConfComp.m_stuck)
        {
            if((enemyConfComp.m_attackPhase != std::get<0>(enemyConfComp.m_previousMove) &&
                enemyConfComp.m_attackPhase != std::get<1>(enemyConfComp.m_previousMove)))
            {
                enemyConfComp.m_stuck = false;
            }
            else
            {
                if(enemyConfComp.m_attackPhase == EnemyAttackPhase_e::MOVE_TO_TARGET_FRONT)
                {
                    enemyConfComp.m_attackPhase = EnemyAttackPhase_e::MOVE_TO_TARGET_DIAG_LEFT;
                }
                else
                {
                    enemyConfComp.m_attackPhase = static_cast<EnemyAttackPhase_e>(static_cast<uint32_t>(enemyConfComp.m_attackPhase) - 1);
                }
            }
        }
        std::swap(std::get<0>(enemyConfComp.m_previousMove), std::get<1>(enemyConfComp.m_previousMove));
        std::get<0>(enemyConfComp.m_previousMove) = enemyConfComp.m_attackPhase;

        std::swap(enemyConfComp.m_previousMove[2], enemyConfComp.m_previousMove[1]);
        std::swap(enemyConfComp.m_previousMove[1], enemyConfComp.m_previousMove[0]);
        enemyConfComp.m_previousMove[0] = enemyConfComp.m_attackPhase;
        timerComp->m_cycleCountB = 0;
        updateEnemyDirection(enemyConfComp, *moveComp, enemyMapComp);
        if(enemyConfComp.m_attackPhase == EnemyAttackPhase_e::SHOOT)
        {
            enemyShoot(enemyConfComp, *moveComp, enemyMapComp, distancePlayer);
            activeSound(enemyEntity, static_cast<uint32_t>(EnemySoundEffect_e::ATTACK));
        }
    }
    else if(enemyConfComp.m_attackPhase != EnemyAttackPhase_e::SHOOT && distancePlayer > LEVEL_TILE_SIZE_PX)
    {
        if(enemyConfComp.m_attackPhase != EnemyAttackPhase_e::SHOOTED)
        {
            moveElementFromAngle(moveComp->m_velocity, getRadiantAngle(moveComp->m_degreeOrientation),
                                 enemyMapComp.m_absoluteMapPositionPX);
            MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(enemyEntity);
            m_mainEngine->addEntityToZone(enemyEntity, *getLevelCoord(mapComp->m_absoluteMapPositionPX));
        }
    }
}

//===================================================================
void IASystem::enemyShoot(EnemyConfComponent &enemyConfComp, MoveableComponent &moveComp,
                          MapCoordComponent &enemyMapComp, float distancePlayer)
{
    if(enemyConfComp.m_meleeAttackDamage && distancePlayer < 32.0f)
    {
        PlayerConfComponent *playerConfComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(m_playerEntity);
        playerConfComp->takeDamage(*enemyConfComp.m_meleeAttackDamage);
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
void IASystem::confVisibleShoot(std::vector<uint32_t> &visibleShots, const PairFloat_t &point, float degreeAngle, CollisionTag_e tag)
{
    uint32_t currentShot = 0;
    assert(!visibleShots.empty());
    GeneralCollisionComponent *genComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(visibleShots[currentShot]);
    for(; currentShot < visibleShots.size(); ++currentShot)
    {
        if(!genComp->m_active)
        {
            break;
        }
        //if all shoot active create a new one
        else if(currentShot == (visibleShots.size() - 1))
        {
            visibleShots.push_back(m_mainEngine->createAmmoEntity(tag, true));
            confNewVisibleShot(visibleShots);
            ++currentShot;
            genComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(visibleShots[currentShot]);
            break;
        }
    }
    ShotConfComponent *targetShotConfComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(visibleShots[currentShot]);
    if(targetShotConfComp->m_ejectMode)
    {
        targetShotConfComp->m_ejectMode = false;
        CircleCollisionComponent *circleTargetComp = Ecsm_t::instance().getComponent<CircleCollisionComponent, Components_e::CIRCLE_COLLISION_COMPONENT>(visibleShots[currentShot]);
        std::swap(targetShotConfComp->m_ejectExplosionRay, circleTargetComp->m_ray);
    }

    MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(visibleShots[currentShot]);
    MoveableComponent *ammoMoveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(visibleShots[currentShot]);
    TimerComponent *ammoTimeComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(visibleShots[currentShot]);
    genComp->m_active = true;
    ammoTimeComp->m_cycleCountA = 0;
    std::optional<PairUI_t> coord = getLevelCoord(point);
    assert(coord);
    mapComp->m_coord = *coord;
    mapComp->m_absoluteMapPositionPX = point;
    m_mainEngine->addEntityToZone(visibleShots[currentShot], mapComp->m_coord);
    moveElementFromAngle(LEVEL_HALF_TILE_SIZE_PX, getRadiantAngle(degreeAngle),
                         mapComp->m_absoluteMapPositionPX);
    ammoMoveComp->m_degreeOrientation = degreeAngle;
    ammoMoveComp->m_currentDegreeMoveDirection = degreeAngle;
}

//===================================================================
void IASystem::confNewVisibleShot(const std::vector<uint32_t> &visibleShots)
{
    assert(visibleShots.size() > 1);
    uint32_t targetIndex = visibleShots.size() - 1, baseIndex = targetIndex - 1;
    MemSpriteDataComponent *baseMemSpriteComp = Ecsm_t::instance().getComponent<MemSpriteDataComponent, Components_e::MEM_SPRITE_DATA_COMPONENT>(visibleShots[baseIndex]);
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
    targetMoveComp->m_velocity = baseMoveComp->m_velocity;
    targetShotConfComp->m_damage = baseShotConfComp->m_damage;
    float maxWidth = EPSILON_FLOAT;
    targetShotConfComp->m_ejectExplosionRay = maxWidth * LEVEL_HALF_TILE_SIZE_PX;
}
