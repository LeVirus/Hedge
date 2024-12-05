#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

struct SegmentCollisionComponent : public ECS::Component
{
    SegmentCollisionComponent() = default;
    //first point is in map Componnent
    std::pair<PairFloat_t, PairFloat_t> m_points;
    uint32_t m_impactEntity;
    //orientation from first point to second
    float m_degreeOrientation;
    virtual ~SegmentCollisionComponent() = default;
};
