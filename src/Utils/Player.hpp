#ifndef SRC_UTILS_PLAYER_HPP__
#define SRC_UTILS_PLAYER_HPP__

#include <Components/AbsoluteAlpha.hpp>
#include <Components/AnimData.hpp>
#include <Components/Player/ConsumeTimer.hpp>
#include <Components/Player/Doglegs.hpp>
#include <Components/Player/ExtraLife.hpp>
#include <Components/Player/FootstepType.hpp>
#include <Components/Stats/PlayerStats.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <optional>
#include <source_location>

namespace Game::Cmp
{
class BaseAction;
class Position;
class RectBounds;
class PlayerHealth;
class ZOrderValue;
class Direction;
class LastDirection;
struct PlayerInventorySlot;
class TorchRadius;
} // namespace Game::Cmp

namespace Game::Cmp::Player
{
class Wealth;
class Mortality;
class LastGraveyardPosition;
class Curse;
class Cadaver;
class LevelDepth;
class HeartBeat;
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
//! @return const Cmp::Position& Read-only reference to the player's position. To move the player, trigger
//! Events::UpdatePlayerPositionEvent: PlayerSystem is the only writer, so the navmeshes stay in sync.
//! @throws std::runtime_error if the player has no Cmp::Position.
const Cmp::Position &get_position( entt::registry &reg );

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

//! @brief Whether the player's current level is the last of the run (Cmp::Persist::MaxLevelDepth or deeper).
//! @param reg
//! @return true if completing this level ends the run
bool is_final_level_depth( entt::registry &reg );

//! @brief Get the player's wealth object.
//! @param reg reference to the entt registry
//! @return Cmp::Player::Wealth& Reference to the player's wealth.
//! @throws std::runtime_error if the player has no Cmp::Player::Wealth.
Cmp::Player::Wealth &get_wealth( entt::registry &reg );

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

//! @brief Get the player's base movement speed, adjusted for tachycardia/bradycardia toxicity.
//! Linearly interpolates from Cmp::Persist::PlayerMovementSpeed (no toxicity) towards
//! Cmp::Persist::TachycardiaMovementSpeed or Cmp::Persist::BradycardiaMovementSpeed (toxicity 100).
//! Does not include Utils::Player::get_speed_penalty.
//! @param reg reference to the entt registry
//! @return float The movement speed in pixels/second.
float get_movement_speed( entt::registry &reg );

//! @brief Get the player's current movement speed multiplier from Cmp::Player::SpeedPenalty and any burrs on the ground
//! under the player (see Utils::get_burr_speed_multiplier).
//! @param reg reference to the entt registry
//! @return float The speed penalty multiplier, or 1.f if the player is neither penalised nor walking over burrs.
float get_speed_penalty( entt::registry &reg );

//! @brief Remove the player's Cmp::LerpPosition component, if present, to prevent stale
//! position interpolation carrying over from a previous scene.
//! @param reg reference to the entt registry
void remove_lerp_cmp( entt::registry &reg );

//! @brief Get the number of player inventory slots (Cmp::Persist::InventorySlotCount, clamped to [1, Cmp::kMaxInventorySlots]).
//! @param reg reference to the entt registry
//! @return uint8_t The slot count.
uint8_t get_inventory_slot_count( entt::registry &reg );

//! @brief Get the index of the selected ("in hand") inventory slot, clamped to the current slot count.
//! @param reg reference to the entt registry
//! @return uint8_t The selected slot index, 0 if the player has no Cmp::Player::SelectedInventorySlot.
uint8_t get_selected_slot_idx( entt::registry &reg );

//! @brief Get the inventory slot entity at the given index, whether it is empty or not.
//! @param reg reference to the entt registry
//! @param idx The slot index.
//! @return entt::entity The slot entity, or entt::null if there is no slot with that index.
entt::entity get_inventory_slot_entt( entt::registry &reg, uint8_t idx );

//! @brief Get the index of the first empty inventory slot, in slot order. A slot whose entity doesn't exist yet counts
//! as empty (see Factory::Player::ensure_inventory_slots()).
//! @param reg The registry.
//! @return std::optional<uint8_t> The slot index, or std::nullopt if every slot holds an item.
std::optional<uint8_t> get_first_empty_slot_idx( entt::registry &reg );

//! @brief Get how many items an inventory slot or world item holds (Cmp::Inventory::StackCount).
//! @param reg The registry.
//! @param entt The slot or world item entity.
//! @return int The stack count, 1 if the entity has no Cmp::Inventory::StackCount.
int get_stack_count( entt::registry &reg, entt::entity entt );

//! @brief Check whether a world item can be added to an inventory slot's stack: both hold the same stackable item
//! (items.json "stackable"), the slot is below Cmp::Persist::InventoryStackSize, and neither carries per-instance item
//! state (wear, a particle bound to its UUID, dowsing target, summoning offering, curse tablet) that a stack would lose.
//! Plants never stack. A merged item's own UUID is dropped; the slot keeps its own.
//! @param reg The registry.
//! @param slot_entt The inventory slot entity.
//! @param world_item_entt The world item entity.
//! @return bool true if at least one item of the world item can join the slot's stack.
bool can_stack( entt::registry &reg, entt::entity slot_entt, entt::entity world_item_entt );

//! @brief Find an inventory slot whose stack the world item can join (see can_stack()), the selected slot first, then
//! in slot order.
//! @param reg The registry.
//! @param world_item_entt The world item entity.
//! @return entt::entity The slot entity, or entt::null if no slot can take it.
entt::entity find_stackable_slot( entt::registry &reg, entt::entity world_item_entt );

//! @brief Get the selected inventory slot entity, whether it is empty or not.
//! @param reg reference to the entt registry
//! @return entt::entity The slot entity, or entt::null if there is no slot with the selected index.
entt::entity get_selected_slot_entt( entt::registry &reg );

//! @brief Get the entity and types of the item held in the player's selected ("in hand") inventory slot.
//! @param reg reference to the entt registry
//! @return std::tuple<entt::entity, Sys::ItemKey, Sys::SpriteKey> The selected slot entity, its item type and its sprite type;
//! entt::null and empty strings for the types if the selected slot is empty.
std::tuple<entt::entity, Sys::ItemKey, Sys::SpriteKey> get_inventory( entt::registry &reg );

//! @brief Get the selected inventory slot entity, if it holds an item.
//! @param reg reference to the entt registry
//! @return entt::entity The selected slot entity, or entt::null if the selected slot is empty.
entt::entity get_inventory_entt( entt::registry &reg );

//! @brief Get the wear level of the item in the player's selected inventory slot.
//! @param reg reference to the entt registry
//! @return float The wear level, or -1 if the selected slot is empty or has no Cmp::Inventory::WearLevel.
float get_inventory_wear_level( entt::registry &reg );

//! @brief Check whether the given inventory slot holds a torch that is lit: either it is the selected slot, or
//! Cmp::Persist::StowedTorchLit is enabled.
//! @param reg reference to the entt registry
//! @param slot_entt The inventory slot entity.
//! @return bool true if the slot holds a lit torch.
bool is_torch_slot_lit( entt::registry &reg, entt::entity slot_entt );

//! @brief Check whether any inventory slot holds a lit torch, see is_torch_slot_lit().
//! @param reg reference to the entt registry
//! @return bool true if the player is carrying a lit torch.
bool has_lit_torch( entt::registry &reg );

//! @brief Check whether the given position is inside a spawn area.
//! @param reg reference to the entt registry
//! @param player_pos_cmp The position to test (scaled to 90% before testing).
//! @return bool true if `player_pos_cmp` intersects any Cmp::SpawnArea.
bool is_in_spawn( entt::registry &reg, const Cmp::Position &player_pos_cmp );

//! @brief Check whether the ElderFlute is currently playing.
//! @param reg reference to the entt registry
//! @return bool true if the player is in the graveyard with an ElderFlute in the inventory slot that has not worn out.
bool is_elderflute_active( entt::registry &reg );

//! @brief Check whether the player is currently flying on the witchesbroom.
//! @param reg reference to the entt registry
//! @return bool true if the player has the Cmp::Player::Flying tag.
bool is_flying( entt::registry &reg );

//! @brief Check whether the player is descending from flight to their landing cell.
//! @param reg reference to the entt registry
//! @return bool true if the player has Cmp::Player::Flying with landing set.
bool is_landing( entt::registry &reg );

//! @brief Check whether the flying player's sprite has risen above 1x1 obstacles, so is drawn in front of them.
//! @param reg reference to the entt registry
//! @return bool true if the player has Cmp::Player::Flying with elevated set.
bool is_flying_elevated( entt::registry &reg );

//! @brief Check whether the item in the inventory slot is actively in use. Used for items whose wear is "active_only".
//! @param reg reference to the entt registry
//! @return bool true if the ElderFlute is playing or the witchesbroom is flying.
bool is_inventory_item_active( entt::registry &reg );

bool is_illuminated( entt::registry &reg );

//! @brief True if the player's hitbox (scaled `player_hitbox_scale` times the grid size) intersects the given bounds
//! @param reg reference to the entt registry
//! @param bounds The bounds to test against the player's hitbox.
//! @return bool true if the player is near `bounds`.
bool is_player_near( entt::registry &reg, const sf::FloatRect &bounds, float player_hitbox_scale = 1.5f );

//! @brief True if the player's last dig action is still within its cooldown window (Cmp::Persist::DiggingCooldownThreshold)
//! @param reg reference to the entt registry
//! @return bool true if the player must wait before digging again.
bool is_digging_on_cooldown( entt::registry &reg );

//! @brief The grid cell one tile in front of the player, based on their last facing direction
//! @param reg reference to the entt registry
//! @return Cmp::Position A 1x1 position one grid cell ahead of the player's current center.
Cmp::Position get_projected_position( entt::registry &reg );

//! @brief Get the player's cadaver count object.
//! @param reg reference to the entt registry
//! @return Cmp::Player::CadaverCount& Reference to the player's cadaver count.
//! @throws std::runtime_error if the player has no Cmp::Player::CadaverCount.
Cmp::Player::Cadaver &get_cadaver( entt::registry &reg );

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

//! @brief Build the player's starting stats from the Cmp::Persist::Starting* settings (no toxidrome affliction).
//! @param reg reference to the entt registry
//! @return Cmp::PlayerStats New stats object holding the starting values.
Cmp::PlayerStats make_starting_stats( entt::registry &reg );

//! @brief Reset the player's health, fear, despair, infamy, luck and anima to the Cmp::Persist::Starting* settings.
//! The toxidrome is left untouched.
//! @param reg reference to the entt registry
//! @param keep_anima true to leave the player's anima as it is (see Cmp::Persist::RetainAnimaBetweenLevels)
void reset_stats_to_starting( entt::registry &reg, bool keep_anima );

Cmp::Player::Footstep &get_footstep_type( entt::registry &reg );

//! @brief Get the player's global bomb-flash timer, used to time the screen flash effect for bomb detonations.
//! @param reg reference to the entt registry
//! @return sf::Time& Reference to the clock stored on the player's Cmp::Player::Character.
//! @throws std::runtime_error if the player has no Cmp::Player::Character.
sf::Time &get_global_bomb_flash_timer( entt::registry &reg );

//! @brief Check whether the player currently has an extra life.
//! @param reg reference to the entt registry
//! @return bool true if any entity has a Cmp::Player::ExtraLife component.
bool player_has_extra_life( entt::registry &reg );

//! @brief Look up and apply the ActionT stat-modifier from a world item's action map.
//! @tparam ActionT The Cmp::BaseAction subclass to apply (e.g. Cmp::DestroyAction). Only the subclasses
//! in src/Components/Stats are explicitly instantiated in Player.cpp - using any other type is a link error.
//! @param reg reference to the entt registry
//! @param world_item_entt entity carrying the Cmp::WorldItem component to look the action up on
//! @return Cmp::BaseAction The applied action, so the calling system can trigger its side effects (see Cmp::SideEffects).
//! @throws std::runtime_error if world_item_entt has no Cmp::WorldItem.
//! @throws std::out_of_range if the world item has no ActionT registered.
template <typename ActionT>
Cmp::BaseAction apply_action_from_world_item( entt::registry &reg, entt::entity world_item_entt,
                                              const std::source_location &loc = std::source_location::current() );

//! @brief Lookup up and apply the ActionT stat-modifier from the item in player's selected inventory slot
//! @tparam ActionT  The Cmp::BaseAction subclass to apply (e.g. Cmp::DestroyAction). Only the subclasses
//! in src/Components/Stats are explicitly instantiated in Player.cpp - using any other type is a link error.
//! @param reg reference to the entt registry
//! @return Cmp::BaseAction The applied action (a zero action if there is no inventory), so the calling system can trigger its
//! side effects (see Cmp::SideEffects).
//! @throws std::out_of_range if the player inventory item has no ActionT registered.
template <typename ActionT>
Cmp::BaseAction apply_action_from_inventory_item( entt::registry &reg );

//! @brief Lookup up and apply the ActionT stat-modifier from the item store.
//! @tparam ActionT The Cmp::BaseAction subclass to apply (e.g. Cmp::DestroyAction). Only the subclasses
//! in src/Components/Stats are explicitly instantiated in Player.cpp - using any other type is a link error.
//! @param reg reference to the entt registry
//! @param item_type string identifier (see item.json)
//! @return Cmp::BaseAction The applied action, so the calling system can trigger its side effects (see Cmp::SideEffects).
//! @throws std::runtime_error if the key is not present in the store.
template <typename ActionT>
Cmp::BaseAction apply_action_from_item_store( entt::registry &reg, const Sys::ItemKey &item_type );

//! @brief Look up the ActionT stat-modifier from the item store, without applying it.
//! @tparam ActionT The Cmp::BaseAction subclass to look up (e.g. Cmp::DestroyAction). Only the subclasses
//! in src/Components/Stats are explicitly instantiated in Player.cpp - using any other type is a link error.
//! @param item_type string identifier (see item.json)
//! @return Cmp::BaseAction The stat modifier, for the caller to accumulate/apply itself.
//! @throws std::runtime_error if the key is not present in the store.
//! @throws std::out_of_range if the item has no ActionT registered.
template <typename ActionT>
Cmp::BaseAction get_action_from_item_store( const Sys::ItemKey &item_type );

//! @brief Get the heartbeat object
//! @param reg
//! @return Cmp::Player::HeartBeat&
Cmp::Player::HeartBeat &get_heartbeat( entt::registry &reg );

//! @brief Check player entity for Cmp::Player::EatingTimeAccumulator component
//! @param reg
//! @return Cmp::Player::EatingTimeAccumulator* modifiable component, or nullptr if the player is not eating
Cmp::Player::ConsumeTimer *is_player_eating( entt::registry &reg );

//! @brief Get the player's dowsing rod doglegs, creating an empty component if the player has none in this scene
//! @param reg
//! @return Cmp::Player::Doglegs& modifiable component
Cmp::Player::Doglegs &get_doglegs( entt::registry &reg );

//! @brief Apply a player weapon hit to a plant (Cmp::PlantMultiBlock + Cmp::WorldItem) or NPC (Cmp::Npc::NPC).
//! @details The target loses its "damage_from_weapon" for this weapon from its Cmp::Health, and spawns a Cmp::DamagePopup
//! if Cmp::Persist::ShowDamageText is on. The weapon wears by the target's "damage_to_weapon", but only while it is still
//! the player's held item (an arrow may land after the bow is swapped out). Killing the target is left to the caller.
//! @param reg reference to the entt registry
//! @param target_entt The plant multiblock or NPC entity that was hit.
//! @param weapon The weapon's item key, e.g. "item.shovel" or "item.longbow".
//! @return bool True if the hit took the target's health to zero.
bool apply_weapon_hit( entt::registry &reg, entt::entity target_entt, const Sys::ItemKey &weapon );

} // namespace Game::Utils::Player

#endif // SRC_UTILS_PLAYER_HPP__
