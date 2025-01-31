#pragma once

#include <ECS_Headers/System.hpp>
#include "constants.hpp"
#include "ZoneLevelColl.hpp"
#include <map>

struct CircleCollisionComponent;
struct RectangleCollisionComponent;
struct SegmentCollisionComponent;
struct MapCoordComponent;
struct GeneralCollisionComponent;
struct CollisionArgs;
struct EjectYArgs;
struct EjectXArgs;
struct PlayerConfComponent;
struct WeaponComponent;
struct ShotConfComponent;
class MainEngine;

using OptUint_t = std::optional<uint32_t>;

//TMP
class CollisionSystem : public ECS::System<TOTAL_COMPONENTS>
{
public:
    CollisionSystem();
    void execSystem()override;
    inline const std::vector<uint32_t> &getStaticEntitiesToDelete()const
    {
        return m_vectEntitiesToDelete;
    }
    inline const std::vector<uint32_t> &getBarrelEntitiesDestruct()const
    {
        return m_vectBarrelsEntitiesDestruct;
    }
    inline void clearVectBarrelsDestruct()
    {
        m_vectBarrelsEntitiesDestruct.clear();
    }
    inline void clearVectObjectToDelete()
    {
        m_vectEntitiesToDelete.clear();
    }
    inline void linkMainEngine(MainEngine *mainEngine)
    {
        m_refMainEngine = mainEngine;
    }
    inline void memPlayerDatas(uint32_t playerEntity)
    {
        m_playerEntity = playerEntity;
    }
    inline void addEntityToZone(uint32_t entity, const PairUI_t &coord)
    {
        m_zoneLevel->updateEntityToZones(entity, coord);
    }
    inline void removeEntityToZone(uint32_t entity)
    {
        m_zoneLevel->removeEntityToZones(entity);
    }
    void updateZonesColl();
    void writePlayerInfo(const std::string &info);
private:
    void checkCollisionFirstRect(CollisionArgs &args);
    void treatSegmentShots();
    void rmEnemyCollisionMaskEntity(uint32_t numEntity);
    void setUsedComponents();
    void initArrayTag();
    bool checkTag(CollisionTag_e entityTagA, CollisionTag_e entityTagB);
    //return false if new collision iteration have to be done
    bool treatCollision(uint32_t entityNumA, uint32_t entityNumB, GeneralCollisionComponent &tagCompA,
                        GeneralCollisionComponent &tagCompB, bool shotExplosionEject = false);
    //Collisions detection
    void treatCollisionFirstRect(CollisionArgs &args);
    bool treatCollisionFirstCircle(CollisionArgs &args, bool shotExplosionEject = false);
    bool treatCollisionPlayer(CollisionArgs &args, CircleCollisionComponent &circleCompA, RectangleCollisionComponent &rectCompB);
    void setDamageCircle(uint32_t shotEntity, bool active, uint32_t baseEntity = 0);
    void treatActionPlayerCircle(CollisionArgs &args);
    void treatPlayerPickObject(CollisionArgs &args);
    void treatCollisionFirstSegment(CollisionArgs &args);
    void treatCrushing(uint32_t entityNum);
    //Collisions treatment
    void collisionCircleRectEject(CollisionArgs &args,
                                  float circleRay, const RectangleCollisionComponent &rectCollB, bool visibleShotFirstEject = false);
    void collisionRectRectEject(CollisionArgs &args);
    float getVerticalCircleRectEject(const EjectYArgs &args, bool &limitEject, bool visibleShot);
    float getHorizontalCircleRectEject(const EjectXArgs &args, bool &limitEject, bool visibleShot);
    void collisionCircleCircleEject(CollisionArgs &args,
                                    const CircleCollisionComponent &circleCollA,
                                    const CircleCollisionComponent &circleCollB);
//    void treatCollisionCircleSegment(CollisionArgs &args,
//                                     const CircleCollisionComponent &circleCollA,
//                                     const SegmentCollisionComponent &segmCollB);
    void collisionEject(MapCoordComponent &mapComp, float diffX, float diffY,
                        bool limitEjectY = false, bool limitEjectX = false, bool crushCase = false);
    //Components accessors
    CircleCollisionComponent &getCircleComponent(uint32_t entityNum);
    RectangleCollisionComponent &getRectangleComponent(uint32_t entityNum);
    SegmentCollisionComponent &getSegmentComponent(uint32_t entityNum);
    MapCoordComponent &getMapComponent(uint32_t entityNum);
    void checkCollisionFirstSegment(uint32_t numEntityA, uint32_t numEntityB,
                                    GeneralCollisionComponent &tagCompB,
                                    MapCoordComponent &mapCompB);
    void treatEnemyTakeDamage(uint32_t enemyEntityNum, uint32_t damage = 1);
    void confDropedObject(uint32_t objectEntity, uint32_t enemyEntity);
    void activeSound(uint32_t entityNum);
    bool checkEnemyRemoveCollisionMask(uint32_t entityNum);
    void treatGeneralCrushing(uint32_t entityNum);
    void secondEntitiesLoop(uint32_t entityA, uint32_t currentIteration, GeneralCollisionComponent &tagCompA, bool shotExplosionEject = false);
    bool iterationLoop(uint32_t currentIteration, uint32_t entityA, uint32_t entityB, GeneralCollisionComponent &tagCompA, bool shotExplosionEject);
private:
    std::unique_ptr<ZoneLevelColl> m_zoneLevel;
    uint32_t m_playerEntity;
    std::multimap<CollisionTag_e, CollisionTag_e> m_tagArray;
    std::pair<std::optional<uint32_t>, float> m_memDistCurrentBulletColl;
    //first bullet second target
    std::vector<PairUI_t> m_vectMemShots;
    std::vector<uint32_t> m_vectEntitiesToDelete, m_vectBarrelsEntitiesDestruct;
    //0 movement eject, 1 angle behaviour, 2 Direction,
    //3 if moveable wall current direction
    std::vector<std::tuple<PairFloat_t, bool, Direction_e, std::optional<Direction_e>>> m_memCrush;
    MainEngine *m_refMainEngine;
};

bool opposingDirection(Direction_e dirA, Direction_e dirB);
bool pickUpWeapon(uint32_t numWeapon, WeaponComponent &weaponComp,
                  uint32_t objectContaining);
bool pickUpAmmo(uint32_t numWeapon, WeaponComponent &weaponComp, uint32_t objectContaining);
Direction_e getDirection(float diffX, float diffY);

struct CollisionArgs
{
    uint32_t entityNumA, entityNumB;
    GeneralCollisionComponent &tagCompA, &tagCompB;
    MapCoordComponent &mapCompA, &mapCompB;
};

struct EjectXArgs
{
    float circlePosX, circlePosY, elementPosY, elementPosX,
    elementSecondPosX, ray, radiantAngle;
    bool angleMode;
};

struct EjectYArgs
{
    float circlePosX, circlePosY, elementPosX, elementPosY,
    elementSecondPosY, ray, radiantAngle;
    bool angleMode;
};
