#include <cassert>
#include "VisionSystem.hpp"
#include <constants.hpp>
#include <CollisionUtils.hpp>
#include <PhysicalEngine.hpp>
#include <ECS/ECSManager.hpp>
#include <math.h>
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/MapCoordComponent.hpp>
#include <ECS/Components/GeneralCollisionComponent.hpp>
#include <ECS/Components/VisionComponent.hpp>
#include <ECS/Components/CircleCollisionComponent.hpp>
#include <ECS/Components/RectangleCollisionComponent.hpp>
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/MoveableComponent.hpp>
#include <ECS/Components/MemSpriteDataComponent.hpp>
#include <ECS/Components/SpriteTextureComponent.hpp>
#include <ECS/Components/EnemyConfComponent.hpp>
#include <ECS/Components/TimerComponent.hpp>

//===========================================================================
VisionSystem::VisionSystem(const ECSManager *memECSManager) :
    m_memECSManager(memECSManager)
{
    setUsedComponents();
}

//===========================================================================
void VisionSystem::setUsedComponents()
{
    bAddComponentToSystem(Components_e::VISION_COMPONENT);
}

//===========================================================================
void VisionSystem::execSystem()
{
    System::execSystem();
    GeneralCollisionComponent *collComp, *collCompB;
    VisionComponent *visionCompA;
    MapCoordComponent *mapCompA;
    MoveableComponent *moveCompA;
    for(uint32_t i = 0; i < mVectNumEntity.size(); ++i)
    {
        collComp = stairwayToComponentManager().
                searchComponentByType<GeneralCollisionComponent>(mVectNumEntity[i],
                                      Components_e::GENERAL_COLLISION_COMPONENT);
        assert(collComp);
        std::bitset<Components_e::TOTAL_COMPONENTS> bitsetComp;
        bitsetComp[Components_e::MAP_COORD_COMPONENT] = true;
        bitsetComp[Components_e::SPRITE_TEXTURE_COMPONENT] = true;
        std::vector<uint32_t> vectEntities = m_memECSManager->getEntityContainingComponents(bitsetComp);

        visionCompA = stairwayToComponentManager().
                searchComponentByType<VisionComponent>(mVectNumEntity[i], Components_e::VISION_COMPONENT);
        mapCompA = stairwayToComponentManager().
                searchComponentByType<MapCoordComponent>(mVectNumEntity[i], Components_e::MAP_COORD_COMPONENT);
        moveCompA = stairwayToComponentManager().
                searchComponentByType<MoveableComponent>(mVectNumEntity[i], Components_e::MOVEABLE_COMPONENT);
        updateTriangleVisionFromPosition(visionCompA, mapCompA, moveCompA);
        visionCompA->m_vectVisibleEntities.clear();
        for(uint32_t j = 0; j < vectEntities.size(); ++j)
        {
            if(mVectNumEntity[i] == vectEntities[j])
            {
                continue;
            }
            collCompB = stairwayToComponentManager().
                    searchComponentByType<GeneralCollisionComponent>(vectEntities[j],
                                          Components_e::GENERAL_COLLISION_COMPONENT);
            assert(collCompB);
            if((collComp->m_tag == CollisionTag_e::ENEMY_CT && collCompB->m_tag != CollisionTag_e::PLAYER_CT) ||
                    (collCompB->m_tag == CollisionTag_e::DOOR_CT) || (collCompB->m_tag == CollisionTag_e::WALL_CT))
            {
                continue;
            }
            //FAIRE DES TESTS POUR LES COLLISIONS
            treatVisible(visionCompA, vectEntities[j], collCompB->m_shape);
        }
        updateSprites(mVectNumEntity[i], visionCompA->m_vectVisibleEntities);
    }
}

