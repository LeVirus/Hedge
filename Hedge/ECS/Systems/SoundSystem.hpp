#pragma once

#include "ECS_Headers/System.hpp"
#include <cstdint>
#include <optional>
#include <vector>
#include <constants.hpp>

typedef unsigned int ALuint;

using OptUint_t = std::optional<uint32_t>;
class SoundSystem : public ECS::System<Components_e::TOTAL_COMPONENTS>
{
public:
    SoundSystem();
    ~SoundSystem();
    void execSystem()override;
    void play(ALuint source);
    void cleanUpSourceData(ALuint source);
    void cleanUp();
    void stop(ALuint source);
    ALuint createSource(ALuint memSoundBuffer);
    inline void memPlayerEntity(uint32_t entityNum)
    {
        m_playerEntity = entityNum;
    }
    void updateEffectsVolume(uint32_t volume, bool playEffect = true);
    inline uint32_t getEffectsVolume()const
    {
        return m_effectsVolume;
    }
    inline void memAudioMenuSound(ALuint source)
    {
        m_sourceMenuAudio = source;
    }
private:
    std::optional<float> getVolumeFromDistance(uint32_t distantEntity, float maxDistance);
private:
    ALuint m_sourceMenuAudio;
    uint32_t m_playerEntity;
    std::vector<ALuint> m_vectSource;
    uint32_t m_effectsVolume = 100;
};
