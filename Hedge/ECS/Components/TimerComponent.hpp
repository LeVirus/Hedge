#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>
#include <chrono>
#include <ctime>

struct TimerComponent : public ECS::Component
{
    TimerComponent() = default;
    std::chrono::time_point<std::chrono::system_clock> m_clock;
    std::optional<uint32_t> m_timeIntervalOptional;
    uint32_t m_cycleCountA = 0, m_cycleCountB = 0, m_cycleCountC = 0, m_cycleCountD = 0;
    virtual ~TimerComponent() = default;
};

