#include <cassert>
#include "VisionSystem.hpp"
#include <constants.hpp>
#include <MainEngine.hpp>
#include <CollisionUtils.hpp>
#include <PhysicalEngine.hpp>
#include <math.h>
#include <alias.hpp>
#include <ECS/Systems/IASystem.hpp>
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/MapCoordComponent.hpp>
#include <ECS/Components/GeneralCollisionComponent.hpp>
#include <ECS/Components/CircleCollisionComponent.hpp>
#include <ECS/Components/RectangleCollisionComponent.hpp>
#include <ECS/Components/MoveableComponent.hpp>
#include <ECS/Components/MemSpriteDataComponent.hpp>
#include <ECS/Components/SpriteTextureComponent.hpp>
#include <ECS/Components/EnemyConfComponent.hpp>
#include <ECS/Components/TimerComponent.hpp>
#include <ECS/Components/ShotConfComponent.hpp>
#include <ECS/Components/WallMultiSpriteComponent.hpp>

//===========================================================================
VisionSystem::VisionSystem()
{
    setUsedComponents();
}

//===========================================================================
void VisionSystem::setUsedComponents()
{
    addComponentsToSystem(Components_e::MAP_COORD_COMPONENT, 1);
    addComponentsToSystem(Components_e::SPRITE_TEXTURE_COMPONENT, 1);
    addComponentsToSystem(Components_e::GENERAL_COLLISION_COMPONENT, 1);
}

//===========================================================================
void VisionSystem::execSystem()
{
    for(std::set<uint32_t>::iterator it = m_usedEntities.begin(); it != m_usedEntities.end(); ++it)
    {
        MemSpriteDataComponent *memSpriteComp = Ecsm_t::instance().getComponent<MemSpriteDataComponent, Components_e::MEM_SPRITE_DATA_COMPONENT>(*it);
        if(!memSpriteComp)
        {
            continue;
        }
        GeneralCollisionComponent *genComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(*it);
        if(!genComp->m_active)
        {
            continue;
        }
        SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(*it);
        TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(*it);
        if(genComp->m_tagA == CollisionTag_e::BULLET_ENEMY_CT || genComp->m_tagA == CollisionTag_e::BULLET_PLAYER_CT || genComp->m_tagA == CollisionTag_e::VEHICULE_CT)
        {
            updateVisibleShotSprite(*it, *memSpriteComp, *spriteComp, *timerComp, *genComp);
        }
        //OOOOK put enemy tag to tagB
        else if(genComp->m_tagA == CollisionTag_e::ENEMY_CT || genComp->m_tagA == CollisionTag_e::GHOST_CT)
        {
            EnemyConfComponent *enemyConfComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(*it);
            if(enemyConfComp)
            {
                updateEnemySprites(*it, *memSpriteComp, *spriteComp, *timerComp, *enemyConfComp);
            }
        }
        else if(genComp->m_tagA == CollisionTag_e::PLAYER_CT)
        {
            updatePlayerSprites(*it, *memSpriteComp, *spriteComp, *timerComp);
        }
    }
    updateWallSprites();
}

//===========================================================================
void VisionSystem::updateExitVehicleSprites(uint32_t vehicleEntity, VehicleComponent &vehicleComp)
{
    vehicleComp.m_currentShootAnimation = false;
    if(vehicleComp.m_currentSpritesType == VehicleSpriteType_e::SHOOT_MOVE_LEFT)
    {
        vehicleComp.m_currentSpritesType = VehicleSpriteType_e::MOVE_LEFT;
    }
    else if(vehicleComp.m_currentSpritesType == VehicleSpriteType_e::SHOOT_MOVE_RIGHT)
    {
        vehicleComp.m_currentSpritesType = VehicleSpriteType_e::MOVE_RIGHT;
    }
    else if(vehicleComp.m_currentSpritesType == VehicleSpriteType_e::SHOOT_STAIR_DOWN_RIGHT)
    {
        vehicleComp.m_currentSpritesType = VehicleSpriteType_e::STAIR_DOWN_RIGHT;
    }
    else if(vehicleComp.m_currentSpritesType == VehicleSpriteType_e::SHOOT_STAIR_UP_RIGHT)
    {
        vehicleComp.m_currentSpritesType = VehicleSpriteType_e::STAIR_UP_RIGHT;
    }
    else if(vehicleComp.m_currentSpritesType == VehicleSpriteType_e::SHOOT_STAIR_DOWN_LEFT)
    {
        vehicleComp.m_currentSpritesType = VehicleSpriteType_e::STAIR_DOWN_LEFT;
    }
    else if(vehicleComp.m_currentSpritesType == VehicleSpriteType_e::SHOOT_STAIR_UP_LEFT)
    {
        vehicleComp.m_currentSpritesType = VehicleSpriteType_e::STAIR_UP_LEFT;
    }
    updateVehicleSprites(vehicleEntity, vehicleComp);
}

