#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>
#include <functional>
#include <optional>
#include <set>

using PairFloat_t = std::pair<float, float>;

struct MoveableComponent : public ECS::Component
{
    MoveableComponent() = default;
    float m_degreeOrientation;
    //used for movement (forward backward strafe...)
    float m_currentDegreeMoveDirection;
    float m_velocity = 3.0f;
    float m_rotationAngle = 3.000f;
    //first eject velocity, SECOND Time
    std::optional<std::pair<float, double>> m_ejectData = std::nullopt;
    std::set<uint32_t> m_entitiesCollTreat;
    //first direction, second entity, third vertical == true or lateral == false
    virtual ~MoveableComponent() = default;
};
