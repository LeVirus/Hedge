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
        if(genComp->m_tagA == CollisionTag_e::BULLET_ENEMY_CT ||
            genComp->m_tagA == CollisionTag_e::BULLET_PLAYER_CT)
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
        if(shotComp->m_spriteShotNum != memSpriteComp.m_vectSpriteData.size() - 1)
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
    if(Level::getDialogMode())
    {
        return;
    }
    PlayerConfComponent *playerConfComp = Ecsm_t::instance().getComponent<PlayerConfComponent, Components_e::PLAYER_CONF_COMPONENT>(playerEntity);
    MapPlayerSprite_t::const_iterator it = playerConfComp->m_mapSpriteAssociate.find(playerConfComp->m_spriteType);
    //if sprite outside
    if(playerConfComp->m_currentSprite < it->second.first ||
        playerConfComp->m_currentSprite > it->second.second)
    {
        playerConfComp->m_currentSprite = it->second.first;
        timerComp.m_cycleCountD = 0;
    }
    else if(++timerComp.m_cycleCountD > playerConfComp->m_standardSpriteInterval)
    {
        if(playerConfComp->m_currentSprite == it->second.second)
        {
            playerConfComp->m_currentSprite = it->second.first;
            if(playerConfComp->m_spriteType == PlayerSpriteElementType_e::JUMP_RIGHT || playerConfComp->m_spriteType == PlayerSpriteElementType_e::JUMP_LEFT)
            {
                ++playerConfComp->m_currentSprite;
            }
        }
        else
        {
            ++playerConfComp->m_currentSprite;
        }
        timerComp.m_cycleCountD = 0;
    }
    spriteComp.m_spriteData = memSpriteComp.m_vectSpriteData[static_cast<uint32_t>(playerConfComp->m_currentSprite)];
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
                assert(m_refMainEngine);
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
