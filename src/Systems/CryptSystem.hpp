#ifndef SRC_SYSTEMS_CRYPTSYSTEM_HPP__
#define SRC_SYSTEMS_CRYPTSYSTEM_HPP__

#include <Events/CryptRoomEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Utils/Crypt.hpp>

#include <Factory/ObstacleFactory.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Crypt/Variants/CryptLayoutParams.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <set>
#include <span>

// clang-format off
namespace Game::Cmp { class Position; }
namespace Game::Cmp::Crypt { class RoomEnd; class RoomStart; }
namespace Game::Sys::Crypt { class CryptVariant; }
// clang-format on

namespace Game::Sys
{

//! @brief Crypt toolkit: room open/close primitives, levers, chests, lava pits, spike traps, and the graveyard
//! entrance/exit/objective mechanics. The layout and progression policy (when rooms open, which content appears)
//! belongs to the active Sys::Crypt::CryptVariant, which composes the public primitives below.
class CryptSystem : public Game::Sys::BaseSystem
{
public:
  //! @brief Which wall category a restored obstacle belongs to when decorating a room.
  enum class RoomWallType {
    //! @brief A wall on the interior of a room.
    INTERIOR = 0,
    //! @brief A wall on the outer border between rooms.
    BORDER = 1
  };

  //! @brief Construct a new Crypt System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  //! @param scenemanager_event_dispatcher
  CryptSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank, entt::dispatcher &scenemanager_event_dispatcher )
      : Game::Sys::BaseSystem( reg, window, sound_bank ),
        m_scenemanager_event_dispatcher( scenemanager_event_dispatcher )
  {
    // The entt::dispatcher is independent of the registry, so it is safe to bind event handlers in the constructor
    std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&CryptSystem::on_player_action>( this );
    std::ignore = get_systems_event_queue().sink<Events::CryptRoomEvent>().connect<&CryptSystem::on_room_event>( this );
  }

  //! @brief init the weak pointer for the spatial grid
  //! @param spatial_grid_ptr
  //! @param item_sm Ground world items (see Scene::m_item_sm).
  void init( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh, const PathFinding::SpatialHashGridSharedPtr &player_navmesh,
             const PathFinding::SpatialHashGridSharedPtr &reserved_sm = nullptr, const PathFinding::SpatialHashGridSharedPtr &loot_sm = nullptr,
             const PathFinding::SpatialHashGridSharedPtr &item_sm = nullptr )
  {
    m_npc_navmesh = npc_navmesh;
    m_player_navmesh = player_navmesh;
    m_reserved_sm = reserved_sm;
    m_loot_sm = loot_sm;
    m_item_sm = item_sm;
  }

  //! @brief Set the variant that owns the crypt's layout/progression policy. Non-owning; the CryptScene owns it.
  //! @param variant The active variant, or nullptr when no crypt scene is active.
  void set_variant( Crypt::CryptVariant *variant ) { m_variant = variant; }

  //! @brief The shared sound bank, for crypt variants to play room/passage effects.
  Audio::SoundBank &sound_bank() { return m_sound_bank; }

  //! @brief Frame update. Called from Scene::do_update()
  void update( sf::Time dt );

  //! @brief Handle player action events
  //! @param event
  void on_player_action( Events::PlayerActionEvent &event );

  //! @brief Handle crypt room events
  //! @param event
  void on_room_event( Events::CryptRoomEvent &event );

  //! @brief Check for collisions with the entrance
  void check_entrance_collision();

  //! @brief Check for collisions with the exit
  void check_exit_collision();

  //! @brief Unlock the crypt door. Refused (key kept, Events::CryptEntryRefusedEvent sent) when the player has already
  //! collected every cadaver part.
  void unlock_crypt_door();

  //! @brief If the player is in front of a closed crypt door without the crypt key, send a crow to fetch it.
  void request_cryptkey_delivery();

  //! @brief Lock again every crypt door tagged with Cmp::Crypt::EnteredDoor, i.e. the door the player walked through
  //! into the crypt. Called from GraveyardScene::on_enter() when the player returns to the graveyard.
  void lock_entered_crypt_doors();