//===========================================================================
void VisionSystem::updateSprites(uint32_t observerEntity,
                                 const std::vector<uint32_t> &vectEntities)
{
    MemSpriteDataComponent *memSpriteComp;
    SpriteTextureComponent *spriteComp;
    TimerComponent *timerComp;
    EnemyConfComponent *enemyConfComp;
    MoveableComponent *enemyMoveComp;
    for(uint32_t i = 0; i < vectEntities.size(); ++i)
    {
        memSpriteComp = stairwayToComponentManager().
                searchComponentByType<MemSpriteDataComponent>(vectEntities[i],
                                                              Components_e::MEM_SPRITE_DATA_COMPONENT);
        if(!memSpriteComp)
        {
            continue;
        }
        enemyConfComp = stairwayToComponentManager().
                searchComponentByType<EnemyConfComponent>(vectEntities[i],
                                                          Components_e::ENEMY_CONF_COMPONENT);
        spriteComp = stairwayToComponentManager().
                searchComponentByType<SpriteTextureComponent>(vectEntities[i],
                                                              Components_e::SPRITE_TEXTURE_COMPONENT);
        timerComp = stairwayToComponentManager().
                searchComponentByType<TimerComponent>(vectEntities[i], Components_e::TIMER_COMPONENT);
        assert(spriteComp);
        assert(timerComp);
        if(enemyConfComp)
        {
            if(enemyConfComp->m_displayMode == EnemyDisplayMode_e::NORMAL)
            {
                if(!enemyConfComp->m_life)
                {
                    enemyConfComp->m_displayMode = EnemyDisplayMode_e::DYING;
                    spriteComp->m_spriteData = memSpriteComp->
                            m_vectSpriteData[static_cast<uint32_t>(EnemySpriteType_e::DYING)];
                    timerComp->m_clockA = std::chrono::system_clock::now();
                }
                else
                {
                    enemyMoveComp = stairwayToComponentManager().
                            searchComponentByType<MoveableComponent>(vectEntities[i],
                                                                     Components_e::MOVEABLE_COMPONENT);
                    assert(enemyMoveComp);
                    enemyConfComp->m_visibleOrientation =
                            getOrientationFromAngle(observerEntity, vectEntities[i],
                                                    enemyMoveComp->m_degreeOrientation);
                    if(!enemyConfComp->m_staticPhase)
                    {
                        enemyConfComp->m_visibleOrientation =
                                static_cast<EnemySpriteType_e>(static_cast<uint32_t>(
                                                                   enemyConfComp->m_visibleOrientation) + 1);
                    }
                    spriteComp->m_spriteData = memSpriteComp->m_vectSpriteData[
                            static_cast<uint32_t>(enemyConfComp->m_visibleOrientation)];
                    std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - timerComp->m_clockA;
                    if(elapsed_seconds.count() > 0.5)
                    {
                        enemyConfComp->m_staticPhase = !enemyConfComp->m_staticPhase;
                        timerComp->m_clockA = std::chrono::system_clock::now();
                        //TESTTT
                        if(enemyConfComp->m_staticPhase)
                        {
                            spriteComp->m_spriteData = memSpriteComp->m_vectSpriteData[
                                    static_cast<uint32_t>(enemyConfComp->m_visibleOrientation)];
                        }
                        else
                        {
                            spriteComp->m_spriteData = memSpriteComp->m_vectSpriteData[
                                    static_cast<uint32_t>(enemyConfComp->m_visibleOrientation) + 1];
                        }
                    }
                }
            }
            else if(enemyConfComp->m_displayMode == EnemyDisplayMode_e::DYING)
            {
                std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() -
                        timerComp->m_clockA;
                if(elapsed_seconds.count() > 0.5)
                {
                    enemyConfComp->m_displayMode = EnemyDisplayMode_e::DEAD;
                    spriteComp->m_spriteData = memSpriteComp->
                            m_vectSpriteData[static_cast<uint32_t>(EnemySpriteType_e::DEAD)];
                }
            }
        }
    }
}

