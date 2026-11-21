#ifndef SRC_FACTORY_ALTARFACTORY_HPP__
#define SRC_FACTORY_ALTARFACTORY_HPP__

#include <Components/Position.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <entt/entity/fwd.hpp>

namespace Game::Factory::Altar
{

//! @brief Tag of the centre column particles shown, along with the altar's active sprite, while a graveyard altar is
//! activated (Cmp::Altar::Activated) and its key grave is yet to be exhumed.
inline constexpr const char *kActivatedParticleTag = "graveyard.altar.particle.activated";
//! @brief Tag of the sparkle particles shown, along with the altar's active sprite, while the player holds its extra life.
inline constexpr const char *kExtraLifeParticleTag = "altar.particle.extralife";

//! @brief Spawn the one-shot offering animation played on an altar.
//! @param reg
//! @param pos Position and size of the animation entity.
//! @param anim_type Sprite type to play.
void create_altar_offering_anim( entt::registry &reg, Cmp::Position pos, Sys::SpriteKey anim_type );

//! @brief Get the active or inactive variant of an altar sprite.
//! @details e.g. "sprite.graveyard.altar.inactive" -> "sprite.graveyard.altar.active"
//! @param sprite_type An altar sprite type ending in ".active" or ".inactive".
//! @param active Whether to return the active variant.
//! @return The sprite type with its state suffix replaced, or `sprite_type` unchanged if it has no state suffix.
Sys::SpriteKey get_altar_sprite( const Sys::SpriteKey &sprite_type, bool active );

//! @brief Remove the extra life effect from every altar in the scene: revert the active sprites and destroy the extra
//! life particles. The candles are left as they are, and so is an altar activated for a key grave
//! (Cmp::Altar::Activated), which keeps its active sprite until the key grave is exhumed.
//! @param reg
void deactivate_altars( entt::registry &reg );

} // namespace Game::Factory::Altar

#endif // SRC_FACTORY_ALTARFACTORY_HPP__
