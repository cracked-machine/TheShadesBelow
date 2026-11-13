#ifndef SRC_COMPONENTS_STATS_SIDEEFFECTS_HPP__
#define SRC_COMPONENTS_STATS_SIDEEFFECTS_HPP__

//! @file
//! @brief The optional side effects of an action (see BaseAction::side_effects()).

#include <Systems/Stores/StoreKey.hpp>

#include <entt/core/fwd.hpp>
#include <optional>
#include <string>
#include <vector>

namespace Game::Cmp
{
class PlayerStats;

//! @brief How likely a side effect is to fire each time its action is applied.
enum class Probability {
  //! @brief Always fires.
  ALWAYS,
  //! @brief Rolled against the player's luck: high luck makes good effects likelier and bad effects rarer.
  PLAYER_LUCK
};

//! @brief Optional side effect of an action: spawn an NPC at the action's source position.
struct SpawnNpcEffect
{
  //! @brief Where the NPC spawns relative to the action's source.
  enum class Location {
    //! @brief A random free cell beside the source.
    NEARBY,
    //! @brief The NPC sprite centred on the source's exact pixel centre (not grid snapped), e.g. on an altar.
    CENTER
  };

  //! @brief The NPC to spawn, see res/json/npc.json.
  std::string npc_type;
  //! @brief Where the NPC spawns relative to the action's source.
  Location location{ Location::NEARBY };
  //! @brief How likely the effect is to fire each time the action is applied.
  Probability probability{ Probability::ALWAYS };
  //! @brief Max live NPCs spawned by each world item instance, across all of its actions. 0 = unlimited.
  int max_instances{ 0 };
  //! @brief Distance in blocks from the item that the player must be within before the NPC spawn is triggered.
  int trigger_range{ 1 };
  //! @brief Distance from the item to spawned NPC.
  int spawn_radius{ 1 };

  //! @brief Optional gate: the NPC only spawns while a player stat equals a value. See "side_effects.spawn_npc.if".
  struct Condition
  {
    //! @brief The player stat compared.
    enum class Stat {
      HEALTH,
      FEAR,
      DESPAIR,
      INFAMY,
      LUCK,
      ANIMA,
      //! @brief One toxidrome's toxicity, 0 if inactive (see toxidrome_id).
      TOXIDROME,
      //! @brief The summed toxicity of every active toxidrome.
      ALL_TOXIDROMES
    };
    //! @brief The player stat compared.
    Stat stat{ Stat::HEALTH };
    //! @brief The "stat" name as written in JSON, for display.
    std::string stat_name;
    //! @brief The toxidrome's entt::type_hash id, only used when stat is TOXIDROME.
    entt::id_type toxidrome_id{ 0 };
    //! @brief The value the stat must equal.
    int value{ 0 };
  };
  //! @brief Spawn only while this holds. No condition = always eligible.
  std::optional<Condition> condition;

  //! @brief Whether the condition (if any) holds for the given player stats.
  //! @param stats The player's current stats.
  //! @return true if there is no condition or it holds.
  [[nodiscard]] bool condition_met( const PlayerStats &stats ) const;
  //! @brief This effect's chance of firing at the given player luck.
  //! @param player_luck Player's luck stat, range [0, 100].
  //! @return Chance in [0, 1].
  [[nodiscard]] float chance( int player_luck ) const;
  //! @brief Roll this effect's probability.
  //! @param player_luck Player's luck stat, range [0, 100].
  //! @return true if the effect should fire.
  [[nodiscard]] bool roll( int player_luck ) const;
};

//! @brief Optional side effect of an action: a lightning strike (on a working lightning rod in view, else the player).
struct LightningEffect
{
  //! @brief How likely the effect is to fire each time the action is applied.
  Probability probability{ Probability::ALWAYS };
  //! @brief This effect's chance of firing at the given player luck.
  //! @param player_luck Player's luck stat, range [0, 100].
  //! @return Chance in [0, 1].
  [[nodiscard]] float chance( int player_luck ) const;
  //! @brief Roll this effect's probability.
  //! @param player_luck Player's luck stat, range [0, 100].
  //! @return true if the effect should fire.
  [[nodiscard]] bool roll( int player_luck ) const;
};

//! @brief Optional side effect of an action affecting player movement and/or NPC pathfinding.
struct MovementEffect
{
  //! @brief Effect on the player's movement.
  struct Player
  {
    //! @brief The kind of player movement effect.
    enum class Type {
      //! @brief Slow the player down.
      SLOW,
      //! @brief Make the player's movement erratic.
      DRUNK
    };
    //! @brief The kind of player movement effect.
    Type type{ Type::SLOW };
    //! @brief How long (seconds) the effect lasts.
    int tick{ 0 };
  };

