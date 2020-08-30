#include "FirstPersonDisplaySystem.hpp"
#include <CollisionUtils.hpp>
#include <PhysicalEngine.hpp>
#include <ECS/Components/VisionComponent.hpp>
#include <ECS/Components/GeneralCollisionComponent.hpp>
#include <ECS/Components/MapCoordComponent.hpp>
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/RectangleCollisionComponent.hpp>
#include <ECS/Components/MoveableComponent.hpp>
#include <ECS/Components/SpriteTextureComponent.hpp>
#include <ECS/Components/CircleCollisionComponent.hpp>
#include <PictureData.hpp>
#include <cmath>

//===================================================================
FirstPersonDisplaySystem::FirstPersonDisplaySystem()
{
    setUsedComponents();
}

//===================================================================
void FirstPersonDisplaySystem::setUsedComponents()
{
    bAddComponentToSystem(Components_e::VISION_COMPONENT);
}

//===================================================================
void FirstPersonDisplaySystem::execSystem()
{
    System::execSystem();
    confCompVertexMemEntities();
    drawVertex();
}

//===================================================================
void FirstPersonDisplaySystem::confCompVertexMemEntities()
{
    uint32_t numEntity = mVectNumEntity.size();
    m_numVertexToDraw.resize(numEntity);
    //treat one player
    uint32_t toRemove = 0;
    VisionComponent *visionComp;
    MoveableComponent *moveComp;
    MapCoordComponent *mapCompA;
    GeneralCollisionComponent *genCollComp;
    MapCoordComponent *mapCompB;
    for(uint32_t i = 0; i < numEntity; ++i)
    {
        visionComp = stairwayToComponentManager().
                searchComponentByType<VisionComponent>(mVectNumEntity[i], Components_e::VISION_COMPONENT);
        moveComp = stairwayToComponentManager().
                searchComponentByType<MoveableComponent>(mVectNumEntity[i], Components_e::MOVEABLE_COMPONENT);
        mapCompA = stairwayToComponentManager().
                searchComponentByType<MapCoordComponent>(mVectNumEntity[i], Components_e::MAP_COORD_COMPONENT);
        assert(visionComp);
        assert(mapCompA);
        assert(moveComp);
        float leftAngleVision = moveComp->m_degreeOrientation + (visionComp->m_coneVision / 2);
        if(leftAngleVision > 360.0f)
        {
            leftAngleVision -= 360.0f;
        }
        toRemove = 0;
        m_numVertexToDraw[i] = visionComp->m_vectVisibleEntities.size();
        m_entitiesNumMem.clear();
        for(uint32_t j = 0; j < m_numVertexToDraw[i]; ++j)
        {
            genCollComp = stairwayToComponentManager().
                    searchComponentByType<GeneralCollisionComponent>(visionComp->m_vectVisibleEntities[j],
                                                                     Components_e::GENERAL_COLLISION_COMPONENT);
            mapCompB = stairwayToComponentManager().
                    searchComponentByType<MapCoordComponent>(visionComp->m_vectVisibleEntities[j],
                                                             Components_e::MAP_COORD_COMPONENT);
            assert(mapCompB);
            assert(genCollComp);
            treatDisplayEntity(genCollComp, mapCompA, mapCompB, visionComp, toRemove, leftAngleVision, j);
        }
        m_numVertexToDraw[i] -= toRemove;
    }
}

