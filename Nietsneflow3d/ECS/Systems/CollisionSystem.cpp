#include "CollisionSystem.hpp"
#include "constants.hpp"
#include <ECS/Components/MapCoordComponent.hpp>
#include <ECS/Components/GeneralCollisionComponent.hpp>
#include <ECS/Components/CircleCollisionComponent.hpp>
#include <ECS/Components/RectangleCollisionComponent.hpp>
#include <ECS/Components/LineCollisionComponent.hpp>
#include <ECS/Components/MoveableComponent.hpp>
#include <CollisionUtils.hpp>
#include <PhysicalEngine.hpp>
#include <cassert>
#include <math.h>

using multiMapTagIt = std::multimap<CollisionTag_e, CollisionTag_e>::const_iterator;

//===================================================================
CollisionSystem::CollisionSystem()
{
    setUsedComponents();
    initArrayTag();
}

//===================================================================
void CollisionSystem::setUsedComponents()
{
    bAddComponentToSystem(Components_e::GENERAL_COLLISION_COMPONENT);
    bAddComponentToSystem(Components_e::MAP_COORD_COMPONENT);
}

//===================================================================
void CollisionSystem::execSystem()
{
    System::execSystem();
    for(uint32_t i = 0; i < mVectNumEntity.size(); ++i)
    {
        GeneralCollisionComponent *tagCompA = stairwayToComponentManager().
                searchComponentByType<GeneralCollisionComponent>(mVectNumEntity[i],
                                                         Components_e::GENERAL_COLLISION_COMPONENT);
        if(tagCompA->m_tag == CollisionTag_e::WALL_C ||
                tagCompA->m_tag == CollisionTag_e::OBJECT_C)
        {
            continue;
        }
        assert(tagCompA);
        for(uint32_t j = 0; j < mVectNumEntity.size(); ++j)
        {
            if(i == j)
            {
                continue;
            }
            GeneralCollisionComponent *tagCompB = stairwayToComponentManager().
                    searchComponentByType<GeneralCollisionComponent>(mVectNumEntity[j],
                                                        Components_e::GENERAL_COLLISION_COMPONENT);
            assert(tagCompB);
            if(checkTag(tagCompA->m_tag, tagCompB->m_tag))
            {
                checkCollision(mVectNumEntity[i], mVectNumEntity[j],
                               tagCompA, tagCompB);
            }
        }
    }
}

//===================================================================
void CollisionSystem::initArrayTag()
{
    m_tagArray.insert({PLAYER, WALL_C});
    m_tagArray.insert({PLAYER, ENEMY});
    m_tagArray.insert({PLAYER, BULLET_ENEMY});
    m_tagArray.insert({PLAYER, OBJECT_C});

    m_tagArray.insert({ENEMY, BULLET_PLAYER});
    m_tagArray.insert({ENEMY, PLAYER});
    m_tagArray.insert({ENEMY, WALL_C});

    m_tagArray.insert({WALL_C, PLAYER});
    m_tagArray.insert({WALL_C, ENEMY});
    m_tagArray.insert({WALL_C, BULLET_ENEMY});
    m_tagArray.insert({WALL_C, BULLET_PLAYER});

    m_tagArray.insert({BULLET_ENEMY, PLAYER});
    m_tagArray.insert({BULLET_ENEMY, WALL_C});

    m_tagArray.insert({BULLET_PLAYER, ENEMY});
    m_tagArray.insert({BULLET_PLAYER, WALL_C});

    m_tagArray.insert({OBJECT_C, PLAYER});
}

//===================================================================
bool CollisionSystem::checkTag(CollisionTag_e entityTagA,
                               CollisionTag_e entityTagB)
{
    for(multiMapTagIt it = m_tagArray.find(entityTagA);
        it != m_tagArray.end() || it->first != entityTagA; ++it)
    {
        if(it->second == entityTagB)
        {
            return true;
        }
    }
    return false;
}

//===================================================================
void CollisionSystem::checkCollision(uint32_t entityNumA, uint32_t entityNumB,
                                     GeneralCollisionComponent *tagCompA,
                                     GeneralCollisionComponent *tagCompB)
{
    MapCoordComponent &mapCompA = getMapComponent(entityNumA),
            &mapCompB = getMapComponent(entityNumB);
    CollisionArgs args = {entityNumA, entityNumB,
                          tagCompA, tagCompB,
                          mapCompA, mapCompB};
    if(tagCompA->m_shape == CollisionShape_e::RECTANGLE_C)
    {
        checkCollisionFirstRect(args);
    }
    else if(tagCompA->m_shape == CollisionShape_e::CIRCLE)
    {
        checkCollisionFirstCircle(args);
    }
    else if(tagCompA->m_shape == CollisionShape_e::SEGMENT)
    {
        checkCollisionFirstSegment(args);
    }
}

