#include <Components/Altar/Sacrifice.hpp>
#include <Components/AnimData.hpp>
#include <Components/ZOrderValue.hpp>
#include <Factory/AltarFactory.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <entt/entity/registry.hpp>

namespace Game::Factory::Altar
{

void create_altar_sacrifice_anim( entt::registry &reg, Cmp::Position pos, Sys::SpriteKey anim_type )
{
  entt::entity entt = reg.create();
  reg.emplace_or_replace<Cmp::Altar::Sacrifice>( entt );
  reg.emplace_or_replace<Cmp::Position>( entt, pos.position, pos.size );
  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( entt, Cmp::AnimData::Config{
    .sprite_type        = anim_type,
    .framerate          = 0.2,
    .anim_type = Cmp::AnimType::ONESHOTRESET
  });
  // clang-format off

  reg.emplace_or_replace<Cmp::ZOrderValue>( entt, pos.position.y * 4.f );
}

} // namespace Game::Factory::Altar