//===================================================================
void FirstPersonDisplaySystem::treatDisplayEntity(GeneralCollisionComponent *genCollComp, MapCoordComponent *mapCompA,
                                                  MapCoordComponent *mapCompB, VisionComponent *visionComp,
                                                  uint32_t &toRemove, float leftAngleVision, uint32_t numIteration)
{
    if(genCollComp->m_tag == CollisionTag_e::WALL_CT)
    {
        float distance[4];
        pairFloat_t absolPos[4];
        float lateralPos[3];
        float observerAngle = leftAngleVision + visionComp->m_coneVision / 2.0f;
        observerAngle += 270.0f;
        if(observerAngle > 360.0f)observerAngle -= 360.0f;
        bool pointIn[3];
        bool leftLimit[3];
        //calculate distance
        fillWallEntitiesData(visionComp->m_vectVisibleEntities[numIteration], absolPos, distance,
                             mapCompA, mapCompB, getRadiantAngle(observerAngle), visionComp, pointIn, leftLimit);

        float currentTrigoAngle;
        //calculate all 3 display position
        for(uint32_t i = 0; i < 3; ++i)
        {
            //A virer pour le calcul distance
            if(!pointIn[i])
            {
                if(leftLimit[i])
                {
                    lateralPos[i] = -1.0f;
                }
                else
                {
                    lateralPos[i] = visionComp->m_coneVision + 1.0f;
                }
                continue;
            }
            //A virer pour le calcul distance
            currentTrigoAngle =  getTrigoAngle(mapCompA->m_absoluteMapPositionPX, absolPos[i]);
            if(std::abs(currentTrigoAngle - leftAngleVision) > 150.0f)
            {
                currentTrigoAngle += 360.0f;
            }
            lateralPos[i] = leftAngleVision - currentTrigoAngle;
            //tmp -30.0f
            if(lateralPos[i] < -60.0f)
            {
                //Quick fix
                lateralPos[i] = (leftAngleVision + 360.0f) - getTrigoAngle(mapCompA->m_absoluteMapPositionPX, absolPos[i]);
            }
        }
        //conf screen position
        confWallEntityVertex(visionComp->m_vectVisibleEntities[numIteration],
                             visionComp, lateralPos, distance);
        fillVertexFromEntitie(visionComp->m_vectVisibleEntities[numIteration],
                              numIteration, distance[2]);
    }
    else
    {
        pairFloat_t centerPosB = getCenterPosition(mapCompB, genCollComp, visionComp->m_vectVisibleEntities[numIteration]);
        float distance = getDistance(mapCompA->m_absoluteMapPositionPX, centerPosB);
        if(distance > visionComp->m_distanceVisibility)
        {
            ++toRemove;
            return;
        }
        float lateralPos = leftAngleVision - getTrigoAngle(mapCompA->m_absoluteMapPositionPX, centerPosB);
        confNormalEntityVertex(visionComp->m_vectVisibleEntities[numIteration], visionComp, lateralPos, distance);
        fillVertexFromEntitie(visionComp->m_vectVisibleEntities[numIteration], numIteration, distance);
    }
}

//===================================================================
void FirstPersonDisplaySystem::confWallEntityVertex(uint32_t numEntity, VisionComponent *visionComp,
                                                    float lateralPosDegree[], float distance[])
{
    PositionVertexComponent *positionComp = stairwayToComponentManager().
            searchComponentByType<PositionVertexComponent>(numEntity, Components_e::POSITION_VERTEX_COMPONENT);
    SpriteTextureComponent *spriteComp = stairwayToComponentManager().
                searchComponentByType<SpriteTextureComponent>(numEntity, Components_e::SPRITE_TEXTURE_COMPONENT);
    assert(spriteComp);
    assert(positionComp);
    assert(visionComp);

    bool excludeZero = (lateralPosDegree[0] > lateralPosDegree[1]) &&
            ((lateralPosDegree[0] < lateralPosDegree[2]) || (lateralPosDegree[0] < lateralPosDegree[1])) &&
            (lateralPosDegree[0] > lateralPosDegree[2]);
    bool excludeTwo = (lateralPosDegree[2] > lateralPosDegree[0]) &&
            ((lateralPosDegree[2] < lateralPosDegree[1]) || (lateralPosDegree[2] < lateralPosDegree[0])) &&
            (lateralPosDegree[2] > lateralPosDegree[1]);
    positionComp->m_vertex.resize(6);
    //convert to GL context
    float depthPos = (spriteComp->m_glFpsSize.second) / distance[0];
    float halfVerticalSize = depthPos / 2.0f;
    float lateralPosGL = (lateralPosDegree[0] / visionComp->m_coneVision * 2.0f) - 1.0f;
    //convert to GL context
    float depthPosMid = (spriteComp->m_glFpsSize.second) / distance[1];
    float halfVerticalSizeMid = depthPosMid / 2.0f;
    float lateralPosGLMid = (lateralPosDegree[1] / visionComp->m_coneVision * 2.0f) - 1.0f;


    positionComp->m_vertex[0].first = lateralPosGL;
    positionComp->m_vertex[0].second = halfVerticalSize;
    positionComp->m_vertex[1].first = lateralPosGLMid;
    positionComp->m_vertex[1].second = halfVerticalSizeMid;

    positionComp->m_vertex[2].first = lateralPosGLMid;
    positionComp->m_vertex[2].second = -halfVerticalSizeMid;
    positionComp->m_vertex[3].first = lateralPosGL;
    positionComp->m_vertex[3].second = -halfVerticalSize;

    if(excludeZero || excludeTwo)
    {
        positionComp->m_vertex.resize(4);
        return;
    }
    float depthPosMax = (spriteComp->m_glFpsSize.second) / distance[2];
    float halfVerticalSizeMax = depthPosMax / 2.0f;
    float lateralPosMaxGL = (lateralPosDegree[2] / visionComp->m_coneVision * 2.0f) - 1.0f;
    positionComp->m_vertex[4].first = lateralPosMaxGL;
    positionComp->m_vertex[4].second = halfVerticalSizeMax;
    positionComp->m_vertex[5].first = lateralPosMaxGL;
    positionComp->m_vertex[5].second = -halfVerticalSizeMax;
}

