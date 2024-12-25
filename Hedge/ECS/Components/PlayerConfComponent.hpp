#pragma once

#include <ECS_Headers/Component.hpp>
#include <constants.hpp>
#include <array>
#include <set>

using MapPlayerSprite_t = std::map<PlayerSpriteType_e, PairUI_t>;

enum class MapMode_e
{
    NONE,
    MINI_MAP,
    FULL_MAP
};

enum class PlayerEntities_e
{
    WEAPON,
    AMMO_WRITE,
    TITLE_MENU,
    MENU_ENTRIES,
    LIFE_AMMO_PANNEL,
    MENU_INFO_WRITE,
    ACTION,
    HIT_MELEE,
    LIFE_WRITE,
    NUM_INFO_WRITE,
    DISPLAY_TELEPORT,
    MAP_DETECT_SHAPE,
    LIFE_ICON,
    AMMO_ICON,
    CURSOR_WEAPON_PREVIEW,
    MENU_GENERIC_BACKGROUND,
    MENU_TITLE_BACKGROUND,
    MENU_LEFT_BACKGROUND,
    MENU_RIGHT_LEFT_BACKGROUND,
    MENU_SELECTED_LINE,
    TOTAL
};

struct PlayerConfComponent : public ECS::Component
{
    PlayerConfComponent()
    {
        m_currentAim.fill(false);
        m_currentAim[static_cast<uint32_t>(PlayerAimDirection_e::RIGHT)] = true;
    }
    void takeDamage(uint32_t damage)
    {
        m_takeDamage = true;
        if(m_life <= damage)
        {
            m_life = 0;
            m_inMovement = false;
        }
        else
        {
            m_life -= damage;
        }
    }
    bool m_playerShoot = false, m_takeDamage = false, m_inMovement = false, m_inputModified, m_firstMenu = true,
    m_pickItem = false, m_crush = false, m_frozen = false, m_insideWall = false, m_keyboardInputMenuMode = true;
    std::pair<bool, std::string> m_infoWriteData = {false, ""};
    std::set<uint32_t> m_card;
    uint32_t m_currentCursorPos = 0, m_currentSelectedSaveFile, m_life = 100, m_currentCustomLevelCusorMenu, m_levelToLoad, m_velocityInertie = 0, m_memEntityAssociated, m_currentSprite,
        m_standardSpriteInterval = 0.5 / FPS_VALUE;
    std::array<uint32_t, static_cast<uint32_t>(PlayerEntities_e::TOTAL)> m_vectEntities;
    //display only weapons when changing weapons
    std::array<uint32_t, 6> m_vectPossessedWeaponsPreviewEntities;
    std::optional<uint32_t> m_secretsFound, m_enemiesKilled;
    std::optional<std::pair<uint32_t, Direction_e>> m_currentCheckpoint;
    std::optional<PairUI_t> m_checkpointReached;
    MenuMode_e m_menuMode, m_previousMenuMode;
    MapMode_e m_mapMode = MapMode_e::NONE;
    MoveOrientation_e m_previousMove = MoveOrientation_e::FORWARD;
    MapPlayerSprite_t m_mapSpriteAssociate;
    PlayerSpriteType_e m_spriteType = PlayerSpriteType_e::STATIC;
    bool m_currentDirectionRight = true;
    std::array<bool, static_cast<uint32_t>(PlayerAimDirection_e::TOTAL)> m_currentAim;
    virtual ~PlayerConfComponent() = default;
};

struct PlayerData
{

    //after dying animation the ennemy is represented by the last sprite
    //contained in m_dyingSprites
    std::vector<uint16_t> m_staticSprites, m_runLeftSprites, m_runRightSprites,
        m_runShootLeftSprites, m_runShootRightSprites, m_runShootUpRightSprites, m_runShootDownRightSprites, m_runShootUpLeftSprites, m_runShootDownLeftSprites, m_runShootUpSprites,
        m_shootLeftSprites, m_shootRightSprites, m_shootUpRightSprites, m_shootDownRightSprites, m_shootUpLeftSprites, m_shootDownLeftSprites, m_shootUpSprites,
        m_jumpSprites, m_jumpShootLeftSprites, m_jumpShootRightSprites, m_jumpShootUpRightSprites, m_jumpShootDownRightSprites, m_jumpShootUpLeftSprites, m_jumpShootDownLeftSprites,
        m_jumpShootUpSprites, m_jumpShootDownSprites, m_crouchLeftSprites, m_crouchRightSprites;

    //In Game sprite size in % relative to a tile
    PairDouble_t m_inGameSpriteSize;
    std::pair<uint32_t, uint32_t> m_TileGamePosition;
    std::string m_visibleShootID, m_impactID, m_dropedObjectID;
    std::string m_detectBehaviourSoundFile, m_attackSoundFile, m_deathSoundFile;
    uint32_t m_attackPower, m_life;
    std::optional<uint32_t> m_meleeDamage, m_simultaneousShot;
    bool m_frozenOnAttack;
    std::optional<PairUI_t> m_endLevelPos;
    std::optional<float> m_damageZone;
    float m_velocity, m_shotVelocity;
    bool m_traversable, m_meleeOnly;
};