//===========================================================================
void VisionSystem::updateWallSprites()
{
    if(m_memMultiSpritesWallEntities.empty())
    {
        memMultiSpritesWallEntities();
    }
    float currentInterval;
    for(uint32_t i = 0; i < m_memMultiSpritesWallEntities.size(); ++i)
    {
        SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(m_memMultiSpritesWallEntities[i]);
        assert(spriteComp);
        TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(m_memMultiSpritesWallEntities[i]);
        assert(timerComp);
        MemSpriteDataComponent *memSpriteComp = Ecsm_t::instance().getComponent<MemSpriteDataComponent, Components_e::MEM_SPRITE_DATA_COMPONENT>(m_memMultiSpritesWallEntities[i]);
        assert(memSpriteComp);
        WallMultiSpriteComponent *multiSpriteConf = Ecsm_t::instance().getComponent<WallMultiSpriteComponent, Components_e::WALL_MULTI_SPRITE_CONF_COMPONENT>(m_memMultiSpritesWallEntities[i]);
        assert(multiSpriteConf);
        if(!multiSpriteConf->m_cyclesTime.empty())
        {
            assert(memSpriteComp->m_current < multiSpriteConf->m_cyclesTime.size());
            currentInterval = multiSpriteConf->m_cyclesTime[memSpriteComp->m_current];
        }
        else
        {
            currentInterval = m_defaultInterval;
        }
        if(++timerComp->m_cycleCountA >= currentInterval)
        {
            ++memSpriteComp->m_current;
            if(memSpriteComp->m_current >= memSpriteComp->m_vectSpriteData.size())
            {
                memSpriteComp->m_current = 0;
            }
            spriteComp->m_spriteData = memSpriteComp->m_vectSpriteData[memSpriteComp->m_current];
            if(!multiSpriteConf->m_elec.empty())
            {
                GeneralCollisionComponent *collComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(m_memMultiSpritesWallEntities[i]);
                assert(collComp);
                collComp->m_tagA = multiSpriteConf->m_elec[memSpriteComp->m_current] ? CollisionTag_e::ELECTRIC_WALL_CT : CollisionTag_e::WALL_CT;
            }
            if(!multiSpriteConf->m_appear.empty())
            {
                GeneralCollisionComponent *collComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(m_memMultiSpritesWallEntities[i]);
                assert(collComp);
                if(!multiSpriteConf->m_appear[memSpriteComp->m_current])
                {
                    collComp->m_tagA = CollisionTag_e::GHOST_CT;
                }
                if(multiSpriteConf->m_appear[memSpriteComp->m_current] && collComp->m_tagA == CollisionTag_e::GHOST_CT)
                {
                    collComp->m_tagA = CollisionTag_e::WALL_CT;
                }
            }
            timerComp->m_cycleCountA = 0;
        }
    }
}

//===========================================================================
void VisionSystem::memMultiSpritesWallEntities()
{
    std::array<uint32_t, Components_e::TOTAL_COMPONENTS> arrayComp;
    std::set<uint32_t> set;
    arrayComp.fill(0);
    arrayComp[Components_e::MAP_COORD_COMPONENT] = 1;
    arrayComp[Components_e::SPRITE_TEXTURE_COMPONENT] = 1;
    arrayComp[Components_e::MEM_SPRITE_DATA_COMPONENT] = 1;
    arrayComp[Components_e::TIMER_COMPONENT] = 1;
    set.insert(Components_e::MAP_COORD_COMPONENT);
    set.insert(Components_e::SPRITE_TEXTURE_COMPONENT);
    set.insert(Components_e::MEM_SPRITE_DATA_COMPONENT);
    set.insert(Components_e::TIMER_COMPONENT);
    std::optional<std::set<uint32_t>> vectEntities = Ecsm_t::instance().getEntitiesCustomComponents(set, arrayComp);
    assert(vectEntities);
    for(std::set<uint32_t>::const_iterator it = (*vectEntities).begin(); it != (*vectEntities).end(); ++it)
    {
        GeneralCollisionComponent *genComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(*it);
        assert(genComp);
        if(genComp->m_tagA == CollisionTag_e::WALL_CT)
        {
            m_memMultiSpritesWallEntities.push_back(*it);
        }
    }
}