//===================================================================
void FirstPersonDisplaySystem::fillAbsolAndDistanceWall(pairFloat_t absolPos[],
                                                        float distance[],
                                                        MapCoordComponent *mapCompA,
                                                        MapCoordComponent *mapCompB,
                                                        uint32_t numEntity)
{
    RectangleCollisionComponent *rectComp = stairwayToComponentManager().
            searchComponentByType<RectangleCollisionComponent>(numEntity, Components_e::RECTANGLE_COLLISION_COMPONENT);
    assert(rectComp);
    //get all points WALL rect position
    //up left
    absolPos[0] = mapCompB->m_absoluteMapPositionPX;
    absolPos[1] = {mapCompB->m_absoluteMapPositionPX.first + rectComp->m_size.first,
                   mapCompB->m_absoluteMapPositionPX.second};
    absolPos[2] = {mapCompB->m_absoluteMapPositionPX.first + rectComp->m_size.first,
                   mapCompB->m_absoluteMapPositionPX.second + rectComp->m_size.second};
    absolPos[3] = {mapCompB->m_absoluteMapPositionPX.first,
                   mapCompB->m_absoluteMapPositionPX.second + rectComp->m_size.second};

    //up left
    distance[0] = getDistance(mapCompA->m_absoluteMapPositionPX, absolPos[0]);
    //up right
    distance[1] = getDistance(mapCompA->m_absoluteMapPositionPX, absolPos[1]);
    //down right
    distance[2] = getDistance(mapCompA->m_absoluteMapPositionPX, absolPos[2]);
    //down left
    distance[3] = getDistance(mapCompA->m_absoluteMapPositionPX, absolPos[3]);

    uint32_t minVal = getMinValueFromEntries(distance);
    //second display
    if(minVal != 1)
    {
        std::swap(distance[minVal], distance[1]);
        std::swap(absolPos[minVal], absolPos[1]);
    }
    uint32_t maxVal = getMaxValueFromEntries(distance);
    //do not display
    if(maxVal != 3)
    {
        std::swap(distance[3], distance[maxVal]);
        std::swap(absolPos[3], absolPos[maxVal]);
    }

    float trigoAngleA =  getTrigoAngle(mapCompA->m_absoluteMapPositionPX, absolPos[0]),
    trigoAngleC =  getTrigoAngle(mapCompA->m_absoluteMapPositionPX, absolPos[2]);
    if(trigoAngleA < trigoAngleC)
    {
        std::swap(distance[0], distance[2]);
        std::swap(absolPos[0], absolPos[2]);
    }
}

