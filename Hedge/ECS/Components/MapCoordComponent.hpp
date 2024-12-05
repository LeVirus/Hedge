#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>
#include <functional>

struct MapCoordComponent : public ECS::Component
{
    MapCoordComponent() = default;
    PairUI_t m_coord;
    PairFloat_t m_absoluteMapPositionPX;
};