//===========================================================================
void VisionSystem::updateVisibleShotSprite(uint32_t shotEntity, MemSpriteDataComponent &memSpriteComp, SpriteTextureComponent &spriteComp,
                                           TimerComponent &timerComp, GeneralCollisionComponent &genComp)
{
    ShotConfComponent *shotComp = Ecsm_t::instance().getComponent<ShotConfComponent, Components_e::SHOT_CONF_COMPONENT>(shotEntity);
    if(!shotComp->m_destructPhase)
    {
        return;
    }
    if(++timerComp.m_cycleCountA >= shotComp->m_cycleDestructNumber)
    {
        timerComp.m_cycleCountA = 0;
        if(shotComp->m_spriteShotNum < shotComp->m_spriteTotal - 1 /*memSpriteComp.m_vectSpriteData.size()*/ /*- 1*/)
        {
            ++shotComp->m_spriteShotNum;
        }
        else
        {
            genComp.m_active = false;
            shotComp->m_destructPhase = false;
            shotComp->m_spriteShotNum = 0;
        }
    }
    spriteComp.m_spriteData = memSpriteComp.m_vectSpriteData[shotComp->m_spriteShotNum];
}

//===========================================================================
void VisionSystem::updatePlayerSprites(uint32_t playerEntity, MemSpriteDataComponent &memSpriteComp, SpriteTextureComponent &spriteComp, TimerComponent &timerComp)
{
    PlayerConfComponent *playerConfComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(playerEntity);
    if(Level::getDialogMode() || playerConfComp->m_frozen)
    {
        return;
    }
    MapPlayerSprite_t::const_iterator it = playerConfComp->m_mapSpriteAssociate.find(playerConfComp->getCurrentSpriteType());
    if(playerConfComp->m_memPreviousSprite)
    {
        playerConfComp->m_currentSprite = it->second.first + *playerConfComp->m_memPreviousSprite;
        playerConfComp->m_memPreviousSprite = {};
    }
    else
    {
        //if sprite outside ==> change sprite type
        if(playerConfComp->m_currentSprite < it->second.first ||
            playerConfComp->m_currentSprite > it->second.second)
        {
            playerConfComp->m_currentSprite = it->second.first;
            if(!getPreviousCycleCount({playerConfComp->getCurrentSpriteType(), playerConfComp->getPreviousSpriteType()}))
            {
                timerComp.m_cycleCountD = 0;
                playerConfComp->m_countAnimationCycle = 0;
            }
            //Mem previous cycle count for run anim
            else
            {
                playerConfComp->m_currentSprite += playerConfComp->m_countAnimationCycle;
                ++timerComp.m_cycleCountD;
            }
        }
        else if(++timerComp.m_cycleCountD > playerConfComp->m_standardSpriteInterval)
        {
            if(playerConfComp->m_currentSprite == it->second.second)
            {
                playerConfComp->m_currentSprite = it->second.first;
                playerConfComp->m_countAnimationCycle = 0;
                if(playerConfComp->getCurrentSpriteType() == PlayerSpriteElementType_e::JUMP_RIGHT || playerConfComp->getCurrentSpriteType() == PlayerSpriteElementType_e::JUMP_LEFT)
                {
                    ++playerConfComp->m_currentSprite;
                }
            }
            else
            {
                ++playerConfComp->m_currentSprite;
                ++playerConfComp->m_countAnimationCycle;
            }
            timerComp.m_cycleCountD = 0;
        }
    }
    spriteComp.m_spriteData = memSpriteComp.m_vectSpriteData[static_cast<uint32_t>(playerConfComp->m_currentSprite)];
    updateShotAnimSprite(playerConfComp, playerEntity);
    if(playerConfComp->m_associatedVehicle)
    {
        VehicleComponent *vehicleComp = Ecsm_t::instance().getComponent<VehicleComponent, Components_e::VEHICLE_COMPONENT>(*playerConfComp->m_associatedVehicle);
        assert(vehicleComp);
        if(!vehicleComp->m_onStair && vehicleComp->m_stairCount > 3)
        {
            updateVehicleGroundSprites(*playerConfComp, *vehicleComp);
        }
        updateVehicleSprites(*playerConfComp->m_associatedVehicle, *vehicleComp);
    }
}

