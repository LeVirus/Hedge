#pragma once

#include "ECS/Components/AudioComponent.hpp"
#include "ECS/Components/CheckpointComponent.hpp"
#include "ECS/Components/CircleCollisionComponent.hpp"
#include "ECS/Components/ColorVertexComponent.hpp"
#include "ECS/Components/EnemyConfComponent.hpp"
#include "ECS/Components/GeneralCollisionComponent.hpp"
#include "ECS/Components/InputComponent.hpp"
#include "ECS/Components/LogComponent.hpp"
#include "ECS/Components/MapCoordComponent.hpp"
#include "ECS/Components/MemPositionsVertexComponents.hpp"
#include "ECS/Components/MemSpriteDataComponent.hpp"
#include "ECS/Components/MoveableComponent.hpp"
#include "ECS/Components/ObjectConfComponent.hpp"
#include "ECS/Components/PlayerConfComponent.hpp"
#include "ECS/Components/PositionVertexComponent.hpp"
#include "ECS/Components/RectangleCollisionComponent.hpp"
#include "ECS/Components/SegmentCollisionComponent.hpp"
#include "ECS/Components/ShotConfComponent.hpp"
#include "ECS/Components/SpriteTextureComponent.hpp"
#include "ECS/Components/TimerComponent.hpp"
#include "ECS/Components/WeaponComponent.hpp"
#include "ECS/Components/WriteComponent.hpp"
#include "ECS/Components/GravityComponent.hpp"
#include <ECS_Headers/ComponentsManager.hpp>
#include <stdint.h>
#include <constants.hpp>

template <typename S>
concept Component_C = std::derived_from<S, ECS::Component>;

