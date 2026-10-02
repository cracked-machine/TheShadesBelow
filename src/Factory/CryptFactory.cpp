#include <Components/AnimData.hpp>
#include <Components/Crypt/Chest.hpp>
#include <Components/Crypt/InteriorMultiBlock.hpp>
#include <Components/Crypt/InteriorSegment.hpp>
#include <Components/Crypt/Lever.hpp>
#include <Components/Crypt/PassageBlock.hpp>
#include <Components/Crypt/PassageSpikeTrap.hpp>
#include <Components/Crypt/RoomEnd.hpp>
#include <Components/Crypt/RoomLavaPit.hpp>
#include <Components/Crypt/RoomLavaPitCell.hpp>
#include <Components/Crypt/RoomOpen.hpp>
#include <Components/Crypt/RoomStart.hpp>
#include <Components/Crypt/ShuffleTimer.hpp>
#include <Components/Exit.hpp>
#include <Components/FootStepAlpha.hpp>
#include <Components/FootStepTimer.hpp>
#include <Components/Hazard/CollisionResist.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Particle/BlockParticle.hpp>
#include <Components/Persistent/HazardPushbackResist.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <Components/ZOrderValue.hpp>
#include <Factory/CryptFactory.hpp>
#include <Factory/MultiblockFactory.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Player.hpp>
#include <Utils/Random.hpp>

#include <stdexcept>