//===========================================================================
void VisionSystem::updateShotAnimSprite(PlayerConfComponent *playerConfComp, uint32_t playerEntity)
{
    uint32_t shotAnimEntity = playerConfComp->m_vectEntities[static_cast<uint32_t>(PlayerEntities_e::SHOT_ANIM)];
    GeneralCollisionComponent *collComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(shotAnimEntity);
    if(collComp->m_active)
    {
        TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(shotAnimEntity);
        if(++timerComp->m_cycleCountA > *timerComp->m_timeIntervalOptional)
        {
            MemSpriteDataComponent *memSpriteComp = Ecsm_t::instance().getComponent<MemSpriteDataComponent, Components_e::MEM_SPRITE_DATA_COMPONENT>(shotAnimEntity);
            if(++memSpriteComp->m_current < memSpriteComp->m_vectSpriteData.size())
            {
                SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(shotAnimEntity);
                spriteComp->m_spriteData = memSpriteComp->m_vectSpriteData[memSpriteComp->m_current];
                timerComp->m_cycleCountA = 0;
            }
            else
            {
                //Stop anim
                collComp->m_active = false;
            }
        }
        MapCoordComponent *playerMapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(playerEntity);
        MapCoordComponent *mapComp = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(shotAnimEntity);
        mapComp->m_absoluteMapPositionPX = Ecsm_t::instance().getSystem<IASystem>(static_cast<uint32_t>(Systems_e::IA_SYSTEM))->getShootPoint(playerMapComp->m_absoluteMapPositionPX);
    }
}

//===========================================================================
bool getPreviousCycleCount(const std::pair<PlayerSpriteElementType_e, PlayerSpriteElementType_e> &playerSpriteType)
{
    MultiMapAssociatedPlayerSpriteType_t::const_iterator it = MAP_PLAYER_PREVIOUS_SPRITE_ASSOCIATED.find(playerSpriteType.first);
    if(it != MAP_PLAYER_PREVIOUS_SPRITE_ASSOCIATED.end())
    {
        do
        {
            if(it->second == playerSpriteType.second)
            {
                return true;
            }
            ++it;
        }while(it != MAP_PLAYER_PREVIOUS_SPRITE_ASSOCIATED.end() && it->first == playerSpriteType.first);
    }
    return false;
}


//===========================================================================
void VisionSystem::updateVehicleGroundSprites(PlayerConfComponent &playerComp, VehicleComponent &vehicleComp)
{
    bool currentShoot = vehicleComp.m_currentShootAnimation ? true: false;
    VehicleSpriteType_e previous;
    if(playerComp.m_currentDirectionRight)
    {
        previous = currentShoot ? VehicleSpriteType_e::SHOOT_MOVE_RIGHT : VehicleSpriteType_e::MOVE_RIGHT;
    }
    else
    {
        previous = currentShoot ? VehicleSpriteType_e::SHOOT_MOVE_LEFT : VehicleSpriteType_e::MOVE_LEFT;
    }
    if(previous == vehicleComp.m_currentSpritesType)
    {
        return;
    }
    vehicleComp.m_currentSpritesType = previous;
    if(currentShoot)
    {
        MapVehicleSprite_t::const_iterator it = vehicleComp.m_mapSpriteAssociate.find(vehicleComp.m_currentSpritesType);
        vehicleComp.m_currentSprite = it->second.first + vehicleComp.m_shootCount;
    }
}

//===========================================================================
void VisionSystem::updateVehicleSprites(uint32_t vehicleEntity, VehicleComponent &vehicleComp)
{
    TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(vehicleEntity);
    assert(timerComp);
    SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(vehicleEntity);
    assert(spriteComp);
    MemSpriteDataComponent *memSpriteComp = Ecsm_t::instance().getComponent<MemSpriteDataComponent, Components_e::MEM_SPRITE_DATA_COMPONENT>(vehicleEntity);
    assert(memSpriteComp);
    MapVehicleSprite_t::const_iterator it = vehicleComp.m_mapSpriteAssociate.find(vehicleComp.m_currentSpritesType);
    //if sprite outside
    if(vehicleComp.m_currentSprite < it->second.first || vehicleComp.m_currentSprite > it->second.second)
    {
        vehicleComp.m_currentSprite = it->second.first;
        timerComp->m_cycleCountD = 0;
    }
    else if(++timerComp->m_cycleCountD > vehicleComp.m_spriteInterval)
    {
        if(vehicleComp.m_currentSprite == it->second.second)
        {
            vehicleComp.m_currentSprite = it->second.first;
        }
        else
        {
            if(vehicleComp.m_currentSprite == it->second.second - 1)
            {
                vehicleComp.m_currentShootAnimation = false;
            }
            ++vehicleComp.m_currentSprite;
            ++vehicleComp.m_shootCount;
        }
        timerComp->m_cycleCountD = 0;
    }
    spriteComp->m_spriteData = memSpriteComp->m_vectSpriteData[vehicleComp.m_currentSprite];
}