//===================================================================
void CollisionSystem::checkCollisionFirstRect(CollisionArgs &args)
{
    RectangleCollisionComponent &rectCompA = getRectangleComponent(args.entityNumA);
    bool collision = false;
    switch(args.tagCompB->m_shape)
    {
    case CollisionShape_e::RECTANGLE_C:
    {
        RectangleCollisionComponent &rectCompB = getRectangleComponent(args.entityNumB);
        collision = checkRectRectCollision(args.mapCompA.m_absoluteMapPositionPX, rectCompA.m_size,
                               args.mapCompB.m_absoluteMapPositionPX, rectCompB.m_size);
    }
        break;
    case CollisionShape_e::CIRCLE:
    {
        CircleCollisionComponent &circleCompB = getCircleComponent(args.entityNumB);
        collision = checkCircleRectCollision(args.mapCompB.m_absoluteMapPositionPX, circleCompB.m_ray,
                                 args.mapCompA.m_absoluteMapPositionPX, rectCompA.m_size);
    }
        break;
    case CollisionShape_e::SEGMENT:
    {
        SegmentCollisionComponent &segmentCompB = getSegmentComponent(args.entityNumB);
        collision = checkSegmentRectCollision(args.mapCompB.m_absoluteMapPositionPX, segmentCompB.m_secondPoint,
                               args.mapCompA.m_absoluteMapPositionPX, rectCompA.m_size);
    }
        break;
    }
}

//===================================================================
void CollisionSystem::checkCollisionFirstCircle(CollisionArgs &args)
{
    CircleCollisionComponent &circleCompA = getCircleComponent(args.entityNumA);
    bool collision = false;
    switch(args.tagCompB->m_shape)
    {
    case CollisionShape_e::RECTANGLE_C:
    {
        RectangleCollisionComponent &rectCompB = getRectangleComponent(args.entityNumB);
        collision = checkCircleRectCollision(args.mapCompA.m_absoluteMapPositionPX, circleCompA.m_ray,
                                 args.mapCompB.m_absoluteMapPositionPX, rectCompB.m_size);
        if(collision)
        {
            treatCollisionCircleRect(args, circleCompA, rectCompB);
        }
    }
        break;
    case CollisionShape_e::CIRCLE:
    {
        CircleCollisionComponent &circleCompB = getCircleComponent(args.entityNumB);
        collision = checkCircleCircleCollision(args.mapCompA.m_absoluteMapPositionPX, circleCompA.m_ray,
                                   args.mapCompB.m_absoluteMapPositionPX, circleCompB.m_ray);
        if(collision)
        {
            treatCollisionCircleCircle(args, circleCompA, circleCompB);
        }
    }
        break;
    case CollisionShape_e::SEGMENT:
    {
        SegmentCollisionComponent &segmentCompB = getSegmentComponent(args.entityNumB);
        collision = checkCircleSegmentCollision(args.mapCompA.m_absoluteMapPositionPX, circleCompA.m_ray,
                                 args.mapCompB.m_absoluteMapPositionPX, segmentCompB.m_secondPoint);
        if(collision)
        {
            treatCollisionCircleSegment(args, circleCompA, segmentCompB);
        }
    }
        break;
    }
}

//===================================================================
void CollisionSystem::checkCollisionFirstSegment(CollisionArgs &args)
{
    bool collision = false;
    SegmentCollisionComponent &lineCompA = getSegmentComponent(args.entityNumA);
    switch(args.tagCompB->m_shape)
    {
    case CollisionShape_e::RECTANGLE_C:
    {
        RectangleCollisionComponent &rectCompB = getRectangleComponent(args.entityNumB);
        collision = checkSegmentRectCollision(args.mapCompA.m_absoluteMapPositionPX, lineCompA.m_secondPoint,
                               args.mapCompB.m_absoluteMapPositionPX, rectCompB.m_size);
    }
        break;
    case CollisionShape_e::CIRCLE:
    {
        CircleCollisionComponent &circleCompB = getCircleComponent(args.entityNumB);
        collision = checkCircleSegmentCollision(args.mapCompB.m_absoluteMapPositionPX, circleCompB.m_ray,
                                 args.mapCompA.m_absoluteMapPositionPX, lineCompA.m_secondPoint);
    }
        break;
    case CollisionShape_e::SEGMENT:
    {}
        break;
    }
}