namespace Game::Factory::Crypt
{

entt::entity create_crypt_exit( entt::registry &reg, sf::Vector2f spawn_pos_px )
{
  if ( not reg.view<Cmp::Exit>().empty() ) { throw std::runtime_error( "Crypt level data defines more than one exit tile" ); }

  auto entity = reg.create();
  reg.emplace_or_replace<Cmp::Position>( entity, spawn_pos_px, Constants::kGridSizePxF );
  reg.emplace_or_replace<Cmp::AnimData>( entity, Cmp::AnimData::Config{ .sprite_type = "sprite.crypt.exit" } );
  reg.emplace_or_replace<Cmp::ZOrderValue>( entity, spawn_pos_px.y );
  reg.emplace_or_replace<Cmp::Npc::NoPathFinding>( entity );
  reg.emplace_or_replace<Cmp::Exit>( entity );
  return entity;
}

entt::entity create_crypt_lever( entt::registry &reg, sf::Vector2f pos, Sys::SpriteKey sprite_type, unsigned int sprite_idx, float zorder )
{
  auto entt = reg.create();
  reg.emplace_or_replace<Cmp::Position>( entt, pos, Constants::kGridSizePxF );
  reg.emplace_or_replace<Cmp::Crypt::Lever>( entt );
  reg.emplace_or_replace<Cmp::AnimData>( entt, Cmp::AnimData::Config{ .sprite_type = sprite_type, .frame_index_offset = sprite_idx } );
  reg.emplace_or_replace<Cmp::ZOrderValue>( entt, zorder );
  return entt;
}

void destroy_crypt_lever( entt::registry &reg, entt::entity entt )
{
  if ( reg.all_of<Cmp::Crypt::Lever>( entt ) ) reg.remove<Cmp::Crypt::Lever>( entt );
  if ( reg.all_of<Cmp::Position>( entt ) ) reg.remove<Cmp::Position>( entt );
  if ( reg.all_of<Cmp::AnimData>( entt ) ) reg.remove<Cmp::AnimData>( entt );
  if ( reg.all_of<Cmp::ZOrderValue>( entt ) ) reg.remove<Cmp::ZOrderValue>( entt );
  if ( reg.valid( entt ) ) { reg.destroy( entt ); }
}

entt::entity create_crypt_chest( entt::registry &reg, sf::Vector2f pos, Sys::SpriteKey sprite_type, unsigned int sprite_idx, float zorder )
{
  auto entt = reg.create();
  reg.emplace_or_replace<Cmp::Position>( entt, pos, Constants::kGridSizePxF );
  reg.emplace_or_replace<Cmp::Crypt::Chest>( entt );
  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( entt, Cmp::AnimData::Config{ 
        .sprite_type = sprite_type, 
        .frame_index_offset = sprite_idx,
        .framerate = 0.4, 
        .enabled = false, 
        .anim_type = Cmp::AnimType::ONESHOTHOLD 
  });
  // clang-format on
  reg.emplace_or_replace<Cmp::ZOrderValue>( entt, zorder );
  reg.emplace_or_replace<Cmp::Player::NoPath>( entt );
  reg.emplace_or_replace<Cmp::Particle::BlockParticle>( entt );
  return entt;
}

void destroy_crypt_chest( entt::registry &reg, entt::entity entt )
{
  if ( not reg.valid( entt ) ) return;
  if ( reg.all_of<Cmp::Crypt::Chest>( entt ) ) reg.remove<Cmp::Crypt::Lever>( entt );
  if ( reg.all_of<Cmp::Position>( entt ) ) reg.remove<Cmp::Position>( entt );
  if ( reg.all_of<Cmp::AnimData>( entt ) ) reg.remove<Cmp::AnimData>( entt );
  if ( reg.all_of<Cmp::ZOrderValue>( entt ) ) reg.remove<Cmp::ZOrderValue>( entt );
  if ( reg.all_of<Cmp::Player::NoPath>( entt ) ) { reg.remove<Cmp::Player::NoPath>( entt ); }
  if ( reg.all_of<Cmp::Particle::BlockParticle>( entt ) ) { reg.remove<Cmp::Particle::BlockParticle>( entt ); }
  if ( reg.valid( entt ) ) { reg.destroy( entt ); }
}

void create_crypt_lava_pit( entt::registry &reg, const Cmp::Crypt::RoomOpen &room,
                            std::shared_ptr<Game::PathFinding::SpatialHashGrid> pathfinding_navmesh )
{
  // add the lava pit area
  sf::Vector2f adjusted_position = { room.position.x + Constants::kGridSizePxF.x, room.position.y + Constants::kGridSizePxF.y };
  sf::Vector2f adjusted_size = { room.size.x - ( Constants::kGridSizePxF.x * 2 ), room.size.y - ( Constants::kGridSizePxF.y * 2 ) };
  sf::FloatRect lava_pit_bounds( adjusted_position, adjusted_size );

  auto lava_pit_entt = reg.create();
  reg.emplace<Cmp::Crypt::RoomLavaPit>( lava_pit_entt, lava_pit_bounds );
  reg.emplace_or_replace<Cmp::Npc::NoPathFinding>( lava_pit_entt );

  const auto player_pos = Utils::Player::get_position( reg );

  auto pushback_resist_seconds = Sys::PersistSystem::get<Cmp::Persist::HazardPushbackResist>( reg ).get_value();

  // add the inidividual lava cells
  for ( auto [pos_entt, pos_cmp] : reg.view<Cmp::Position>().each() )
  {
    if ( reg.all_of<Cmp::FootStepTimer>( pos_entt ) ) continue; // dont add lava to footstep positions
    if ( player_pos.findIntersection( pos_cmp ) ) continue;     // dont add lava cell to player pos!

    if ( not lava_pit_bounds.findIntersection( pos_cmp ) ) continue; // only add lava to this lava pit
    auto lava_cell_entt = reg.create();
    reg.emplace_or_replace<Cmp::Position>( lava_cell_entt, pos_cmp.position, pos_cmp.size );
    reg.emplace_or_replace<Cmp::Npc::NoPathFinding>( lava_cell_entt );
    reg.emplace_or_replace<Cmp::Crypt::RoomLavaPitCell>( lava_cell_entt, pos_cmp.position, pos_cmp.size );
    reg.emplace_or_replace<Cmp::Hazard::CollisionResist>( lava_cell_entt, pushback_resist_seconds );
    // clang-format off
    reg.emplace_or_replace<Cmp::AnimData>( lava_cell_entt, Cmp::AnimData::Config{ 
          .sprite_type = "sprite.crypt.lava", 
          .enabled = true, 
    });
    // clang-format on
    if ( pathfinding_navmesh ) { pathfinding_navmesh->remove( pos_entt, pos_cmp ); }
    // reg.emplace<Cmp::ZOrderValue>( lava_cell_entt, pos_cmp.size.y );
  }
}

void destroy_crypt_lava_pit( entt::registry &reg, entt::entity entt, std::shared_ptr<Game::PathFinding::SpatialHashGrid> pathfinding_navmesh )
{
  auto *lava_pit_cmp = reg.try_get<Cmp::Crypt::RoomLavaPit>( entt );
  if ( lava_pit_cmp )
  {
    for ( auto [lava_cell_entt, lava_cell_cmp] : reg.view<Cmp::Crypt::RoomLavaPitCell>().each() )
    {
      if ( not lava_cell_cmp.findIntersection( *lava_pit_cmp ) ) continue;
      if ( pathfinding_navmesh ) { pathfinding_navmesh->remove( lava_cell_entt, lava_cell_cmp ); }
      reg.destroy( lava_cell_entt );
    }
  }
  reg.destroy( entt );
}

void add_spike_trap( entt::registry &reg, const entt::entity entt, const int passage_id )
{
  auto *pblock_cmp = reg.try_get<Cmp::Crypt::PassageBlock>( entt );
  if ( not pblock_cmp ) return;
  sf::Vector2f position = *pblock_cmp;

  auto spike_entt = reg.create();
  reg.emplace_or_replace<Cmp::Position>( spike_entt, position, Constants::kGridSizePxF );
  // reg.emplace_or_replace<Cmp::SpriteAnimation>( spike_entt, 0, 0, false, "sprite.crypt.spikes", 0, 0.2, Cmp::AnimType::ONESHOTRESET );
  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( spike_entt, Cmp::AnimData::Config{ 
        .sprite_type = "sprite.crypt.spikes", 
        .framerate = 0.2,
        .enabled = false,
        .anim_type =   Cmp::AnimType::ONESHOTRESET
  });
  // clang-format on
  reg.emplace_or_replace<Cmp::ZOrderValue>( spike_entt, position.y - 16.f ); // always behind player
  reg.emplace_or_replace<Cmp::Crypt::PassageSpikeTrap>( spike_entt, position, passage_id );
}

void create_crypt_shuffle_timer( entt::registry &reg, float threshold )
{
  auto entt = reg.create();
  reg.emplace_or_replace<Cmp::Crypt::ShuffleTimer>( entt, sf::seconds( threshold ) );
}

void destroy_crypt_shuffle_timer( entt::registry &reg )
{
  auto timer_view = reg.view<Cmp::Crypt::ShuffleTimer>();
  if ( not timer_view.empty() ) reg.destroy( timer_view.front() );
}

} // namespace Game::Factory::Crypt