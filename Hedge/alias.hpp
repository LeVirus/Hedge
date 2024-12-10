#pragma once

#include <ECS_Headers/ECSManager.hpp>
#include <ECS_Headers/ComponentsManager.hpp>
#include <ComponentManagerExtend.hpp>

#include <ECS/Components/MemPositionsVertexComponents.hpp>
#include <ECS/Components/ColorVertexComponent.hpp>
#include <ECS/Components/PositionVertexComponent.hpp>
#include <ECS/Components/AudioComponent.hpp>
#include <ECS/Components/CheckpointComponent.hpp>
#include <ECS/Components/EnemyConfComponent.hpp>
#include <ECS/Components/ShotConfComponent.hpp>
#include <ECS/Components/SpriteTextureComponent.hpp>
#include <ECS/Components/MemSpriteDataComponent.hpp>
#include <ECS/Components/MapCoordComponent.hpp>
#include <ECS/Components/InputComponent.hpp>
#include <ECS/Components/CircleCollisionComponent.hpp>
#include <ECS/Components/SegmentCollisionComponent.hpp>
#include <ECS/Components/RectangleCollisionComponent.hpp>
#include <ECS/Components/GeneralCollisionComponent.hpp>
#include <ECS/Components/MoveableComponent.hpp>
#include <ECS/Components/TimerComponent.hpp>
#include <ECS/Components/PlayerConfComponent.hpp>
#include <ECS/Components/WriteComponent.hpp>
#include <ECS/Components/ObjectConfComponent.hpp>
#include <ECS/Components/WeaponComponent.hpp>
#include <ECS/Components/LogComponent.hpp>

#include <tuple>
#include <ECS_Headers/SystemManager.hpp>



using TupleComp_t = std::tuple<PositionVertexComponent, SpriteTextureComponent, MemSpriteDataComponent, ColorVertexComponent,
                               MapCoordComponent, InputComponent, CircleCollisionComponent, SegmentCollisionComponent,
                               RectangleCollisionComponent, GeneralCollisionComponent, MoveableComponent, TimerComponent, PlayerConfComponent,
                               EnemyConfComponent, MemPositionsVertexComponents, WriteComponent, ShotConfComponent,
                               ObjectConfComponent, WeaponComponent, AudioComponent, CheckpointComponent, LogComponent>;

using Ecsm_t = ECS::ECSManager<22u, PositionVertexComponent, SpriteTextureComponent, MemSpriteDataComponent, ColorVertexComponent,
                               MapCoordComponent, InputComponent, CircleCollisionComponent, SegmentCollisionComponent,
                               RectangleCollisionComponent, GeneralCollisionComponent, MoveableComponent, TimerComponent, PlayerConfComponent,
                               EnemyConfComponent, MemPositionsVertexComponents, WriteComponent, ShotConfComponent,
                               ObjectConfComponent, WeaponComponent, AudioComponent, CheckpointComponent, LogComponent>;


using EcsCompManager_t = ComponentManagerExtend<22u, PositionVertexComponent, SpriteTextureComponent, MemSpriteDataComponent, ColorVertexComponent,
                                                MapCoordComponent, InputComponent, CircleCollisionComponent, SegmentCollisionComponent,
                                                RectangleCollisionComponent, GeneralCollisionComponent, MoveableComponent, TimerComponent, PlayerConfComponent,
                                                EnemyConfComponent, MemPositionsVertexComponents, WriteComponent, ShotConfComponent,
                                                ObjectConfComponent, WeaponComponent, AudioComponent, CheckpointComponent, LogComponent>;