//===========================================================================
void VisionSystem::updateEnemySprites(uint32_t enemyEntity,
                                      MemSpriteDataComponent &memSpriteComp,
                                      SpriteTextureComponent &spriteComp,
                                      TimerComponent &timerComp, EnemyConfComponent &enemyConfComp)
{
    if(enemyConfComp.m_touched)
    {
        if(enemyConfComp.m_life < 1000)
        {
            if(enemyConfComp.m_currentDirRight)
            {
                mapEnemySprite_t::const_iterator it = enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::TOUCHED_RIGHT);
                if(it != enemyConfComp.m_mapSpriteAssociate.end())
                {
                    enemyConfComp.m_currentSprite = enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::TOUCHED_RIGHT)->second.first;
                }
                else
                {
                    enemyConfComp.m_currentSprite = enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::TOUCHED)->second.first;
                }
            }
            else
            {
                enemyConfComp.m_currentSprite = enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::TOUCHED)->second.first;
            }
        }
        if(++timerComp.m_cycleCountC >= enemyConfComp.m_cycleNumberSpriteUpdate)
        {
            enemyConfComp.m_touched = false;
        }
    }
    else if(enemyConfComp.m_behaviourMode == EnemyBehaviourMode_e::ATTACK &&
            enemyConfComp.m_attackPhase == EnemyAttackPhase_e::SHOOT)
    {
        MoveableComponent *moveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(enemyEntity);
        updateEnemyAttackSprite(enemyConfComp, timerComp, moveComp->m_currentDegreeMoveDirection);
    }
    else if(enemyConfComp.m_displayMode == EnemyDisplayMode_e::NORMAL)
    {
        updateEnemyNormalSprite(enemyConfComp, timerComp, enemyEntity);
    }
    else if(enemyConfComp.m_displayMode == EnemyDisplayMode_e::DYING)
    {
        mapEnemySprite_t::const_iterator it = enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::DYING),
            itt = enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::DYING_RIGHT);
        if(enemyConfComp.m_currentSprite == it->second.second || (itt != enemyConfComp.m_mapSpriteAssociate.end() && enemyConfComp.m_currentSprite == itt->second.second))
        {
            enemyConfComp.m_displayMode = EnemyDisplayMode_e::DEAD;
            if(enemyConfComp.m_endLevel)
            {
                assert(m_refMainEngine);
                m_refMainEngine->activeEndLevel();
            }
            GeneralCollisionComponent *collComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(enemyEntity);
            assert(collComp);
            collComp->m_active = false;
        }
        else if(++timerComp.m_cycleCountB >= enemyConfComp.m_cycleNumberDyingInterval)
        {
            ++enemyConfComp.m_currentSprite;
            timerComp.m_cycleCountB = 0;
        }
    }
    spriteComp.m_spriteData = memSpriteComp.m_vectSpriteData[static_cast<uint32_t>(enemyConfComp.m_currentSprite)];
}