//===========================================================================
EnemySpriteType_e VisionSystem::getOrientationFromAngle(uint32_t observerEntity,
                                                        uint32_t targetEntity,
                                                        float targetDegreeAngle)
{
    MapCoordComponent *observMapComp = stairwayToComponentManager().
            searchComponentByType<MapCoordComponent>(observerEntity,
                                                     Components_e::MAP_COORD_COMPONENT);
    MapCoordComponent *targetMapComp = stairwayToComponentManager().
            searchComponentByType<MapCoordComponent>(targetEntity,
                                                     Components_e::MAP_COORD_COMPONENT);
    assert(observMapComp);
    assert(targetMapComp);
    float observerDegreeAngle = getTrigoAngle(observMapComp->m_absoluteMapPositionPX,
                                              targetMapComp->m_absoluteMapPositionPX);
    float radDiff = getRadiantAngle(observerDegreeAngle - targetDegreeAngle),
            valSin = std::sin(radDiff), valCos = std::cos(radDiff);
    if(valCos > 0.333f && valSin > 0.333f)
    {
        return EnemySpriteType_e::STATIC_MID_BACK_RIGHT_A;
    }
    else if(valCos < -0.333f && valSin > 0.333f)
    {
        return EnemySpriteType_e::STATIC_MID_FRONT_RIGHT_A;
    }
    else if(valCos > 0.333f && valSin < -0.333f)
    {
        return EnemySpriteType_e::STATIC_MID_BACK_LEFT_A;
    }
    else if(valCos < -0.333f && valSin < -0.333f)
    {
        return EnemySpriteType_e::STATIC_MID_FRONT_LEFT_A;
    }
    else if(valSin < -0.5f)
    {
        return EnemySpriteType_e::STATIC_LEFT_A;
    }
    else if(valSin > 0.5f)
    {
        return EnemySpriteType_e::STATIC_RIGHT_A;
    }
    else if(valCos < -0.5f)
    {
        return EnemySpriteType_e::STATIC_FRONT_A;
    }
    else
    {
        return EnemySpriteType_e::STATIC_BACK_A;
    }
}

//===========================================================================
void VisionSystem::treatVisible(VisionComponent *visionComp, uint32_t checkVisibleId,
                                CollisionShape_e shapeElement)
{
    MapCoordComponent *mapCompB = stairwayToComponentManager().
            searchComponentByType<MapCoordComponent>(checkVisibleId, Components_e::MAP_COORD_COMPONENT);
    assert(visionComp);
    assert(mapCompB);
    switch(shapeElement)
    {
    case CollisionShape_e::CIRCLE_C:
    {
        CircleCollisionComponent *circleColl = stairwayToComponentManager().
                searchComponentByType<CircleCollisionComponent>(checkVisibleId, Components_e::CIRCLE_COLLISION_COMPONENT);
        assert(circleColl);
        if(checkTriangleCircleCollision(visionComp->m_triangleVision, mapCompB->m_absoluteMapPositionPX, circleColl->m_ray))
        {
            visionComp->m_vectVisibleEntities.push_back(checkVisibleId);
        }
    }
        break;
    case CollisionShape_e::RECTANGLE_C:
    {
        RectangleCollisionComponent *rectColl = stairwayToComponentManager().
                searchComponentByType<RectangleCollisionComponent>(checkVisibleId, Components_e::RECTANGLE_COLLISION_COMPONENT);
        assert(rectColl);
        if(checkTriangleRectCollision(visionComp->m_triangleVision, {mapCompB->m_absoluteMapPositionPX, rectColl->m_size}))
        {
            visionComp->m_vectVisibleEntities.push_back(checkVisibleId);
        }
    }
        break;
    case CollisionShape_e::SEGMENT_C:
        break;
    }
}

//===========================================================================
void updateTriangleVisionFromPosition(VisionComponent *visionComp, const MapCoordComponent *mapComp,
                                      const MoveableComponent *movComp)
{
    assert(visionComp);
    assert(mapComp);
    assert(movComp);
    visionComp->m_triangleVision[0] = mapComp->m_absoluteMapPositionPX;
    visionComp->m_triangleVision[1] = mapComp->m_absoluteMapPositionPX;
    visionComp->m_triangleVision[2] = mapComp->m_absoluteMapPositionPX;
    //second point of view
    float angleDegree = movComp->m_degreeOrientation - (HALF_CONE_VISION + 10.0f);//QUICK FIX
    float radiantAngle;
    for(uint32_t i = 1; i < 3; ++i)
    {
        radiantAngle = getRadiantAngle(angleDegree);
        visionComp->m_triangleVision[i].first += cos(radiantAngle) * visionComp->m_distanceVisibility;
        visionComp->m_triangleVision[i].second -= sin(radiantAngle) * visionComp->m_distanceVisibility;
        if(i == 2)
        {
            break;
        }
        angleDegree += (CONE_VISION + 20.0f);//QUICK FIX
    }
}
