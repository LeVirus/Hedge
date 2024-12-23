#pragma once

#include <ECS_Headers/System.hpp>
#include <constants.hpp>

class ECSManager;
struct VisionComponent;
struct MapCoordComponent;
struct MoveableComponent;
struct EnemyConfComponent;
struct MemSpriteDataComponent;
struct SpriteTextureComponent;
struct TimerComponent;
struct GeneralCollisionComponent;
struct TimerComponent;
struct TimerComponent;
class MainEngine;

using mapEnemySprite_t = std::map<EnemySpriteType_e, PairUI_t>;

class VisionSystem : public ECS::System<Components_e::TOTAL_COMPONENTS>
{
public:
    VisionSystem();
    void memECSManager(const ECSManager *memECSMan);
    void execSystem()override;
    inline void clearMemMultiSpritesWall()
    {
        m_memMultiSpritesWallEntities.clear();
    }
    inline void clearVectObjectToDelete()
    {
        m_vectBarrelsEntitiesToDelete.clear();
    }
    inline void memTeleportAnimEntity(uint32_t teleportAnimEntity)
    {
        m_memTeleportAnimEntity = teleportAnimEntity;
    }
    inline const std::vector<uint32_t> &getBarrelEntitiesToDelete()const
    {
        return m_vectBarrelsEntitiesToDelete;
    }
private:
    void setUsedComponents();
    void updateSprites();
    void updateEnemySprites(uint32_t enemyEntity, MemSpriteDataComponent &memSpriteComp, SpriteTextureComponent &spriteComp,
                            TimerComponent &timerComp, EnemyConfComponent &enemyConfComp);
    void updateEnemyNormalSprite(EnemyConfComponent &enemyConfComp, TimerComponent &timerComp, uint32_t enemyEntity);
    void updatePlayerSprites(uint32_t playerEntity, MemSpriteDataComponent &memSpriteComp, SpriteTextureComponent &spriteComp, TimerComponent &timerComp);
    void updateVisibleShotSprite(uint32_t shotEntity, MemSpriteDataComponent &memSpriteComp, SpriteTextureComponent &spriteComp, TimerComponent &timerComp, GeneralCollisionComponent &genComp);
private:
    std::vector<uint32_t> m_memMultiSpritesWallEntities, m_vectBarrelsEntitiesToDelete;
    uint32_t m_defaultInterval = 0.8 / FPS_VALUE, m_memTeleportAnimEntity;
    //first change sprite interval, second interval total time
    PairUI_t m_teleportIntervalTime = {0.1 / FPS_VALUE, 0.4 / FPS_VALUE};
    MainEngine *m_refMainEngine;
};

mapEnemySprite_t::const_reverse_iterator findMapLastElement(const mapEnemySprite_t &map,
                                                            EnemySpriteType_e key);
void updateTriangleVisionFromPosition(VisionComponent &visionComp, MapCoordComponent &mapComp,
                                      MoveableComponent &movComp);
void updateEnemyAttackSprite(EnemyConfComponent &enemyConfComp, TimerComponent &timerComp);