  //! @brief Check for objective activation
  //! @param action
  void check_objective_activation( Events::PlayerActionEvent::GameActions action );

  //! @brief Check for lever activation. Each pull is reported to the active variant.
  void check_lever_activation();

  //! @brief Open a chest the player has activated, dropping loot from it.
  //! @param action
  void check_chest_activation( Events::PlayerActionEvent::GameActions action );

  //! @brief Restores border Cmp::Obstacles to Cmp::Crypt::RoomEnd, Cmp::Crypt::RoomStart and Cmp::Crypt::RoomOpen areas
  //! @param uuid_map
  void create_room_borders( const Factory::Obstacle::UUIDEntityMap &uuid_map );

  //! @brief Generate the initial Crypt interior walls (fills in Cmp::CryptRoomsClosed)
  //! @note Except for start/end rooms, all other rooms should start as Cmp::CryptRoomsClosed
  void gen_crypt_initial_interior();

  //! @brief Create the initial rooms that are closed by default.
  //! @param map_grid_size
  //! @param params Room count/size/spacing tunables
  void create_initial_closed_rooms( sf::Vector2u map_grid_size, const Crypt::LayoutParams &params );

  //! @brief Generate the main objective for the Crypt
  //! @param map_grid_size
  void create_end_room( sf::Vector2u map_grid_size );

  // ---- Room primitives, composed by Sys::Crypt::CryptVariant ----

  //! @brief Build a map of Cmp::UUID -> entity for all entities that are not Cmp::Obstacle
  Factory::Obstacle::UUIDEntityMap build_non_obstacle_uuid_map();

  //! @brief Change all Cmp::Crypt::RoomOpen to Cmp::Crypt::RoomClosed
  //! @note Excludes start/end rooms and the open room occupied by the player)
  //! @param player_pos_cmp
  void close_open_rooms( const Cmp::Position &player_pos_cmp );

  //! @brief Restores missing Cmp::Obstacles components to Cmp::Crypt::RoomClosed areas
  //! @param uuid_map
  void fill_closed_rooms( const Factory::Obstacle::UUIDEntityMap &uuid_map );

  //! @brief Change selected Cmp::Crypt::RoomClosed to Cmp::Crypt::RoomOpen.
  //! @param selected_rooms Set of entity ids to open
  //! @note Removes Cmp::Obstacle
  void open_selected_rooms( const std::set<entt::entity> &selected_rooms );

  //! @brief Open all rooms
  void open_all_rooms();

  //! @brief Removes Cmp::Obstacles from Cmp::Crypt::RoomOpen areas
  //! @param uuid_map
  void empty_open_rooms( const Factory::Obstacle::UUIDEntityMap &uuid_map );

  //! @brief Adds Cmp::Lever components to Cmp::Crypt::RoomOpen areas
  void add_lever_to_open_rooms();

  //! @brief Add a chest to a random border position of each open room the player is not standing in.
  //! @param player_pos_cmp
  void add_chest_to_open_rooms( const Cmp::Position &player_pos_cmp );

  //! @brief Add lava pits to open rooms
  //! @param player_pos_cmp
  void add_lava_pit_open_rooms( const Cmp::Position &player_pos_cmp );

  //! @brief Remove lava pits from open rooms
  //! @param player_pos_cmp
  void remove_lava_pit_open_rooms( const Cmp::Position &player_pos_cmp );

  //! @brief Removes Cmp::Lever components from Cmp::Crypt::RoomOpen areas
  //! @param player_pos_cmp
  void remove_lever_open_rooms( const Cmp::Position &player_pos_cmp );

  //! @brief Destroy chests in open rooms the player is not standing in.
  //! @param player_pos_cmp
  void remove_chest_open_rooms( const Cmp::Position &player_pos_cmp );

  //! @brief Removes ALL levers.
  void remove_all_levers();

  //! @brief Removes ALL chests.
  void remove_all_chests();

