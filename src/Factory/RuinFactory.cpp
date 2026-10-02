#include <Components/AbsoluteAlpha.hpp>
#include <Components/AnimData.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/ShadowHand.hpp>
#include <Components/Obstacle.hpp>
#include <Components/Particle/BlockParticle.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Ruin/Bookcase.hpp>
#include <Components/Ruin/Cobweb.hpp>
#include <Components/Ruin/RuneMarking.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/RuinFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Utils/Collision.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Random.hpp>

#include <entt/entity/registry.hpp>

namespace Game::Factory::Ruin
{

void create_bookcase( entt::registry &reg, sf::Vector2f spawn_position, const Sprites::SpriteSheet &bookcase_ms, size_t sprite_idx )
{
  // We must modify the **existing** Cmp::Position-owning entity so that we don't have
  // new entity with Cmp::Npc::NoPathFinding and existing entity without. This screws up path finding
  Cmp::Position search_pos( spawn_position, bookcase_ms.sprite_size() );
  for ( auto [existing_entt, existing_pos_cmp] : reg.view<Cmp::Position>().each() )
  {
    if ( search_pos.findIntersection( existing_pos_cmp ) )
    {
      reg.emplace_or_replace<Cmp::Position>( existing_entt, spawn_position, bookcase_ms.sprite_size() );
      // clang-format off
      reg.emplace_or_replace<Cmp::AnimData>( existing_entt, Cmp::AnimData::Config{ 
            .sprite_type = bookcase_ms.type(), 
            .frame_index_offset = sprite_idx,
            .enabled = true
      });
      // clang-format on
      reg.emplace_or_replace<Cmp::ZOrderValue>( existing_entt, -5.f );
      reg.emplace_or_replace<Cmp::Ruin::Bookcase>( existing_entt );
      reg.emplace_or_replace<Cmp::Npc::NoPathFinding>( existing_entt );
      reg.emplace_or_replace<Cmp::Player::NoPath>( existing_entt );
      reg.emplace_or_replace<Cmp::Particle::BlockParticle>( existing_entt );
      break;
    }
  }
}

void create_cobweb( entt::registry &reg, entt::entity selected_entt, sf::Vector2f spawn_position, const Sprites::SpriteSheet &cobweb_ms,
                    size_t sprite_idx, PathFinding::SpatialHashGrid &reserved_sm )
{
  // auto cobweb_entt = reg.create();
  Cmp::Position new_pos( spawn_position, cobweb_ms.sprite_size() );
  reg.emplace_or_replace<Cmp::Position>( selected_entt, new_pos );
  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( selected_entt, Cmp::AnimData::Config{
        .sprite_type = cobweb_ms.type(),
        .frame_index_offset = sprite_idx,
        .enabled = true
  });
  // clang-format on
  reserved_sm.insert( selected_entt, new_pos );
  reg.emplace_or_replace<Cmp::ZOrderValue>( selected_entt, spawn_position.y );
  reg.emplace_or_replace<Cmp::Ruin::Cobweb>( selected_entt, 100 );
}

void create_shadow_hand( entt::registry &reg, sf::Vector2f scene_dimensions, const Sprites::SpriteSheet &hand_ms, size_t sprite_idx )
{
  bool exists = not reg.view<Cmp::Npc::ShadowHand>().empty();

  if ( not exists )
  {
    auto npc_shadowhand_cmp = Sys::NpcStore::instance().get( "npc.shadowhand" );
    const auto hand_ms_size = hand_ms.sprite_size();
    sf::Vector2f starting_pos = { 0 - hand_ms_size.x, ( scene_dimensions.y / 2 ) - ( hand_ms_size.y / 2 ) };

    auto shadowhand_entt = reg.create();
    reg.emplace_or_replace<Cmp::Position>( shadowhand_entt, starting_pos, hand_ms.sprite_size() );
    // clang-format off
    reg.emplace_or_replace<Cmp::AnimData>( shadowhand_entt, Cmp::AnimData::Config{ 
          .sprite_type = hand_ms.type(), 
          .frame_index_offset = sprite_idx,
          .enabled = true
    });
    // clang-format on
    reg.emplace_or_replace<Cmp::ZOrderValue>( shadowhand_entt, hand_ms.zorder( 0 ) ); // above everythign
    reg.emplace_or_replace<Cmp::AbsoluteAlpha>( shadowhand_entt, 200 );
    reg.emplace_or_replace<Cmp::Npc::NPC>( shadowhand_entt, npc_shadowhand_cmp );
    reg.emplace_or_replace<Cmp::Npc::ShadowHand>( shadowhand_entt );
  }
}

entt::entity create_rune_marker( entt::registry &reg, Cmp::Position pos, float zorder, size_t sprite_idx, PathFinding::SpatialHashGrid &reserved_sm )
{
  auto rune_entt = reg.create();
  reg.emplace_or_replace<Cmp::Position>( rune_entt, pos );
  reserved_sm.insert( rune_entt, pos );
  reg.emplace_or_replace<Cmp::UUID>( rune_entt, Cmp::UUID::generate() );

  reg.emplace_or_replace<Cmp::ZOrderValue>( rune_entt, zorder );
  reg.emplace_or_replace<Cmp::RuneMarking>( rune_entt );
  // clang-format off
    reg.emplace_or_replace<Cmp::AnimData>( rune_entt, Cmp::AnimData::Config{
          .sprite_type = "sprite.ruin.runemarking.inactive",
          .frame_index_offset = sprite_idx,
          .enabled = true
    });
  // clang-format on
  return rune_entt;
}

} // namespace Game::Factory::Ruin