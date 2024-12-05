#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>
#include <functional>

struct LogComponent : public ECS::Component
{
    LogComponent() = default;
    std::string m_message;
};
