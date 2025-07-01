#pragma once

#include "constants.hpp"
#include <ECS/Components/EnemyConfComponent.hpp>
#include <ECS/Components/AudioComponent.hpp>
#include <ECS_Headers/System.hpp>
#include <functional>

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
    inline void clear()
    {
        m_refVehicleAmmo.clear();
    }
    inline void memVehicleAmmoVet(std::vector<uint32_t> &vect)
    {
        m_refVehicleAmmo.push_back(vect);
    }
    void updateGeneratorEntities();
private:
    void confEnemiesGenerator(uint32_t generatorEntity, const PairFloat_t &point);
    void treatGenerator();
    void treatEject();
    void confNewVisibleShot(const std::vector<uint32_t> &visibleShots);
    void treatEnemyBehaviourAttack(uint32_t enemyEntity, MapCoordComponent &enemyMapComp,
                                   EnemyConfComponent &enemyConfComp, float distancePlayer);
    void treatStaticEnemy(EnemyConfComponent &enemyConfComp, MoveableComponent &moveComp, uint32_t enemyEntity, float distancePlayer);
    void updateEnemyDirection(EnemyConfComponent &enemyConfComp, MoveableComponent &moveComp, MapCoordComponent &enemyMapComp);
    void treatVisibleShots(const std::vector<uint32_t> &stdAmmo, bool grenade = false);
    void activeSound(uint32_t entityNum, uint32_t soundNum);
    void enemyShoot(EnemyConfComponent &enemyConfComp, MoveableComponent &moveComp, MapCoordComponent &enemyMapComp, float distancePlayer);
    void treatEnemyMove(MapCoordComponent *playerMapComp, MapCoordComponent &mapComp, float velocity, EnemyConfComponent &enemyConfComp, uint32_t enemyEntity);
private:
    uint32_t m_playerEntity, m_intervalEnemyBehaviour = 0.4 / FPS_VALUE, m_intervalVisibleShotLifeTime = 5.0 / FPS_VALUE,
    m_intervalEnemyPlayPassiveSound = 5.0 / FPS_VALUE;
    float m_distanceEnemyBehaviour = LEVEL_TILE_SIZE_PX * 9.0f;
    MainEngine *m_mainEngine;
    std::vector<SoundElement> m_memPlayerVisibleShot;
    std::vector<std::reference_wrapper<VectUI_t>> m_refVehicleAmmo;
    std::optional<std::set<uint32_t>> m_vectGeneratorEntities;
};