//===================================================================
void FirstPersonDisplaySystem::fillWallEntitiesData(uint32_t numEntity, pairFloat_t absolPos[],
                                                    float distance[],
                                                    MapCoordComponent *mapCompA,
                                                    MapCoordComponent *mapCompB,
                                                    float observerAngle,
                                                    VisionComponent *visionComp,
                                                    bool pointIn[], bool outLeft[])
{
    float distanceReal[4];
    fillAbsolAndDistanceWall(absolPos, distanceReal, mapCompA, mapCompB, numEntity);
    float pointAngleVision[3];
    float anglePoint;
    for(uint32_t i = 0; i < 3; ++i)
    {
        anglePoint = getRadiantAngle(getTrigoAngle(mapCompA->m_absoluteMapPositionPX, absolPos[i]));
        if(std::abs(anglePoint - observerAngle) > PI)
        {
            if(anglePoint < observerAngle)
            {
                observerAngle -= PI_DOUBLE;
            }
            else
            {
                observerAngle += PI_DOUBLE;
            }
        }
        pointAngleVision[i] = anglePoint - observerAngle;
        pointIn[i] = std::abs(pointAngleVision[i]) < PI_QUARTER;
        //mem limit left or right
        if(!pointIn[i])
        {
            outLeft[i] = anglePoint > observerAngle;
        }
    }
    std::optional<uint32_t> j;
    for(uint32_t i = 0; i < 3; ++i)
    {
        //standard case
        if(pointIn[i])
        {
            distance[i] = getCameraDistance(mapCompA->m_absoluteMapPositionPX,
                                            absolPos[i], observerAngle) / LEVEL_TILE_SIZE_PX;;
        }
        //out of screen limit case
        else
        {
            j = getLimitIndex(pointIn, distanceReal, i);
            if(!j)
            {
                return;
            }
            pairFloat_t limitPoint = getPointCameraLimitWall(mapCompA->m_absoluteMapPositionPX,
                                                             observerAngle, absolPos[i],
                                                             absolPos[*j], outLeft[i], visionComp);
            distance[i] = getCameraDistance(mapCompA->m_absoluteMapPositionPX,
                                            limitPoint, observerAngle, true) / LEVEL_TILE_SIZE_PX;
            bool breakLoop;
            modifTempTextureBound(numEntity, outLeft[i], absolPos[i], limitPoint, absolPos[*j], {i, *j}, breakLoop);
            if(breakLoop)
            {
                break;
            }
        }
    }
}

std::optional<uint32_t> getLimitIndex(const bool pointIn[], const float distanceReal[], uint32_t i)
{
    if(i == 0 || i == 2)
    {
        return 1;
    }
    else
    {
        if(pointIn[0] && !pointIn[2])
        {
            return 0;
        }
        else if(!pointIn[0] && pointIn[2])
        {
            return 2;
        }
        else if(pointIn[0] && pointIn[2])
        {
            return (distanceReal[0] < distanceReal[2]) ? 0 : 2;
        }
        else
        {
            return {};
        }
    }
}

//===================================================================
void FirstPersonDisplaySystem::modifTempTextureBound(uint32_t numEntity, bool outLeft, const pairFloat_t &outPoint,
                                                     const pairFloat_t &limitPoint, const pairFloat_t &linkPoint,
                                                     const pairUI_t &coordPoints, bool &breakLoop)
{
    breakLoop = false;
    SpriteTextureComponent *spriteComp = stairwayToComponentManager().
            searchComponentByType<SpriteTextureComponent>(numEntity, Components_e::SPRITE_TEXTURE_COMPONENT);
    assert(spriteComp);
    float total, diff, result;

    //!ATTENTION les textures ne sont pas toujours affichées de gauche a droite!!!
    //! MAINTENANT OUI EN PRINCIPE
    //Y case
    if(std::abs(outPoint.first - limitPoint.first) < 0.1f)
    {
        diff = std::abs(outPoint.second - limitPoint.second);
        total = std::abs(outPoint.second - linkPoint.second);
    }
    //X case
    else
    {
        diff = std::abs(outPoint.first - limitPoint.first);
        total = std::abs(outPoint.first - linkPoint.first);
    }
    if(total < 0.01f)
    {
        result = diff;
    }
    else
    {
        result = diff / total;
    }
    spriteComp->fillWallContainer();
    spriteComp->m_limitWallPointActive = true;
    if(outLeft)
    {
        if(coordPoints.first == 0)//OK
        {
            spriteComp->m_limitWallSpriteData->at(0).first = result;
            spriteComp->m_limitWallSpriteData->at(3).first = result;
        }
        else if(coordPoints.first == 1)//OK
        {
            spriteComp->m_limitWallSpriteData->at(4).first = result;
            spriteComp->m_limitWallSpriteData->at(7).first = result;
        }
    }
    else
    {
        result = std::abs(1 - result);
        if(coordPoints.first == 2)//OK
        {
            spriteComp->m_limitWallSpriteData->at(5).first = result;
            spriteComp->m_limitWallSpriteData->at(6).first = result;
        }
        else if(coordPoints.first == 1)//KO
        {
            spriteComp->m_limitWallSpriteData->at(1).first = result;
            spriteComp->m_limitWallSpriteData->at(2).first = result;
            breakLoop = true;
        }
    }
}