//===========================================================================
void VisionSystem::updateEnemyNormalSprite(EnemyConfComponent &enemyConfComp, TimerComponent &timerComp, uint32_t enemyEntity)
{
    if(enemyConfComp.m_behaviourMode == EnemyBehaviourMode_e::DYING)
    {
        enemyConfComp.m_displayMode = EnemyDisplayMode_e::DYING;
        if(enemyConfComp.m_currentDirRight)
        {
            mapEnemySprite_t::const_iterator it = enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::DYING_RIGHT);
            if(it != enemyConfComp.m_mapSpriteAssociate.end())
            {
                enemyConfComp.m_currentSprite = enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::DYING_RIGHT)->second.first;
            }
            else
            {
                enemyConfComp.m_currentSprite = enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::DYING)->second.first;
            }
        }
        else
        {
            enemyConfComp.m_currentSprite = enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::DYING)->second.first;
        }

        // enemyConfComp.m_currentSprite = enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::DYING)->second.first;
        timerComp.m_cycleCountA = 0;
        timerComp.m_cycleCountB = 0;
    }
    else
    {
        // FPS STUFF TO MODIFY
        MoveableComponent *enemyMoveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(enemyEntity);
        mapEnemySprite_t::const_iterator it = enemyConfComp.m_mapSpriteAssociate.find(getEnemySpriteType(enemyConfComp.m_attackPhase, enemyMoveComp->m_degreeOrientation));
        enemyConfComp.m_currentDirRight = it->first == EnemySpriteType_e::STATIC_RIGHT;
        //if sprite outside
        if(enemyConfComp.m_currentSprite < it->second.first ||
                enemyConfComp.m_currentSprite > it->second.second)
        {
            enemyConfComp.m_currentSprite = it->second.first;
            timerComp.m_cycleCountA = 0;
        }
        else if(++timerComp.m_cycleCountA > enemyConfComp.m_standardSpriteInterval)
        {
            if(enemyConfComp.m_currentSprite == it->second.second)
            {
                enemyConfComp.m_currentSprite = it->second.first;
            }
            else
            {
                ++enemyConfComp.m_currentSprite;
            }
            timerComp.m_cycleCountA = 0;
        }
    }
}

//===========================================================================
EnemySpriteType_e VisionSystem::getEnemySpriteType(EnemyAttackPhase_e phase, float degreeAngle)
{
    switch(phase)
    {
    case EnemyAttackPhase_e::MOVE_TO_TARGET_LEFT:
        return EnemySpriteType_e::STATIC_LEFT;
    case EnemyAttackPhase_e::MOVE_TO_TARGET_RIGHT:
        return EnemySpriteType_e::STATIC_RIGHT;
    case EnemyAttackPhase_e::SHOOT:
    {
        return (std::cos(getRadiantAngle(degreeAngle)) < 0) ? EnemySpriteType_e::ATTACK_LEFT : EnemySpriteType_e::ATTACK_RIGHT;
    }
    case EnemyAttackPhase_e::SHOOTED:
        return EnemySpriteType_e::TOUCHED;
    case EnemyAttackPhase_e::TOTAL:
        assert(false);
        break;
    }
}

//===========================================================================
void updateEnemyAttackSprite(EnemyConfComponent &enemyConfComp, TimerComponent &timerComp, float degreeAngle)
{
    EnemySpriteType_e spriteType;
    //if no attack right sprite
    if(enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::ATTACK_RIGHT) == enemyConfComp.m_mapSpriteAssociate.end())
    {
        spriteType = EnemySpriteType_e::ATTACK_LEFT;
    }
    else
    {
        spriteType = (std::cos(getRadiantAngle(degreeAngle)) < 0) ? EnemySpriteType_e::ATTACK_LEFT : EnemySpriteType_e::ATTACK_RIGHT;
    }
    //first element
    mapEnemySprite_t::const_iterator it = enemyConfComp.m_mapSpriteAssociate.find(spriteType);
    //if last animation
    if(enemyConfComp.m_currentSprite == it->second.second)
    {

        timerComp.m_cycleCountB = *timerComp.m_timeIntervalOptional;
        return;
    }
    if(enemyConfComp.m_currentSprite >= it->second.first &&
            enemyConfComp.m_currentSprite <= it->second.second)
    {        
        if(++timerComp.m_cycleCountC >= enemyConfComp.m_cycleNumberAttackInterval)
        {
            ++enemyConfComp.m_currentSprite;
            timerComp.m_cycleCountC = 0;
        }
    }
    //if sprite is not ATTACK Go to First atack sprite
    else
    {
        enemyConfComp.m_currentSprite = enemyConfComp.m_mapSpriteAssociate.find(spriteType)->second.first;
        timerComp.m_cycleCountC = 0;
    }
}

//===========================================================================
mapEnemySprite_t::const_reverse_iterator findMapLastElement(const mapEnemySprite_t &map,
                                                            EnemySpriteType_e key)
{
    for(mapEnemySprite_t::const_reverse_iterator rit = map.rbegin();
        rit != map.rend(); ++rit)
    {
        if(rit->first == key)
        {
            return rit;
        }
    }
    return map.rend();
}
