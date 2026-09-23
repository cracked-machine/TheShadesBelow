#ifndef SRC_SCENECONTROL_SCENEDATA_HPP__
#define SRC_SCENECONTROL_SCENEDATA_HPP__

#include <Sprites/SpriteMetaType.hpp>
#include <Utils/JsonDeserializer.hpp>
#include <nlohmann/json_fwd.hpp>

namespace Game::Scene
{

//! @brief Deserializes a Tiled JSON scene file into map/tileset/tilelayer data used to build a
//!        scene's game area (level generation, floor/wall rendering, multiblocks, player start).
class SceneData : public Utils::JsonDeserializer
{
public:
  //! @brief External tileset used for levelgen layer.
  //!        The IDs are used by the level generator to determine the permitted placement types.
  struct MainTileSet
  {
    //! @brief The offset for the tile indexes within this tileset relative to the other tilesets.
    //! @note  This value is created and managed by Tiled application.
    int first_gid{ 0 };

    //! @brief Void cell
    int void_tile_id{ 0 };
    //! @brief Wall cell
    int wall_tile_id{ 0 };
    //! @brief Open cell
    int open_tile_id{ 0 };
    //! @brief Spawn cell
    int spawn_tile_id{ 0 };
    //! @brief Player cell
    int player_tile_id{ 0 };
    //! @brief Exit cell
    int exit_tile_id{ 0 };
    //! @brief Reserved cell
    int reserved_tile_id{ 0 };

    //! @brief Should be 16x16 pixels
    sf::Vector2u tile_size;
  };

  //! @brief The floor tileset
  struct FloorTileSet
  {
    //! @brief The png image file for the tileset.
    //! @note We have a specialised TileMap class for loading floor tilesets. i.e. not SpriteSheet.
    std::filesystem::path tileset_image;
    //! @brief List of selections to randomly choose from the tileset.
    //! @note  Duplicates will increase the odds of that index being picked.
    std::vector<int> tileset_pool;
    //! @brief Should be 16x16 pixels
    sf::Vector2u tile_size;
  };

  //! @brief The wall tileset
  struct WallTileSet
  {
    //! @brief The offset for the tile indexes within this tileset relative to the other tilesets.
    //! @note  This value is created and managed by Tiled application.
    int first_gid;
    //! @brief The SpriteSheet name. This must use a SpriteSheet defined in res/json/spritesheets.json.
    std::string name;
    //! @brief Should be 16x16 pixels
    sf::Vector2u tile_size;
  };

  //! @brief Main data structure holding all deserialized scene data.
  struct Data
  {
    //! @brief Overal game map size for the scene
    sf::Vector2u map_size;

    //! @brief tile index list for the levelgen layout
    std::vector<int> levelgen_tilelayer;
    //! @brief tile index list for the player layout
    std::vector<int> player_tilelayer;
    //! @brief tile index list for the wall layout
    std::vector<int> wall_tilelayer;
    //! @brief tile index list for the object layout
    std::vector<sf::FloatRect> solid_objectlayer;
    //! @brief tile index list for the mutliblock layout
    std::multimap<Sprites::SpriteMetaType, sf::Vector2f> multiblock_objectlayer;

    //! @brief Player start location
    sf::Vector2u player_start_position{ 0, 0 };

    //! @brief levelgen tileset
    MainTileSet main_tileset;
    //! @brief floor tileset
    FloorTileSet floor_tileset;
    //! @brief wall tileset
    WallTileSet wall_tileset;
  };

  //! @brief Construct a new Scene Data object, deserializing the given Tiled JSON scene file.
  //! @param map_file Path to the Tiled JSON scene file to load.
  SceneData( const std::filesystem::path &map_file );

  //! @brief Get the global tile ID (gid) of the void tile in the levelgen layer.
  //! @return int
  [[nodiscard]] int void_tile_id() const { return m_map_data.main_tileset.void_tile_id + m_map_data.main_tileset.first_gid; }
  //! @brief Get the global tile ID (gid) of the wall tile in the levelgen layer.
  //! @return int
  [[nodiscard]] int wall_tile_id() const { return m_map_data.main_tileset.wall_tile_id + m_map_data.main_tileset.first_gid; }
  //! @brief Get the global tile ID (gid) of the open (walkable) tile in the levelgen layer.
  //! @return int
  [[nodiscard]] int open_tile_id() const { return m_map_data.main_tileset.open_tile_id + m_map_data.main_tileset.first_gid; }
  //! @brief Get the global tile ID (gid) of the spawn tile in the levelgen layer.
  //! @return int
  [[nodiscard]] int spawn_tile_id() const { return m_map_data.main_tileset.spawn_tile_id + m_map_data.main_tileset.first_gid; }
  //! @brief Get the global tile ID (gid) of the player tile in the levelgen layer.
  //! @return int
  [[nodiscard]] int player_tile_id() const { return m_map_data.main_tileset.player_tile_id + m_map_data.main_tileset.first_gid; }
  //! @brief Get the global tile ID (gid) of the exit tile in the levelgen layer.
  //! @return int
  [[nodiscard]] int exit_tile_id() const { return m_map_data.main_tileset.exit_tile_id + m_map_data.main_tileset.first_gid; }
  //! @brief Get the global tile ID (gid) of the reserved tile in the levelgen layer.
  //! @return int
  [[nodiscard]] int reserved_tile_id() const { return m_map_data.main_tileset.reserved_tile_id + m_map_data.main_tileset.first_gid; }

