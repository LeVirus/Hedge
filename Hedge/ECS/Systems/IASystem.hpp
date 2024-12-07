#pragma once

#include "constants.hpp"
#include <ECS/Components/EnemyConfComponent.hpp>
#include <ECS/Components/AudioComponent.hpp>
#include <ECS_Headers/System.hpp>

struct MapCoordComponent;
struct EnemyConfComponent;
struct MoveableComponent;
struct PlayerConfComponent;
class MainEngine;
class ECSManager;

class IASystem : public ECS::System<Components_e::TOTAL_COMPONENTS>
{
public:
    IASystem();
    void execSystem()override;
    void memPlayerDatas(uint32_t playerEntity);
    void confVisibleShoot(std::vector<uint32_t> &visibleShots, const PairFloat_t &point, float degreeAngle, CollisionTag_e tag);
    inline void linkMainEngine(MainEngine *mainEngine)
    {
        m_mainEngine = mainEngine;
    }
private:
    void treatEject();
    void confNewVisibleShot(const std::vector<uint32_t> &visibleShots);
    void treatEnemyBehaviourAttack(uint32_t enemyEntity, MapCoordComponent &enemyMapComp,
                                   EnemyConfComponent &enemyConfComp, float distancePlayer);
    void updateEnemyDirection(EnemyConfComponent &enemyConfComp, MoveableComponent &moveComp, MapCoordComponent &enemyMapComp);
    void treatVisibleShots(const std::vector<uint32_t> &stdAmmo);
    void treatVisibleShot(uint32_t numEntity);
    void activeSound(uint32_t entityNum, uint32_t soundNum);
private:
    uint32_t m_playerEntity, m_intervalEnemyBehaviour = 0.4 / FPS_VALUE, m_intervalVisibleShotLifeTime = 5.0 / FPS_VALUE,
    m_intervalEnemyPlayPassiveSound = 5.0 / FPS_VALUE;
    float m_distanceEnemyBehaviour = LEVEL_TILE_SIZE_PX * 9.0f;
    MainEngine *m_mainEngine;
    std::vector<SoundElement> m_memPlayerVisibleShot;
};

