#ifndef SRC_UTILS_PLAYER_HPP__
#define SRC_UTILS_PLAYER_HPP__

#include <Components/AbsoluteAlpha.hpp>
#include <Components/AnimData.hpp>
#include <Components/Player/ExtraLife.hpp>
#include <Components/Player/FootstepType.hpp>
#include <Components/Stats/PlayerStats.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <Sprites/SpriteMetaType.hpp>
#include <source_location>

namespace Game::Cmp
{
class Position;
class RectBounds;
class PlayerHealth;
class ZOrderValue;
class Direction;
class LastDirection;
class PlayerInventorySlot;
class TorchRadius;
} // namespace Game::Cmp

namespace Game::Cmp::Player
{
class Wealth;
class BlastRadius;
class Mortality;
class LastGraveyardPosition;
class Curse;
class CadaverCount;
class LevelDepth;
} // namespace Game::Cmp::Player

namespace std
{
class source_location;
}

namespace Game::Utils::Player
{

//! @brief Get the player entity.
//! @param reg reference to the entt registry
//! @return entt::entity The player entity.
//! @throws std::runtime_error if no entity has both Cmp::Player::Character and Cmp::Position.
entt::entity get_entity( entt::registry &reg );

//! @brief Get the player's position object.
//! @param reg reference to the entt registry
//! @return Cmp::Position& Reference to the player's position.
//! @throws std::runtime_error if the player has no Cmp::Position.
Cmp::Position &get_position( entt::registry &reg );

//! @brief Get the player's direction object.
//! @param reg reference to the entt registry
//! @return Cmp::Direction& Reference to the player's direction.
//! @throws std::runtime_error if the player has no Cmp::Direction.
Cmp::Direction &get_direction( entt::registry &reg );

//! @brief Get the player's last-facing-direction object.
//! @param reg reference to the entt registry
//! @return Cmp::LastDirection& Reference to the player's last direction.
//! @throws std::runtime_error if the player has no Cmp::LastDirection.
Cmp::LastDirection &get_last_direction( entt::registry &reg );

//! @brief Get the player's sprite animation object.
//! @param reg reference to the entt registry
//! @return Cmp::AnimData& Reference to the player's animation data.
//! @throws std::runtime_error if the player has no Cmp::AnimData.
Cmp::AnimData &get_sprite_anim( entt::registry &reg );

//! @brief Get the player's current ruin floor location.
//! @note Returns the Cmp::Player::RuinLocation::Floor enum as an int to avoid including that header here.
//! @param reg reference to the entt registry
//! @return int The current floor, or Cmp::Player::RuinLocation::Floor::NONE (as int) if the player has no Cmp::Player::RuinLocation.
int get_ruin_location( entt::registry &reg ); // Returns enum as int to avoid include

//! @brief Get the player's last graveyard position object.
//! @param reg reference to the entt registry
//! @return Cmp::Player::LastGraveyardPosition* Pointer to the component, or nullptr if the player doesn't have one.
Cmp::Player::LastGraveyardPosition *get_last_graveyard_pos( entt::registry &reg );

//! @brief Get the player's level depth object.
//! @param reg reference to the entt registry
//! @return Cmp::Player::LevelDepth& Reference to the player's level depth.
//! @throws std::runtime_error if the player has no Cmp::Player::LevelDepth.
Cmp::Player::LevelDepth &get_level_depth( entt::registry &reg );

//! @brief Get the player's wealth object.
//! @param reg reference to the entt registry
//! @return Cmp::Player::Wealth& Reference to the player's wealth.
//! @throws std::runtime_error if the player has no Cmp::Player::Wealth.
Cmp::Player::Wealth &get_wealth( entt::registry &reg );

//! @brief Get the player's blast radius object.
//! @param reg reference to the entt registry
//! @return Cmp::Player::BlastRadius& Reference to the player's blast radius.
//! @throws std::runtime_error if the player has no Cmp::Player::BlastRadius.
Cmp::Player::BlastRadius &get_blast_radius( entt::registry &reg );

//! @brief Get the player's mortality object.
//! @param reg reference to the entt registry
//! @return Cmp::Player::Mortality& Reference to the player's mortality state.
//! @throws std::runtime_error if the player has no Cmp::Player::Mortality.
Cmp::Player::Mortality &get_mortality( entt::registry &reg );

//! @brief Get the player's z-order value.
//! @param reg reference to the entt registry
//! @return Cmp::ZOrderValue& Reference to the player's z-order value.
//! @throws std::runtime_error if the player has no Cmp::ZOrderValue.
Cmp::ZOrderValue &get_zorder( entt::registry &reg );

//! @brief Get the player's absolute alpha value.
//! @param reg reference to the entt registry
//! @return Cmp::AbsoluteAlpha& Reference to the player's absolute alpha.
//! @throws std::runtime_error if the player has no Cmp::AbsoluteAlpha.
Cmp::AbsoluteAlpha &get_alpha( entt::registry &reg );

//! @brief Get the player's curse object.
//! @param reg reference to the entt registry
//! @return Cmp::Player::Curse& Reference to the player's curse state.
//! @throws std::runtime_error if the player has no Cmp::Player::Curse.
Cmp::Player::Curse &get_curse( entt::registry &reg );

//! @brief Deactivate the player's curse and reset its shader alpha.
//! @param reg reference to the entt registry
//! @throws std::runtime_error if the player has no Cmp::Player::Curse.
void reset_curse( entt::registry &reg );

//! @brief Get the player's current movement speed multiplier from Cmp::Player::SpeedPenalty.
//! @param reg reference to the entt registry
//! @return float The speed penalty multiplier, or 1.f if the player has no Cmp::Player::SpeedPenalty.
float get_speed_penalty( entt::registry &reg );

//! @brief Remove the player's Cmp::LerpPosition component, if present, to prevent stale
//! position interpolation carrying over from a previous scene.
//! @param reg reference to the entt registry
void remove_lerp_cmp( entt::registry &reg );

//! @brief Get the entity and sprite type currently held in the player's (single-slot) inventory.
//! @param reg reference to the entt registry
//! @return std::tuple<entt::entity, std::string, Sprites::SpriteMetaType> The inventory slot entity, its item type and its sprite type;
//! entt::null and empty strings for the types if the inventory is empty.
std::tuple<entt::entity, std::string, Sprites::SpriteMetaType> get_inventory( entt::registry &reg );

//! @brief Get the wear level of the item in the player's inventory slot.
//! @param reg reference to the entt registry
//! @return float The wear level, or -1 if the inventory slot has no Cmp::Inventory::WearLevel.
float get_inventory_wear_level( entt::registry &reg );

//! @brief Reduce the wear level of the item in the player's inventory slot by `amount`.
//! @param reg reference to the entt registry
//! @param amount The amount to subtract from the current wear level.
void reduce_inventory_wear_level( entt::registry &reg, float amount );

//! @brief Check whether the given position is inside a spawn area.
//! @param reg reference to the entt registry
//! @param player_pos_cmp The position to test (scaled to 90% before testing).
//! @return bool true if `player_pos_cmp` intersects any Cmp::SpawnArea.
bool is_in_spawn( entt::registry &reg, const Cmp::Position &player_pos_cmp );

//! @brief True if the player's hitbox (scaled 1.5x the grid size) intersects the given bounds
//! @param reg reference to the entt registry
//! @param bounds The bounds to test against the player's hitbox.
//! @return bool true if the player is near `bounds`.
bool is_player_near( entt::registry &reg, const sf::FloatRect &bounds );

//! @brief The grid cell one tile in front of the player, based on their last facing direction
//! @param reg reference to the entt registry
//! @return Cmp::Position A 1x1 position one grid cell ahead of the player's current center.
Cmp::Position get_projected_position( entt::registry &reg );

//! @brief Get the player's cadaver count object.
//! @param reg reference to the entt registry
//! @return Cmp::Player::CadaverCount& Reference to the player's cadaver count.
//! @throws std::runtime_error if the player has no Cmp::Player::CadaverCount.
Cmp::Player::CadaverCount &get_cadaver_count( entt::registry &reg );

//! @brief Get the player's torch radius object.
//! @param reg reference to the entt registry
//! @return Cmp::TorchRadius& Reference to the player's torch radius.
//! @throws std::runtime_error if the player has no Cmp::TorchRadius.
Cmp::TorchRadius &get_torch_radius( entt::registry &reg );

//! @brief Get the player's stats object.
//! @param reg reference to the entt registry
//! @return Cmp::PlayerStats& Reference to the player's stats.
//! @throws std::runtime_error if no entity has both Cmp::Player::Character and Cmp::PlayerStats.
Cmp::PlayerStats &get_stats( entt::registry &reg );

Cmp::Player::Footstep &get_footstep_type( entt::registry &reg );

//! @brief Get the player's global bomb-flash clock, used to time the screen flash effect for bomb detonations.
//! @param reg reference to the entt registry
//! @return sf::Clock& Reference to the clock stored on the player's Cmp::Player::Character.
//! @throws std::runtime_error if the player has no Cmp::Player::Character.
sf::Clock &get_global_bomb_flash_clk( entt::registry &reg );

//! @brief Check whether the player currently has an extra life.
//! @param reg reference to the entt registry
//! @return bool true if any entity has a Cmp::Player::ExtraLife component.
bool player_has_extra_life( entt::registry &reg );

//! @brief Look up and apply the ActionT stat-modifier from a world item's action map.
//! @tparam ActionT The Cmp::BaseAction subclass to apply (e.g. Cmp::DestroyAction). Only the subclasses
//! in src/Components/Stats are explicitly instantiated in Player.cpp - using any other type is a link error.
//! @param reg reference to the entt registry
//! @param world_item_entt entity carrying the Cmp::WorldItem component to look the action up on
//! @throws std::runtime_error if world_item_entt has no Cmp::WorldItem.
//! @throws std::out_of_range if the world item has no ActionT registered.
template <typename ActionT>
void apply_action_from_world_item( entt::registry &reg, entt::entity world_item_entt,
                                   const std::source_location &loc = std::source_location::current() );

//! @brief Lookup up and apply the ActionT stat-modifier from the item in player's current inventory
//! @tparam ActionT  The Cmp::BaseAction subclass to apply (e.g. Cmp::DestroyAction). Only the subclasses
//! in src/Components/Stats are explicitly instantiated in Player.cpp - using any other type is a link error.
//! @param reg reference to the entt registry
//! @throws std::out_of_range if the player inventory item has no ActionT registered.
template <typename ActionT>
void apply_action_from_inventory_item( entt::registry &reg );

//! @brief Lookup up and apply the ActionT stat-modifier from the item store.
//! @tparam ActionT The Cmp::BaseAction subclass to apply (e.g. Cmp::DestroyAction). Only the subclasses
//! in src/Components/Stats are explicitly instantiated in Player.cpp - using any other type is a link error.
//! @param reg reference to the entt registry
//! @param item_type string identifier (see item.json)
//! @throws std::runtime_error if the key is not present in the store.
template <typename ActionT>
void apply_action_from_item_store( entt::registry &reg, const std::string &item_type );

//! @brief Look up the ActionT stat-modifier from the item store, without applying it.
//! @tparam ActionT The Cmp::BaseAction subclass to look up (e.g. Cmp::DestroyAction). Only the subclasses
//! in src/Components/Stats are explicitly instantiated in Player.cpp - using any other type is a link error.
//! @param item_type string identifier (see item.json)
//! @return Cmp::BaseAction The stat modifier, for the caller to accumulate/apply itself.
//! @throws std::runtime_error if the key is not present in the store.
//! @throws std::out_of_range if the item has no ActionT registered.
template <typename ActionT>
Cmp::BaseAction get_action_from_item_store( const std::string &item_type );

//! @brief Lookup up and apply the ActionT stat-modifier from the npc store.
//! @tparam ActionT The Cmp::BaseAction subclass to apply (e.g. Cmp::DestroyAction). Only the subclasses
//! in src/Components/Stats are explicitly instantiated in Player.cpp - using any other type is a link error.
//! @param reg reference to the entt registry
//! @param npc_type string identifier (see npc.json)
//! @throws std::runtime_error if the key is not present in the store.
template <typename ActionT>
void apply_action_from_npc_store( entt::registry &reg, const std::string &npc_type );

} // namespace Game::Utils::Player

#endif // SRC_UTILS_PLAYER_HPP__
