#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H

#include <includesLib/BaseECS/system.hpp>
#include "constants.hpp"
#include <map>

struct CircleCollisionComponent;
struct RectangleCollisionComponent;
struct SegmentCollisionComponent;
struct MapCoordComponent;
struct CollisionComponent;
struct CollisionArgs;
struct EjectArgs;

class CollisionSystem : public ecs::System
{
private:
    std::multimap<CollisionTag_e, CollisionTag_e> m_tagArray;
    float m_memPosX, m_memPosY;
    bool m_memPosActive, m_memVelocity;
    MapCoordComponent *m_memMapComp;
private:
    void setUsedComponents();
    void initArrayTag();
    void postProcessBehavior();
    bool checkTag(CollisionTag_e entityTagA, CollisionTag_e entityTagB);
    void checkCollision(uint32_t entityNumA, uint32_t entityNumB,
                        CollisionComponent *tagCompA,
                        CollisionComponent *tagCompB);
    //Collisions detection
    void checkCollisionFirstRect(CollisionArgs &args);
    void checkCollisionFirstCircle(CollisionArgs &args);
    void checkCollisionFirstLine(CollisionArgs &args);

    //Collisions treatment
    void treatCollisionCircleRect(CollisionArgs &args,
                                const CircleCollisionComponent &circleCollA,
                                const RectangleCollisionComponent &rectCollB);
    float getVerticalEject(const EjectArgs& args);
    void treatCollisionCircleCircle(CollisionArgs &args,
                                const CircleCollisionComponent &circleCollA,
                                const CircleCollisionComponent &circleCollB);
    void treatCollisionCircleSegment(CollisionArgs &args,
                                const CircleCollisionComponent &circleCollA,
                                const SegmentCollisionComponent &segmCollB);
    void collisionEjectCircleRect(MapCoordComponent &mapComp,
                                     float diffX, float diffY, float velocity, bool angleBehavior);
    //Components accessors
    CircleCollisionComponent &getCircleComponent(uint32_t entityNum);
    RectangleCollisionComponent &getRectangleComponent(uint32_t entityNum);
    SegmentCollisionComponent &getSegmentComponent(uint32_t entityNum);
    MapCoordComponent &getMapComponent(uint32_t entityNum);
public:
    CollisionSystem();
    void execSystem()override;
};

struct CollisionArgs
{
    uint32_t entityNumA, entityNumB;
    const CollisionComponent *tagCompA, *tagCompB;
    MapCoordComponent &mapCompA, &mapCompB;
};

struct EjectArgs
{
    float circlePosX, pointElementX, circlePosY, elementPosY,
    elementSecondPosY, ray, radDegree;
    bool angleMode;
};

#endif // COLLISIONSYSTEM_H