//===================================================================
void CollisionSystem::treatCollisionCircleRect(CollisionArgs &args,
                                               const CircleCollisionComponent &circleCollA,
                                               const RectangleCollisionComponent &rectCollB)
{
    if(args.tagCompA->m_tag == CollisionTag_e::PLAYER ||
            args.tagCompA->m_tag == CollisionTag_e::ENEMY)
    {
        MapCoordComponent &mapComp = getMapComponent(args.entityNumA);
        MoveableComponent *moveComp = stairwayToComponentManager().
                searchComponentByType<MoveableComponent>(args.entityNumA,
                                      Components_e::MOVEABLE_COMPONENT);
        assert(moveComp);
        float radDegree = getRadiantAngle(moveComp->m_currentDegreeDirection);
        float circlePosX = args.mapCompA.m_absoluteMapPositionPX.first;
        float circlePosY = args.mapCompA.m_absoluteMapPositionPX.second;
        float elementPosX = args.mapCompB.m_absoluteMapPositionPX.first;
        float elementPosY = args.mapCompB.m_absoluteMapPositionPX.second;
        float elementSecondPosX = elementPosX + rectCollB.m_size.first;
        float elementSecondPosY = elementPosY + rectCollB.m_size.second;

        bool angleBehavior = false;
        //collision on angle of rect
        if((circlePosX < elementPosX || circlePosX > elementSecondPosX) &&
                (circlePosY < elementPosY || circlePosY > elementSecondPosY))
        {
            angleBehavior = true;
        }

        float pointElementX = circlePosX < elementPosX ?
                    elementPosX : elementSecondPosX;
        float pointElementY = circlePosY < elementPosY ?
                    elementPosY : elementSecondPosY;

        float diffY = getVerticalCircleRectEject({circlePosX, circlePosY, pointElementX,
                                  elementPosY,
                                  elementSecondPosY,
                                  circleCollA.m_ray, radDegree,
                                  angleBehavior});
        float diffX = getHorizontalCircleRectEject({circlePosX, circlePosY, pointElementY,
                                  elementPosX,
                                  elementSecondPosX,
                                  circleCollA.m_ray, radDegree,
                                  angleBehavior});
        collisionEjectCircleRect(mapComp, diffX, diffY);
    }
}

//===================================================================
float CollisionSystem::getVerticalCircleRectEject(const EjectYArgs& args)
{
    float adj, diffY;
    if(args.angleMode)
    {
        adj = std::abs(args.circlePosX - args.elementPosX);
        diffY = std::sqrt(args.ray * args.ray - adj * adj);
    }
    //DOWN
    if(std::sin(args.radDegree) < 0.0f)
    {
        if(args.angleMode)
        {
            diffY -= (args.elementPosY - args.circlePosY);
            diffY = -diffY;
        }
        else
        {
            diffY = -args.ray + (args.elementPosY - args.circlePosY);
        }
    }
    //UP
    else
    {
        if(args.angleMode)
        {
            diffY -= (args.circlePosY - args.elementSecondPosY);
        }
        else
        {
            diffY = args.ray - (args.circlePosY - args.elementSecondPosY);
        }
    }
    return diffY;
}

//===================================================================
float CollisionSystem::getHorizontalCircleRectEject(const EjectXArgs &args)
{
    float adj, diffX;
    if(args.angleMode)
    {
        adj = std::abs(args.circlePosY - args.elementPosY);
        diffX = std::sqrt(args.ray * args.ray - adj * adj);
    }
    //RIGHT
    if(std::cos(args.radDegree) > 0.0f)
    {
        if(args.angleMode)
        {
            diffX -= (args.elementPosX - args.circlePosX);
            diffX = -diffX;
        }
        else
        {
            diffX = -args.ray + (args.elementPosX - args.circlePosX);
        }
    }
    //LEFT
    else
    {
        if(args.angleMode)
        {
            diffX -= (args.circlePosX - args.elementSecondPosX);
        }
        else
        {
            diffX = args.ray - (args.circlePosX - args.elementSecondPosX);
        }
    }
    return diffX;
}