//===================================================================
pairFloat_t FirstPersonDisplaySystem::getPointCameraLimitWall(const pairFloat_t &pointObserver, float observerAngle,
                                                              const pairFloat_t &outPoint, const pairFloat_t &linkPoint,
                                                              bool leftLimit, VisionComponent *visionComp)
{
    pairFloat_t pointReturn = outPoint;
    observerAngle = getDegreeAngle(observerAngle);
    float limitAngle = leftLimit ? observerAngle + (visionComp->m_coneVision / 2) :
                                   observerAngle - (visionComp->m_coneVision / 2);
    if(limitAngle < 0.0f)
    {
        limitAngle += 360.0f;
    }
    else if(limitAngle > 360.0f)
    {
        limitAngle -= 360.0f;
    }
    limitAngle = getQuarterAngle(limitAngle);
    limitAngle = getRadiantAngle(limitAngle);
    float correction, memDiff;
    //X mod
    if(std::abs(outPoint.first - linkPoint.first) > 0.3f)
    {
        memDiff = std::abs(outPoint.second - pointObserver.second);
        if(limitAngle <= 0.01f)
        {
            correction = memDiff;
        }
        else
        {
            //TRIGO CALC
            correction = memDiff / std::tan(limitAngle);
        }
        //need only distance so no need to add sense
        if(outPoint.first > pointObserver.first)
        {
            pointReturn.first = pointObserver.first + correction;
        }
        else
        {
            pointReturn.first = pointObserver.first - correction;
        }
    }
    //Y Mod
    else
    {
        memDiff = std::abs(outPoint.first - pointObserver.first);
        if(limitAngle <= 0.01f)
        {
            correction = memDiff;
        }
        else
        {
            correction = std::abs(std::tan(limitAngle) * memDiff);
        }
        if(outPoint.second > pointObserver.second)
        {
            pointReturn.second = pointObserver.second + correction;
        }
        else
        {
            pointReturn.second = pointObserver.second - correction;
        }
    }
    return pointReturn;
}

//===================================================================
float getQuarterAngle(float angle)
{
    if(angle < 89.0f)
    {
        return angle;
    }
    //investigate why
//    else if(angle < 179.0f)
//    {
//        return std::abs(angle - 180.0f);
//    }
    else if(angle < 269.0f)
    {
        return std::abs(angle - 180.0f);
    }
    else
    {
        return std::abs(angle - 360.0f);
    }
}

//===================================================================
void FirstPersonDisplaySystem::fillVertexFromEntitie(uint32_t numEntity, uint32_t numIteration, float distance)
{
    //use 1 vertex for 1 sprite for beginning
    if(numIteration < m_vectVerticesData.size())
    {
        m_vectVerticesData[numIteration].clear();
    }
    else
    {
        m_vectVerticesData.emplace_back(VerticesData(Shader_e::TEXTURE_S));
        assert(numIteration < m_vectVerticesData.size());
    }
    PositionVertexComponent *posComp = stairwayToComponentManager().
            searchComponentByType<PositionVertexComponent>(numEntity,
                                                           Components_e::POSITION_VERTEX_COMPONENT);
    SpriteTextureComponent *spriteComp = stairwayToComponentManager().
            searchComponentByType<SpriteTextureComponent>(numEntity,
                                                          Components_e::SPRITE_TEXTURE_COMPONENT);
    assert(posComp);
    assert(spriteComp);
    m_entitiesNumMem.insert(EntityData(distance, static_cast<Texture_t>(spriteComp->m_spriteData->m_textureNum), numIteration));

    m_vectVerticesData[numIteration].loadVertexTextureComponent(*posComp, *spriteComp);
}

//===================================================================
void FirstPersonDisplaySystem::setVectTextures(std::vector<Texture> &vectTexture)
{
    m_ptrVectTexture = &vectTexture;
    m_vectVerticesData.reserve(50);
}

