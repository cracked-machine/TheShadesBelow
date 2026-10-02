#include <Components/Altar/Sacrifice.hpp>
#include <Components/Altar/Segment.hpp>
#include <Components/AnimData.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Position.hpp>
#include <Components/ZOrderValue.hpp>
#include <Systems/AnimSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>

#include <SFML/System/Time.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace Game::Sys
{

void AnimSystem::update( sf::Time dt )
{

  auto anim_view = reg().view<Cmp::AnimData, Cmp::Position>( entt::exclude<Cmp::Npc::NPC> );
  for ( auto [anim_entt, anim_cmp, pos_cmp] : anim_view.each() )
  {
    if ( not Utils::is_visible_in_view( RenderSystem::get_world_view(), pos_cmp ) ) continue;
    // don't accumulate dt if the player is not moving - fixes initial rapid framerate bug
    if ( anim_cmp.m_sprite_type.contains( "sprite.player" ) and ( Utils::Player::get_direction( reg() ) == sf::Vector2f( 0, 0 ) ) ) continue;
    if ( anim_cmp.m_enabled )
    {
      const auto &ms = Sys::SpriteStore::instance().get( anim_cmp.m_sprite_type );
      update_sequence_frame( anim_cmp, dt, ms, sf::seconds( anim_cmp.get_framerate() ) );

      // disable oneshot animations at the end of their sequence
      if ( anim_cmp.m_anim_type == Cmp::AnimType::LOOP ) continue;
      if ( anim_cmp.m_current_frame == ms.sequence_size() - 1 )
      {
        SPDLOG_DEBUG( "Deactivating animation: {}", static_cast<int>( anim_entt ) );
        anim_cmp.m_enabled = false;
        if ( anim_cmp.m_anim_type == Cmp::AnimType::ONESHOTRESET ) { anim_cmp.m_current_frame = anim_cmp.m_base_frame; }
      }
    }
  }

  // NPC Movement: only update animation for NPC that are actively pathfinding
  auto pathfinding_npc_view = reg().view<Cmp::Npc::NPC, Cmp::Direction, Cmp::AnimData, Cmp::Position>();
  for ( auto entity : pathfinding_npc_view )
  {
    auto [direction_cmp, anim_cmp, pos_cmp] = pathfinding_npc_view.get<Cmp::Direction, Cmp::AnimData, Cmp::Position>( entity );
    if ( not Utils::is_visible_in_view( RenderSystem::get_world_view(), pos_cmp ) ) continue;
    if ( direction_cmp == sf::Vector2f( 0.f, 0.f ) ) continue;

    SPDLOG_DEBUG( "NPC {} framerate is {}", anim_cmp.m_sprite_type, anim_cmp.get_framerate() );
    const auto &npc_walk_sequence = Sys::SpriteStore::instance().get( anim_cmp.m_sprite_type );
    update_sequence_frame( anim_cmp, dt, npc_walk_sequence, sf::seconds( anim_cmp.get_framerate() ) );
  }
}

void AnimSystem::update_sequence_frame( Cmp::AnimData &anim, sf::Time globalDeltaTime, const Sprites::SpriteSheet &ms, sf::Time frame_rate )
{
  anim.m_elapsed_time += globalDeltaTime;

  if ( anim.m_elapsed_time >= frame_rate )
  {
    unsigned int num_animation_frames = ms.sequence_size() / ms.frame_size();
    if ( num_animation_frames < 1 )
    {
      throw std::runtime_error( "Not enough indices in Sprite '" + ms.type().str() + "': sprites per sequence / sprites per frame == 0" );
    }
    unsigned int current_anim_frame = anim.m_current_frame / ms.frame_size();
    unsigned int next_anim_frame = ( current_anim_frame + 1 ) % num_animation_frames;

    anim.m_current_frame = next_anim_frame * ms.frame_size();
    anim.m_elapsed_time -= frame_rate;
  }
}

} // namespace Game::Sys