  //! @brief Effect on NPC pathfinding.
  struct Npc
  {
    //! @brief The kind of NPC pathfinding effect.
    enum class Type {
      //! @brief Slow NPCs down.
      SLOW,
      //! @brief Make NPC pathfinding erratic.
      DRUNK,
      //! @brief NPCs flee from the player instead of chasing them.
      REPEL
    };
    //! @brief The kind of NPC pathfinding effect.
    Type type{ Type::SLOW };
    //! @brief How long (seconds) the effect lasts.
    int tick{ 0 };
  };

  //! @brief Effect on the player's movement, if any.
  std::optional<Player> player;
  //! @brief Effect on NPC pathfinding, if any.
  std::optional<Npc> npc;
  //! @brief How likely the effect is to fire each time the action is applied.
  Probability probability{ Probability::ALWAYS };
  //! @brief This effect's chance of firing at the given player luck.
  //! @param player_luck Player's luck stat, range [0, 100].
  //! @return Chance in [0, 1].
  [[nodiscard]] float chance( int player_luck ) const;
  //! @brief Roll this effect's probability.
  //! @param player_luck Player's luck stat, range [0, 100].
  //! @return true if the effect should fire.
  [[nodiscard]] bool roll( int player_luck ) const;
};

//! @brief Optional side effect of an action: toxidromes can't increase for tick seconds (see Cmp::Player::ToxicImmunity).
struct ToxicImmunityEffect
{
  //! @brief How long (seconds) the immunity lasts.
  int tick{ 0 };
  //! @brief How likely the effect is to fire each time the action is applied.
  Probability probability{ Probability::ALWAYS };
  //! @brief This effect's chance of firing at the given player luck.
  //! @param player_luck Player's luck stat, range [0, 100].
  //! @return Chance in [0, 1].
  [[nodiscard]] float chance( int player_luck ) const;
  //! @brief Roll this effect's probability.
  //! @param player_luck Player's luck stat, range [0, 100].
  //! @return true if the effect should fire.
  [[nodiscard]] bool roll( int player_luck ) const;
};

//! @brief Optional side effect of an action: kill NPCs that come near the action's source (proximity_action only).
//! @note The Burryman is immune (see Events::DestroyNpcEvent) and NPCs spawned by the source itself are spared.
struct KillNpcEffect
{
  //! @brief The NPC types that can be killed, see res/json/npc.json. Empty = any NPC.
  std::vector<Sys::NpcKey> npc_types;
  //! @brief Distance in blocks from the source that an NPC must be within to be killed.
  int trigger_range{ 1 };
  //! @brief Sprite played where the NPC dies, or std::nullopt for no animation.
  std::optional<Sys::SpriteKey> death_anim;
  //! @brief How likely the effect is to fire, rolled per NPC in range each time the action is applied.
  Probability probability{ Probability::ALWAYS };
  //! @brief Wear (out of 100) the source takes per kill. A fully worn source is destroyed.
  float wear_per_kill{ 0.f };
  //! @brief Whether this effect can kill the given NPC type.
  //! @param npc_type The NPC's type key.
  //! @return true if npc_types is empty or contains npc_type.
  [[nodiscard]] bool targets( const Sys::NpcKey &npc_type ) const;
  //! @brief This effect's chance of firing at the given player luck.
  //! @param player_luck Player's luck stat, range [0, 100].
  //! @return Chance in [0, 1].
  [[nodiscard]] float chance( int player_luck ) const;
  //! @brief Roll this effect's probability.
  //! @param player_luck Player's luck stat, range [0, 100].
  //! @return true if the effect should fire.
  [[nodiscard]] bool roll( int player_luck ) const;
};

//! @brief The optional side effects of an action, each rolled independently. See the "side_effects" object in
//! res/json/items.json and res/json/npc.json.
struct SideEffects
{
  //! @brief Spawn (or relocate) an NPC.
  std::optional<SpawnNpcEffect> spawn_npc;
  //! @brief Lightning strike.
  std::optional<LightningEffect> lightning;
  //! @brief Movement/pathfinding effect.
  std::optional<MovementEffect> movement;
  //! @brief Toxic immunity.
  std::optional<ToxicImmunityEffect> toxic_immunity;
  //! @brief Kill nearby NPCs.
  std::optional<KillNpcEffect> kill_npc;
  //! @brief Whether there are no effects at all.
  //! @return true if no effect is set.
  [[nodiscard]] bool empty() const { return not spawn_npc and not lightning and not movement and not toxic_immunity and not kill_npc; }
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_SIDEEFFECTS_HPP__
