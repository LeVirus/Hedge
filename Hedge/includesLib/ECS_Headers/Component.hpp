#pragma once

#include <concepts>

namespace ECS
{
struct Component
{
    ~Component() = default;
protected:
    Component() = default;
};
template <typename S>
concept Component_C = std::derived_from<S, Component>;
}
