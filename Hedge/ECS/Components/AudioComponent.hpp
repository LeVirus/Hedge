#pragma once


#include <ECS_Headers/Component.hpp>
#include <constants.hpp>
#include <vector>
#include <optional>

typedef unsigned int ALuint;

struct SoundElement
{
    ALuint m_sourceALID, m_bufferALID;
    bool m_toPlay;
};

struct AudioComponent : public ECS::Component
{
    AudioComponent() = default;
    std::vector<std::optional<SoundElement>> m_soundElements;
    ALuint m_maxDistance = MAX_SOUND_DISTANCE;
};