template <uint32_t N, Component_C... C>
class ComponentManagerExtend : public ECS::ComponentsManager<N, C...>
{
public:
    ComponentManagerExtend()
    {
        // reserveContainer(500);
    }
    virtual ~ComponentManagerExtend() = default;
    virtual void instanciateComponentFromEntity(uint32_t numEntity, const std::array<uint32_t, N> &vect)override
    {
        std::array<VectUI_t, N> &vectEntity = ECS::ComponentsManager<N, C...>::m_refComponents[numEntity];
        for(uint32_t i = 0; i < Components_e::TOTAL_COMPONENTS; ++i)
        {
            if(vect[i] == 0)
            {
                continue;
            }
            switch(i)
            {
            case Components_e::POSITION_VERTEX_COMPONENT:
                treatNewComponent<Components_e::POSITION_VERTEX_COMPONENT, PositionVertexComponent>(vectEntity, vect);
                break;
            case Components_e::SPRITE_TEXTURE_COMPONENT:
                treatNewComponent<Components_e::SPRITE_TEXTURE_COMPONENT, SpriteTextureComponent>(vectEntity, vect);
                break;
            case Components_e::MEM_SPRITE_DATA_COMPONENT:
                treatNewComponent<Components_e::MEM_SPRITE_DATA_COMPONENT, MemSpriteDataComponent>(vectEntity, vect);
                break;
            case Components_e::COLOR_VERTEX_COMPONENT:
                treatNewComponent<Components_e::COLOR_VERTEX_COMPONENT, ColorVertexComponent>(vectEntity, vect);
                break;
            case Components_e::MAP_COORD_COMPONENT:
                treatNewComponent<Components_e::MAP_COORD_COMPONENT, MapCoordComponent>(vectEntity, vect);
                break;
            case Components_e::INPUT_COMPONENT:
                treatNewComponent<Components_e::INPUT_COMPONENT, InputComponent>(vectEntity, vect);
                break;
            case Components_e::CIRCLE_COLLISION_COMPONENT:
                treatNewComponent<Components_e::CIRCLE_COLLISION_COMPONENT, CircleCollisionComponent>(vectEntity, vect);
                break;
            case Components_e::SEGMENT_COLLISION_COMPONENT:
                treatNewComponent<Components_e::SEGMENT_COLLISION_COMPONENT, SegmentCollisionComponent>(vectEntity, vect);
                break;
            case Components_e::RECTANGLE_COLLISION_COMPONENT:
                treatNewComponent<Components_e::RECTANGLE_COLLISION_COMPONENT, RectangleCollisionComponent>(vectEntity, vect);
                break;
            case Components_e::GENERAL_COLLISION_COMPONENT:
                treatNewComponent<Components_e::GENERAL_COLLISION_COMPONENT, GeneralCollisionComponent>(vectEntity, vect);
                break;
            case Components_e::MOVEABLE_COMPONENT:
                treatNewComponent<Components_e::MOVEABLE_COMPONENT, MoveableComponent>(vectEntity, vect);
                break;
            case Components_e::TIMER_COMPONENT:
                treatNewComponent<Components_e::TIMER_COMPONENT, TimerComponent>(vectEntity, vect);
                break;
            case Components_e::PLAYER_CONF_COMPONENT:
                treatNewComponent<Components_e::PLAYER_CONF_COMPONENT, PlayerConfComponent>(vectEntity, vect);
                break;
            case Components_e::ENEMY_CONF_COMPONENT:
                treatNewComponent<Components_e::ENEMY_CONF_COMPONENT, EnemyConfComponent>(vectEntity, vect);
                break;
            case Components_e::MEM_POSITIONS_VERTEX_COMPONENT:
                treatNewComponent<Components_e::MEM_POSITIONS_VERTEX_COMPONENT, MemPositionsVertexComponents>(vectEntity, vect);
                break;
            case Components_e::WRITE_COMPONENT:
                treatNewComponent<Components_e::WRITE_COMPONENT, WriteComponent>(vectEntity, vect);
                break;
            case Components_e::SHOT_CONF_COMPONENT:
                treatNewComponent<Components_e::SHOT_CONF_COMPONENT, ShotConfComponent>(vectEntity, vect);
                break;
            case Components_e::OBJECT_CONF_COMPONENT:
                treatNewComponent<Components_e::OBJECT_CONF_COMPONENT, ObjectConfComponent>(vectEntity, vect);
                break;
            case Components_e::WEAPON_COMPONENT:
                treatNewComponent<Components_e::WEAPON_COMPONENT, WeaponComponent>(vectEntity, vect);
                break;
            case Components_e::AUDIO_COMPONENT:
                treatNewComponent<Components_e::AUDIO_COMPONENT, AudioComponent>(vectEntity, vect);
                break;
            case Components_e::CHECKPOINT_COMPONENT:
                treatNewComponent<Components_e::CHECKPOINT_COMPONENT, CheckpointComponent>(vectEntity, vect);
                break;
            case Components_e::LOG_COMPONENT:
                treatNewComponent<Components_e::LOG_COMPONENT, LogComponent>(vectEntity, vect);
                break;
            case Components_e::GRAVITY_COMPONENT:
                treatNewComponent<Components_e::GRAVITY_COMPONENT, GravityComponent>(vectEntity, vect);
                break;
            case Components_e::TOTAL_COMPONENTS:
                assert(false);
                break;
            }

        }

                // std::unique_ptr<std::tuple<std::vector<C>...>> m_tup;
        // std::set<uint32_t> m_cacheDeletedEntities;
        // //cache index of entities's component
        // std::vector<std::array<VectUI_t, N>> m_refComponents;
        // std::array<std::vector<uint32_t>, N> m_refDelComponents;
    }
private:
    void reserveContainer(uint32_t size)
    {
        //reserve all components container
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::POSITION_VERTEX_COMPONENT, PositionVertexComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::SPRITE_TEXTURE_COMPONENT, SpriteTextureComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::MEM_SPRITE_DATA_COMPONENT, MemSpriteDataComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::COLOR_VERTEX_COMPONENT, ColorVertexComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::MAP_COORD_COMPONENT, MapCoordComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::INPUT_COMPONENT, InputComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::CIRCLE_COLLISION_COMPONENT, CircleCollisionComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::SEGMENT_COLLISION_COMPONENT, SegmentCollisionComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::RECTANGLE_COLLISION_COMPONENT, RectangleCollisionComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::GENERAL_COLLISION_COMPONENT, GeneralCollisionComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::MOVEABLE_COMPONENT, MoveableComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::TIMER_COMPONENT, TimerComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::PLAYER_CONF_COMPONENT, PlayerConfComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::ENEMY_CONF_COMPONENT, EnemyConfComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::MEM_POSITIONS_VERTEX_COMPONENT, MemPositionsVertexComponents>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::WRITE_COMPONENT, WriteComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::SHOT_CONF_COMPONENT, ShotConfComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::OBJECT_CONF_COMPONENT, ObjectConfComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::WEAPON_COMPONENT, WeaponComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::AUDIO_COMPONENT, AudioComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::CHECKPOINT_COMPONENT, CheckpointComponent>().reserve(size);
        ECS::ComponentsManager<N, C...>::template getVectTuple<Components_e::LOG_COMPONENT, LogComponent>().reserve(size);
    }

    template <uint32_t numComponent, Component_C CC>
    void treatNewComponent(std::array<VectUI_t, N> &vectEntity, const std::array<uint32_t, N> &vect)
    {
        assert(vect[numComponent] != 0);
        vectEntity[numComponent].resize(vect[numComponent]);
        std::vector<CC> &vectComponent = ECS::ComponentsManager<N, C...>::template getVectTuple<numComponent, CC>();
        for(uint32_t j = 0; j < vect[numComponent]; ++j)
        {
            if(!ECS::ComponentsManager<N, C...>::m_refDelComponents[numComponent].empty())
            {
                vectEntity[numComponent][j] = ECS::ComponentsManager<N, C...>::m_refDelComponents[numComponent].back();
                ECS::ComponentsManager<N, C...>::m_refDelComponents[numComponent].pop_back();
            }
            else
            {
                if(vectComponent.empty())
                {
                    vectComponent.reserve(500);
                }
                vectEntity[numComponent][j] = vectComponent.size();
                vectComponent.emplace_back(CC());
            }
        }
    }

};