  //! @brief Get the multiblock object layer, mapping each multiblock's sprite type to its position.
  //! @return std::multimap<Sprites::SpriteMetaType, sf::Vector2f>
  [[nodiscard]] std::multimap<Sprites::SpriteMetaType, sf::Vector2f> multiblock_objectlayer() const { return m_map_data.multiblock_objectlayer; }
  //! @brief Get the solid (collidable) object layer bounds.
  //! @return std::vector<sf::FloatRect>
  [[nodiscard]] std::vector<sf::FloatRect> solid_objectlayer() const { return m_map_data.solid_objectlayer; }
  //! @brief Get the levelgen tile layer's tile index list.
  //! @return std::vector<int>
  [[nodiscard]] std::vector<int> levelgen_tilelayer() const { return m_map_data.levelgen_tilelayer; }
  //! @brief Get the wall tile layer's tile index list.
  //! @return std::vector<int>
  [[nodiscard]] std::vector<int> wall_tilelayer() const { return m_map_data.wall_tilelayer; }
  //! @brief Get the file path of the floor tileset's image.
  //! @return std::filesystem::path
  [[nodiscard]] std::filesystem::path floor_tileset_image() const { return m_map_data.floor_tileset.tileset_image; }
  //! @brief Get the pool of tileset indexes to randomly choose the floor tile from.
  //! @return std::vector<int>
  [[nodiscard]] std::vector<int> floor_tileset_pool() const { return m_map_data.floor_tileset.tileset_pool; }
  //! @brief Get the player start position as grid position and pixel position.
  //! @return std::pair<sf::Vector2u, sf::Vector2f>
  [[nodiscard]] std::pair<sf::Vector2u, sf::Vector2f> get_player_start_position() const;
  //! @brief Get the wall tileset.
  //! @return WallTileSet
  [[nodiscard]] WallTileSet wall_tileset() const { return m_map_data.wall_tileset; }

  //! @brief  Get the bounding box (in pixels) of every 'spawn' tile in the levelgen layer.
  //!         Used to size procgen spawn rooms (e.g. Cmp::Crypt::RoomStart) to match the authored spawn area.
  [[nodiscard]] sf::FloatRect get_spawn_area_bounds() const;

  //! @brief  Get the map size as grid position and pixel position (x16)
  //!         For example, {50,70} and {800.0,1120.0}
  //! @return std::pair<sf::Vector2u, sf::Vector2f>
  [[nodiscard]] std::pair<sf::Vector2u, sf::Vector2f> map_size() const;

private:
  //! @brief Get the tilesets and tilelayers from the Tiled json file
  //! @param json_scene_file_path Path to the Tiled JSON scene file to load
  void deserialize( const std::filesystem::path &json_scene_file_path );

  //! @brief Get the embedded floor tileset from the JSON object
  //! @param json The JSON input
  //! @return true if a floor tileset was found and deserialized
  //! @return false otherwise
  bool deserialize_int_floor_tileset( const nlohmann::json &json );

  //! @brief Get the embedded wall tileset from the JSON object
  //! @param json The JSON input
  //! @return true if a wall tileset was found and deserialized
  //! @return false otherwise
  bool deserialize_int_wall_tileset( const nlohmann::json &json );

  //! @brief Get the external main tileset from the JSON object
  //! @param scene_tilemap_path Used for meaningful logging
  //! @param tileset The JSON input
  //! @return true if the main tileset was found and deserialized
  //! @return false otherwise
  bool deserialize_ext_main_tileset( const std::filesystem::path &scene_tilemap_path, const nlohmann::json &tileset );

  //! @brief Deserialize the scene tilemap json and copy in 'floor_tileset' and 'main_tileset'
  //! @param scene_tilemap_json The JSON input
  void deserialize_tilelayers( const nlohmann::json &scene_tilemap_json );

  //! @brief Get the player start position from player_tilelayer
  void retrieve_player_start_pos();

  //! @brief The deserialized scene data (tilesets, tile layers, object layers, player start position).
  Data m_map_data;
};

} // namespace Game::Scene

#endif // SRC_SCENECONTROL_SCENEDATA_HPP__