//===================================================================
void FirstPersonDisplaySystem::confNormalEntityVertex(uint32_t numEntity, VisionComponent *visionComp,
                                                      float lateralPosDegree, float distance)
{
    PositionVertexComponent *positionComp = stairwayToComponentManager().
            searchComponentByType<PositionVertexComponent>(numEntity, Components_e::POSITION_VERTEX_COMPONENT);
    SpriteTextureComponent *spriteComp = stairwayToComponentManager().
                searchComponentByType<SpriteTextureComponent>(numEntity, Components_e::SPRITE_TEXTURE_COMPONENT);
    assert(spriteComp);
    assert(positionComp);
    assert(visionComp);
    positionComp->m_vertex.resize(4);
    //convert to GL context
    float lateralPosGL = (lateralPosDegree / visionComp->m_coneVision * 2.0f) - 1.0f;
    float depthPos = std::abs((distance / visionComp->m_distanceVisibility) - 1.0f);
    float halfLateralSize = depthPos / spriteComp->m_glFpsSize.first / 2;
    float halfVerticalSize = depthPos / spriteComp->m_glFpsSize.second / 2;
    positionComp->m_vertex[0].first = lateralPosGL - halfLateralSize;
    positionComp->m_vertex[0].second = halfVerticalSize;
    positionComp->m_vertex[1].first = lateralPosGL + halfLateralSize;
    positionComp->m_vertex[1].second = halfVerticalSize;
    positionComp->m_vertex[2].first = lateralPosGL + halfLateralSize;
    positionComp->m_vertex[2].second = -halfVerticalSize;
    positionComp->m_vertex[3].first = lateralPosGL - halfLateralSize;
    positionComp->m_vertex[3].second = -halfVerticalSize;
}

//===================================================================
void FirstPersonDisplaySystem::drawVertex()
{
    m_shader->use();

    //DONT WORK for multiple player
    for(uint32_t i = 0; i < mVectNumEntity.size(); ++i)
    {
        std::set<EntityData>::const_iterator it = m_entitiesNumMem.begin();
        uint32_t numEntity;
        for(;it != m_entitiesNumMem.end(); ++it)
        {
            numEntity = it->m_entityNum;
            assert(numEntity < m_vectVerticesData.size());
            m_ptrVectTexture->operator[](it->m_textureNum).bind();
            m_vectVerticesData[numEntity].confVertexBuffer();
            m_vectVerticesData[numEntity].drawElement();
        }
    }
}

//===================================================================
void FirstPersonDisplaySystem::setShader(Shader &shader)
{
    m_shader = &shader;
}

//===================================================================
pairFloat_t FirstPersonDisplaySystem::getCenterPosition(const MapCoordComponent *mapComp,
                                                        GeneralCollisionComponent *genCollComp, float numEntity)
{
    assert(mapComp);
    assert(genCollComp);
    switch (genCollComp->m_shape)
    {
    case CollisionShape_e::CIRCLE_C:
    case CollisionShape_e::SEGMENT_C://TMP
        break;
    case CollisionShape_e::RECTANGLE_C:
        RectangleCollisionComponent *rectCollComp = stairwayToComponentManager().
                searchComponentByType<RectangleCollisionComponent>(numEntity, Components_e::RECTANGLE_COLLISION_COMPONENT);
        assert(rectCollComp);
        return {mapComp->m_absoluteMapPositionPX.first + rectCollComp->m_size.first / 2,
                    mapComp->m_absoluteMapPositionPX.second + rectCollComp->m_size.second / 2};
    }
    return mapComp->m_absoluteMapPositionPX;
}



uint32_t getMinValueFromEntries(const float distance[])
{
    uint32_t val = 0;
    val = (distance[0] < distance[1]) ? 0 : 1;
    val = (distance[val] < distance[2]) ? val : 2;
    val = (distance[val] < distance[3]) ? val : 3;
    return val;
}

uint32_t getMaxValueFromEntries(const float distance[])
{
    uint32_t val = 0;
    val = (distance[0] > distance[1]) ? 0 : 1;
    val = (distance[val] > distance[2]) ? val : 2;
    val = (distance[val] > distance[3]) ? val : 3;
    return val;
}


