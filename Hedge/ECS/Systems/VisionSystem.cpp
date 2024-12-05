#include <cassert>
#include "VisionSystem.hpp"
#include <constants.hpp>
#include <MainEngine.hpp>
#include <CollisionUtils.hpp>
#include <PhysicalEngine.hpp>
#include <math.h>
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/MapCoordComponent.hpp>
#include <ECS/Components/GeneralCollisionComponent.hpp>
#include <ECS/Components/CircleCollisionComponent.hpp>
#include <ECS/Components/RectangleCollisionComponent.hpp>
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/MoveableComponent.hpp>
#include <ECS/Components/MemSpriteDataComponent.hpp>
#include <ECS/Components/MemFPSGLSizeComponent.hpp>
#include <ECS/Components/SpriteTextureComponent.hpp>
#include <ECS/Components/EnemyConfComponent.hpp>
#include <ECS/Components/BarrelComponent.hpp>
#include <ECS/Components/TimerComponent.hpp>
#include <ECS/Components/ShotConfComponent.hpp>
#include <ECS/Components/ImpactShotComponent.hpp>

//===========================================================================
VisionSystem::VisionSystem()
{
    setUsedComponents();
}

//===========================================================================
void VisionSystem::setUsedComponents()
{
}

//===========================================================================
void VisionSystem::execSystem()
{
    std::bitset<Components_e::TOTAL_COMPONENTS> bitsetComp;
    bitsetComp[Components_e::MAP_COORD_COMPONENT] = true;
    bitsetComp[Components_e::SPRITE_TEXTURE_COMPONENT] = true;
    bitsetComp[Components_e::GENERAL_COLLISION_COMPONENT] = true;
    std::vector<uint32_t> vectEntities = m_memECSManager->getEntitiesContainingComponents(bitsetComp);
    for(uint32_t i = 0; i < mVectNumEntity.size(); ++i)
    {
        MapCoordComponent *mapCompA = Ecsm_t::instance().getComponent<MapCoordComponent, Components_e::MAP_COORD_COMPONENT>(mVectNumEntity[i]);
        MoveableComponent *moveCompA = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(mVectNumEntity[i]);
        for(uint32_t j = 0; j < vectEntities.size(); ++j)
        {
            if(mVectNumEntity[i] == vectEntities[j])
            {
                continue;
            }
        }
        updateSprites(mVectNumEntity[i], vectEntities);
    }
}

//===========================================================================
void VisionSystem::updateSprites(uint32_t observerEntity,
                                 const std::vector<uint32_t> &vectEntities)
{
    OptUint_t compNum;
    for(uint32_t i = 0; i < vectEntities.size(); ++i)
    {
        MemSpriteDataComponent *memSpriteComp = Ecsm_t::instance().getComponent<MemSpriteDataComponent, Components_e::MEM_SPRITE_DATA_COMPONENT>(vectEntities[i]);
        if(!memSpriteComp)
        {
            continue;
        }
        GeneralCollisionComponent *genComp = Ecsm_t::instance().getComponent<GeneralCollisionComponent, Components_e::GENERAL_COLLISION_COMPONENT>(vectEntities[i]);
        if(!genComp->m_active)
        {
            continue;
        }
        SpriteTextureComponent *spriteComp = Ecsm_t::instance().getComponent<SpriteTextureComponent, Components_e::SPRITE_TEXTURE_COMPONENT>(vectEntities[i]);
        TimerComponent *timerComp = Ecsm_t::instance().getComponent<TimerComponent, Components_e::TIMER_COMPONENT>(vectEntities[i]);
        //OOOOK put enemy tag to tagB
        if(genComp->m_tagA == CollisionTag_e::ENEMY_CT || genComp->m_tagA == CollisionTag_e::GHOST_CT)
        {
            EnemyConfComponent *enemyConfComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(vectEntities[i]);
            if(enemyConfComp)
            {
                updateEnemySprites(vectEntities[i], observerEntity, *memSpriteComp, *spriteComp, *timerComp, *enemyConfComp);
            }
        }
    }
}

//===========================================================================
void VisionSystem::updateEnemySprites(uint32_t enemyEntity, uint32_t observerEntity,
                                      MemSpriteDataComponent &memSpriteComp,
                                      SpriteTextureComponent &spriteComp,
                                      TimerComponent &timerComp, EnemyConfComponent &enemyConfComp)
{
    if(enemyConfComp.m_touched)
    {
        if(enemyConfComp.m_frozenOnAttack)
        {
            enemyConfComp.m_currentSprite =
                    enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::TOUCHED)->second.first;
        }
        if(++timerComp.m_cycleCountC >= enemyConfComp.m_cycleNumberSpriteUpdate)
        {
            enemyConfComp.m_touched = false;
        }
    }
    else if(enemyConfComp.m_behaviourMode == EnemyBehaviourMode_e::ATTACK &&
            enemyConfComp.m_attackPhase == EnemyAttackPhase_e::SHOOT)
    {
        updateEnemyAttackSprite(enemyConfComp, timerComp);
    }
    else if(enemyConfComp.m_displayMode == EnemyDisplayMode_e::NORMAL)
    {
        updateEnemyNormalSprite(enemyConfComp, timerComp, enemyEntity, observerEntity);
    }
    else if(enemyConfComp.m_displayMode == EnemyDisplayMode_e::DYING)
    {
        mapEnemySprite_t::const_iterator it = enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::DYING);
        if(enemyConfComp.m_currentSprite == it->second.second)
        {
            enemyConfComp.m_displayMode = EnemyDisplayMode_e::DEAD;
            if(enemyConfComp.m_endLevel)
            {
                m_refMainEngine->activeEndLevel();
            }
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
void VisionSystem::updateEnemyNormalSprite(EnemyConfComponent &enemyConfComp, TimerComponent &timerComp,
                                           uint32_t enemyEntity, uint32_t observerEntity)
{
    if(enemyConfComp.m_behaviourMode == EnemyBehaviourMode_e::DYING)
    {
        enemyConfComp.m_displayMode = EnemyDisplayMode_e::DYING;
        enemyConfComp.m_currentSprite = enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::DYING)->second.first;
        timerComp.m_cycleCountA = 0;
        timerComp.m_cycleCountB = 0;
    }
    else
    {
        MoveableComponent *enemyMoveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(enemyEntity);
        EnemySpriteType_e currentOrientationSprite =
                getOrientationFromAngle(observerEntity, enemyEntity,
                                        enemyMoveComp->m_degreeOrientation);
        mapEnemySprite_t::const_iterator it = enemyConfComp.m_mapSpriteAssociate.find(currentOrientationSprite);
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
void updateEnemyAttackSprite(EnemyConfComponent &enemyConfComp, TimerComponent &timerComp)
{
    //first element
    mapEnemySprite_t::const_iterator it = enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::ATTACK);
    //if last animation
    if(enemyConfComp.m_currentSprite == it->second.second)
    {
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
        enemyConfComp.m_currentSprite = enemyConfComp.m_mapSpriteAssociate.find(EnemySpriteType_e::ATTACK)->second.first;
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