  //! @brief Spawn NPCs in the centre of distinct, randomly picked open rooms.
  //! @param npc_types Pool of NPC types; each spawn picks one at random
  //! @param count Requested number of NPCs, capped by the number of open rooms
  void spawn_npc_in_open_rooms( std::span<const Sys::NpcKey> npc_types, int count );

  //! @brief Get the single Cmp::Crypt::RoomStart component
  //! @return Pair of entt::entity and Cmp::Crypt::RoomStart&
  std::pair<entt::entity, Cmp::Crypt::RoomStart &> get_crypt_room_start();

  //! @brief Get the single Cmp::Crypt::RoomEnd component
  //! @return Pair of entt::entity and Cmp::Crypt::RoomEnd&
  std::pair<entt::entity, Cmp::Crypt::RoomEnd &> get_crypt_room_end();

private:
  //! @brief Return an opened graveyard crypt door to its locked state: solid, blocks the player, closed sprite.
  //! Reverses the open steps in unlock_crypt_door().
  //! @param door_entity entity with Cmp::Crypt::Entrance
  //! @param door_pos_cmp the door's position
  void close_crypt_door( entt::entity door_entity, const Cmp::Position &door_pos_cmp );

  //! @brief Find the Cmp::Crypt::BuildingMultiBlock entity intersecting the given position
  //! @param pos_cmp
  //! @return The intersecting entity, or entt::null if none found
  entt::entity find_intersecting_multiblock( const Cmp::Position &pos_cmp );

  //! @brief Restore the interior/border wall obstacle and its z-fighting-avoidance cap block for one wall position.
  //! @param main_entt
  //! @param main_pos_cmp
  //! @param room_wall_type
  void decorate_interior_wall( entt::entity main_entt, const Cmp::Position &main_pos_cmp, RoomWallType room_wall_type );

  //! @brief Collect open-room floor positions not occupied by the player, a lava pit, a lever, a chest,
  //! or a nearby passage block.
  //! @return The candidate position entities.
  std::vector<entt::entity> get_available_room_positions();

  //! @brief Spit a fireball out of a random active lava pit cell once the effect cooldown has elapsed.
  //! @param dt Time elapsed since the last frame.
  void do_lava_pit_animation( sf::Time dt );

  //! @brief Check player collision with lava pits
  void check_lava_pit_collision();

  //! @brief Activate/deactivate the lava pit if player is close
  void check_lava_pit_activation_by_proximity();

  //! @brief Check player collision with the spike traps
  void check_spike_trap_collision();

  //! @brief Activate/deactivate the spike trap if player is close
  //! @note This uses animation component `m_animation_active` member.
  void check_spike_trap_activation_by_proximity( sf::Time dt );

  //! @brief Dispatcher reference for scene management events
  entt::dispatcher &m_scenemanager_event_dispatcher;

  //! @brief The active crypt variant (non-owning), or nullptr outside the crypt scene
  Crypt::CryptVariant *m_variant{ nullptr };

  //! @brief Time accumulated since the last lava pit fireball was emitted.
  sf::Time m_lava_effect_cooldown_elapsed{ sf::Time::Zero };

  //! @brief Minimum time between lava pit fireballs.
  sf::Time m_lava_effect_cooldown_threshold{ sf::seconds( 1.f ) };

  //! @brief All grid positions that block NPC pathfinding
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;

  //! @brief All grid positions that block player movement
  PathFinding::SpatialHashGridWeakPtr m_player_navmesh;

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;

  //! @brief Loot drops (Cmp::Loot)
  PathFinding::SpatialHashGridWeakPtr m_loot_sm;

  //! @brief Ground world items (see Scene::m_item_sm).
  PathFinding::SpatialHashGridWeakPtr m_item_sm;

  //! @brief Elapsed time since the scene was last entered, used to gate exit collision checks
  sf::Time m_scene_entrance_cooldown_timer;

  //! @brief Minimum time after entering the scene before exit collisions (e.g. crypt entrance) are checked
  sf::Time m_scene_entrance_cooldown_time{ sf::seconds( 2 ) };
};

} // namespace Game::Sys
#endif // SRC_SYSTEMS_CRYPTSYSTEM_HPP__
