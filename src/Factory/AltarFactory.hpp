#ifndef SRC_FACTORY_ALTARFACTORY_HPP__
#define SRC_FACTORY_ALTARFACTORY_HPP__

#include <Components/Position.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <entt/entity/fwd.hpp>

namespace Game::Factory::Altar
{

//! @brief Tag of the candle flame particles lit on an altar per relic. Their colour shows the key progress: default once
//! the exitkey drops, blue once the cryptkey drops.
inline constexpr const char *kCandleFlameParticleTag = "graveyard.altar.particle.flame";
//! @brief Tag of the sparkle particles shown, along with the altar's active sprite, while the player holds its extra life.
inline constexpr const char *kExtraLifeParticleTag = "altar.particle.extralife";

//! @brief Spawn the one-shot sacrifice animation played on an altar.
//! @param reg
//! @param pos Position and size of the animation entity.
//! @param anim_type Sprite type to play.
void create_altar_sacrifice_anim( entt::registry &reg, Cmp::Position pos, Sys::SpriteKey anim_type );

//! @brief Get the active or inactive variant of an altar sprite.
//! @details e.g. "sprite.graveyard.altar2.inactive" -> "sprite.graveyard.altar2.active"
//! @param sprite_type An altar sprite type ending in ".active" or ".inactive".
//! @param active Whether to return the active variant.
//! @return The sprite type with its state suffix replaced, or `sprite_type` unchanged if it has no state suffix.
Sys::SpriteKey get_altar_sprite( const Sys::SpriteKey &sprite_type, bool active );

//! @brief Remove the extra life effect from every altar in the scene: revert the active sprites and destroy the extra
//! life particles. The candles are left as they are.
//! @param reg
void deactivate_altars( entt::registry &reg );

} // namespace Game::Factory::Altar

#endif // SRC_FACTORY_ALTARFACTORY_HPP__
