#pragma once

#include <constants.hpp>
#include "ECS_Headers/System.hpp"

class ECSManager;
struct DoorComponent;
struct MapCoordComponent;
struct MoveableWallConfComponent;
class MainEngine;

class PlatformSystem : public ECS::System<Components_e::TOTAL_COMPONENTS>
{
public:
    PlatformSystem();
    void execSystem()override;
    void clearSystem();
    void memRefMainEngine(MainEngine *mainEngine);
    inline void memPlayerEntity(uint32_t entity)
    {
        m_playerEntity = entity;
    }
    //===================================================================
    inline void memPlayerCurrentWallOnGround(uint32_t entity)
    {
        m_currentWallPlayerOnGround = entity;
    }
    inline void unsetCurrentWallOnGround()
    {
        m_currentWallPlayerOnGround = {};
    }
private:
    void activeDoorSound(uint32_t entityNum);
    //===================================================================
    void treatMoveableWalls();
    void triggerMoveableWall(uint32_t wallEntity);
    void switchToNextPhaseMoveWall(uint32_t wallEntity, MapCoordComponent &mapComp,
                                   MoveableWallConfComponent &moveWallComp,
                                   const PairUI_t &previousPos, uint32_t entityNum);
private:
    std::optional<uint32_t> m_currentWallPlayerOnGround = std::nullopt;
    std::vector<uint32_t> m_vectMoveableWall, m_vectTrigger;
    ECSManager const *m_ECSManager;
    MainEngine *m_refMainEngine;
    uint32_t m_playerEntity = 10000;
};

Direction_e getReverseDirection(Direction_e dir);
void stopMoveWallLevelLimitCase(MapCoordComponent &mapComp, MoveableWallConfComponent &moveWallComp, uint32_t entityNum);
void setInitPhaseMoveWall(MapCoordComponent &mapComp, MoveableWallConfComponent &moveWallComp,
                          Direction_e currentDir, uint32_t wallEntity);
void reverseDirection(MoveableWallConfComponent &moveWallComp);
