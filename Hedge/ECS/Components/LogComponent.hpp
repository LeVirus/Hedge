#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>

struct LogComponent : public ECS::Component
{
    LogComponent() = default;
    std::string m_message;
    std::vector<std::string> m_memCharacterPic;
    bool m_activated = false;
};