//===================================================================
void CollisionSystem::collisionEjectCircleCircle(MapCoordComponent &mapComp,
                                                 float diffX, float diffY)
{
    bool treatX = std::abs(diffX) > std::abs(diffY);
    if(treatX)
    {
        mapComp.m_absoluteMapPositionPX.first += diffX;
    }
    else
    {
        mapComp.m_absoluteMapPositionPX.second += diffY;
    }
}

//===================================================================
void CollisionSystem::collisionEjectCircleRect(MapCoordComponent &mapComp,
                                               float diffX, float diffY)
{
    bool treatX = std::abs(diffX) < std::abs(diffY);
    if(treatX)
    {
        mapComp.m_absoluteMapPositionPX.first += diffX;
    }
    else
    {
        mapComp.m_absoluteMapPositionPX.second += diffY;
    }
}

//===================================================================
void CollisionSystem::treatCollisionCircleCircle(CollisionArgs &args,
                                                 const CircleCollisionComponent &circleCollA,
                                                 const CircleCollisionComponent &circleCollB)
{
    if(args.tagCompA->m_tag == CollisionTag_e::PLAYER)
    {
        MoveableComponent *moveCompA = stairwayToComponentManager().
                searchComponentByType<MoveableComponent>(args.entityNumA,
                                      Components_e::MOVEABLE_COMPONENT);
        assert(moveCompA);
        float radDegree = getRadiantAngle(moveCompA->m_currentDegreeDirection);
        float circleAPosX = args.mapCompA.m_absoluteMapPositionPX.first;
        float circleAPosY = args.mapCompA.m_absoluteMapPositionPX.second;
        float circleBPosX = args.mapCompB.m_absoluteMapPositionPX.first;
        float circleBPosY = args.mapCompB.m_absoluteMapPositionPX.second;
        float distanceX = std::abs(circleAPosX - circleBPosX);
        float distanceY = std::abs(circleAPosY - circleBPosY);
        float hyp = circleCollA.m_ray + circleCollB.m_ray;
        assert(hyp > distanceX && hyp > distanceY);
        float diffY = std::sqrt(hyp * hyp - distanceX * distanceX);
        float diffX = std::sqrt(hyp * hyp - distanceY * distanceY);

        if(std::cos(radDegree) > 0.0f)
        {
            diffX = -diffX;
        }
        if(std::sin(radDegree) < 0.0f)
        {
            diffY = -diffY;
        }
        std::cerr << "diffX================ " << diffX << " diffY============= " << diffY << "\n";
        collisionEjectCircleCircle(args.mapCompA, diffX, diffY);
    }
}

//===================================================================
void CollisionSystem::treatCollisionCircleSegment(CollisionArgs &args,
                                                  const CircleCollisionComponent &circleCollA,
                                                  const SegmentCollisionComponent &segmCollB)
{

}



//===================================================================
CircleCollisionComponent &CollisionSystem::getCircleComponent(uint32_t entityNum)
{
    CircleCollisionComponent *collComp = stairwayToComponentManager().
            searchComponentByType<CircleCollisionComponent>(entityNum,
                                  Components_e::CIRCLE_COLLISION_COMPONENT);
    assert(collComp);
    return *collComp;
}

//===================================================================
RectangleCollisionComponent &CollisionSystem::getRectangleComponent(uint32_t entityNum)
{
    RectangleCollisionComponent *collComp = stairwayToComponentManager().
            searchComponentByType<RectangleCollisionComponent>(entityNum,
                                  Components_e::RECTANGLE_COLLISION_COMPONENT);
    assert(collComp);
    return *collComp;
}

//===================================================================
SegmentCollisionComponent &CollisionSystem::getSegmentComponent(uint32_t entityNum)
{
    SegmentCollisionComponent *collComp = stairwayToComponentManager().
            searchComponentByType<SegmentCollisionComponent>(entityNum,
                                  Components_e::LINE_COLLISION_COMPONENT);
    assert(collComp);
    return *collComp;
}

//===================================================================
MapCoordComponent &CollisionSystem::getMapComponent(uint32_t entityNum)
{
    MapCoordComponent *mapComp = stairwayToComponentManager().
            searchComponentByType<MapCoordComponent>(entityNum,
                                  Components_e::MAP_COORD_COMPONENT);
    assert(mapComp);
    return *mapComp;
}
