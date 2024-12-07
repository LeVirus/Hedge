#include <cassert>
#include "VisionSystem.hpp"
#include <constants.hpp>
#include <MainEngine.hpp>
#include <CollisionUtils.hpp>
#include <PhysicalEngine.hpp>
#include <math.h>
#include <alias.hpp>
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/MapCoordComponent.hpp>
#include <ECS/Components/GeneralCollisionComponent.hpp>
#include <ECS/Components/CircleCollisionComponent.hpp>
#include <ECS/Components/RectangleCollisionComponent.hpp>
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/MoveableComponent.hpp>
#include <ECS/Components/MemSpriteDataComponent.hpp>
#include <ECS/Components/SpriteTextureComponent.hpp>
#include <ECS/Components/EnemyConfComponent.hpp>
#include <ECS/Components/TimerComponent.hpp>
#include <ECS/Components/ShotConfComponent.hpp>

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
    updateSprites();
}

//===========================================================================
void VisionSystem::updateSprites()
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
        //OOOOK put enemy tag to tagB
        if(genComp->m_tagA == CollisionTag_e::ENEMY_CT || genComp->m_tagA == CollisionTag_e::GHOST_CT)
        {
            EnemyConfComponent *enemyConfComp = Ecsm_t::instance().getComponent<EnemyConfComponent, Components_e::ENEMY_CONF_COMPONENT>(*it);
            if(enemyConfComp)
            {
                updateEnemySprites(*it, *memSpriteComp, *spriteComp, *timerComp, *enemyConfComp);
            }
        }
    }
}

//===========================================================================
void VisionSystem::updateEnemySprites(uint32_t enemyEntity,
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
        updateEnemyNormalSprite(enemyConfComp, timerComp, enemyEntity);
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
                                           uint32_t enemyEntity)
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
        //FPS STUFF TO MODIFY
        // MoveableComponent *enemyMoveComp = Ecsm_t::instance().getComponent<MoveableComponent, Components_e::MOVEABLE_COMPONENT>(enemyEntity);
        // mapEnemySprite_t::const_iterator it = enemyConfComp.m_mapSpriteAssociate.find(currentOrientationSprite);
        // //if sprite outside
        // if(enemyConfComp.m_currentSprite < it->second.first ||
        //         enemyConfComp.m_currentSprite > it->second.second)
        // {
        //     enemyConfComp.m_currentSprite = it->second.first;
        //     timerComp.m_cycleCountA = 0;
        // }
        // else if(++timerComp.m_cycleCountA > enemyConfComp.m_standardSpriteInterval)
        // {
        //     if(enemyConfComp.m_currentSprite == it->second.second)
        //     {
        //         enemyConfComp.m_currentSprite = it->second.first;
        //     }
        //     else
        //     {
        //         ++enemyConfComp.m_currentSprite;
        //     }
        //     timerComp.m_cycleCountA = 0;
        // }
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
