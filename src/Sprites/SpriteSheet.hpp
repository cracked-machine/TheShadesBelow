#ifndef SRC_SPRITES_SPRITESHEET_HPP__
#define SRC_SPRITES_SPRITESHEET_HPP__

#include <SFML/System/Vector2.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <Utils/Constants.hpp>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace Game::Sprites
{

//! @brief How a sprite sheet's zorder list is applied, set by the property used in spritesheets.json.
enum class ZOrderMode
{
  //! @brief No zorder property: every sprite sorts at its base y.
  None,
  //! @brief `AbsoluteZOrder`: a non-zero entry is the zorder itself.
  Absolute,
  //! @brief `RelativeZOrder`: a non-zero entry is added to the base y.
  Relative
};

//! @brief Non-mutable container for multiple sprites loaded from a single tile sheet.
class SpriteSheet
{
public:
  //! @brief Used by SpriteStore::SpriteMetaData struct declaration
  SpriteSheet() = default;

  //! @brief Construct a new Multi Sprite object using path to texture file
  //! @param type
  //! @param display_name
  //! @param zorder_mode Whether zorder_list holds absolute or relative values.
  //! @param zorder_list
  //! @param spritesheet_path
  //! @param spritesheet_selections
  //! @param grid_size
  //! @param indices_per_sequence
  //! @param walking_collision_mask
  //! @param flying_collision_mask
  //! @param door_position
  //! @param particle_offsets Particle spawn points, as pixel offsets from the sprite's top-left corner.
  //! @param framerate Animation framerate in frames per second. Forced to 0 for single-frame sheets.
  explicit SpriteSheet( Sys::SpriteKey type, std::string display_name, ZOrderMode zorder_mode, std::vector<float> zorder_list,
                        const std::filesystem::path &spritesheet_png, const std::vector<uint32_t> &spritesheet_selections,
                        sf::Vector2i grid_size = { 1, 1 }, unsigned int indices_per_sequence = 1,
                        std::vector<bool> walking_collision_mask = {}, std::vector<bool> flying_collision_mask = {},
                        sf::Vector2i door_position = {}, std::vector<sf::Vector2f> particle_offsets = {},
                        float framerate = 0.f );

  //! @brief Construct a new Multi Sprite object using texture object
  //! @param type
  //! @param display_name
  //! @param zorder_mode Whether zorder_list holds absolute or relative values.
  //! @param zorder_list
  //! @param spritesheet_texture
  //! @param spritesheet_selections
  //! @param grid_size
  //! @param indices_per_sequence
  //! @param walking_collision_mask
  //! @param flying_collision_mask
  //! @param door_position
  //! @param particle_offsets Particle spawn points, as pixel offsets from the sprite's top-left corner.
  //! @param framerate Animation framerate in frames per second. Forced to 0 for single-frame sheets.
  explicit SpriteSheet( Sys::SpriteKey type, std::string display_name, ZOrderMode zorder_mode, std::vector<float> zorder_list, sf::Texture spritesheet_texture,
                        const std::vector<uint32_t> &spritesheet_selections, sf::Vector2i grid_size = { 1, 1 },
                        unsigned int indices_per_sequence = 1, std::vector<bool> walking_collision_mask = {},
                        std::vector<bool> flying_collision_mask = {}, sf::Vector2i door_position = {},
                        std::vector<sf::Vector2f> particle_offsets = {}, float framerate = 0.f );

  SpriteSheet( SpriteSheet && ) = default;
  SpriteSheet &operator=( SpriteSheet && ) = default;
  ~SpriteSheet() = default;

  //! @brief Get the grid size object
  //! @return SpriteSize
  [[nodiscard]] sf::Vector2i get_grid_size() const { return m_grid_size; }

  //! @brief Get the sprite size in pixels, derived from the grid size and the game's grid cell size.
  //! @return sf::Vector2f
  [[nodiscard]] sf::Vector2f sprite_size() const
  {
    return { static_cast<float>( m_grid_size.x ) * Constants::kGridSizePxF.x, static_cast<float>( m_grid_size.y ) * Constants::kGridSizePxF.y };
  }

  //! @brief The number of sprites in this sprite sheet
  //! @return std::size_t
  [[nodiscard]] std::size_t sheet_size() const { return m_va_list.size(); }

  //! @brief Get the vertex array for one sprite in the sheet, ready to be drawn against the sheet's texture.
  //! @param index Sprite index in the range [0, sprite count)
  //! @return const sf::VertexArray&
  //! @throws std::out_of_range if index is not less than the sprite count.
  [[nodiscard]] const sf::VertexArray &get_vertex_array( std::size_t index ) const { return m_va_list.at( index ); }

  //! @brief Pick a random texture index within this sprite sheet.
  //! @return A texture index in the range [0, sprite count)
  [[nodiscard]] std::size_t get_random_texture_index() const;

  //! @brief How many `sprite_indices` entries the animation loops over, one per animation frame. Each entry
  //! already selects a whole `grid_size` area, including for multi-block sprites.
  //! @return unsigned int
  [[nodiscard]] unsigned int indices_per_sequence() const { return m_indices_per_sequence; }

  //! @brief Animation framerate for this sheet, from `framerate` in spritesheets.json.
  //! @return float Frames per second. 0 = not animated, which is always the case for single-frame sheets.
  [[nodiscard]] float framerate() const { return m_framerate; }

  //! @brief Which cells of the sprite collide with walking actors, one bool per grid cell in row-major order.
  //! @return Either empty, meaning every cell collides, or at least one entry per `grid_size` cell.
  [[nodiscard]] const std::vector<bool> &walking_collision_mask() const { return m_walking_collision_mask; }

  //! @brief Which cells of the sprite block the player while flying, one bool per grid cell in row-major order.
  //! @return Either empty, meaning not specified (see Sys::FlyingSystem::blocks_flight for the fallback), or at
  //! least one entry per `grid_size` cell.
  [[nodiscard]] const std::vector<bool> &flying_collision_mask() const { return m_flying_collision_mask; }

  //! @brief Get the door tile position within the sprite grid.
  //! @return sf::Vector2i
  [[nodiscard]] sf::Vector2i door_position() const { return m_door_position; }

  //! @brief Get the particle spawn points, from `particle_offsets` in spritesheets.json.
  //! @return Pixel offsets from the sprite's top-left corner. Empty if not specified.
  [[nodiscard]] const std::vector<sf::Vector2f> &particle_offsets() const { return m_particle_offsets; }

  //! @brief Get the texture for the whole spritesheet
  //! @return const sf::Texture&
  [[nodiscard]] const sf::Texture &texture() const { return *m_spritesheet_texture; }

  //! @brief Get the display name for the spritesheet
  //! @return std::string
  [[nodiscard]] std::string display_name() const { return m_display_name; }

  //! @brief Whether spritesheets.json gives this sheet an explicit (non-zero) zorder for any sprite.
  [[nodiscard]] bool has_zorder() const
  {
    return std::ranges::any_of( m_zorder_list, []( float z ) { return z != 0.f; } );
  }

  //! @brief Resolve the draw order for sprite `idx` positioned at `base_y`.
  //! @note A zero entry, a missing entry at `idx` or ZOrderMode::None all resolve to `base_y`.
  //! @param idx Sprite index (or multiblock grid cell) into the zorder list.
  //! @param base_y The y-axis value the sprite sorts at by default.
  //! @return float `base_y` + entry if relative, the entry if absolute, or `base_y` if unset.
  [[nodiscard]] float zorder( size_t idx, float base_y ) const
  {
    if ( m_zorder_mode == ZOrderMode::None ) return base_y;
    if ( idx >= m_zorder_list.size() )
    {
      SPDLOG_DEBUG( "Could not find zorder for {} at index {}, defaulting to base y.", m_sprite_type.str(), std::to_string( idx ) );
      return base_y;
    }
    const float value = m_zorder_list.at( idx );
    if ( value == 0.f ) return base_y;
    return m_zorder_mode == ZOrderMode::Absolute ? value : base_y + value;
  }

  //! @brief Get the unique identifier for this sprite sheet
  //! @return const Sys::SpriteKey&
  [[nodiscard]] const Sys::SpriteKey &type() const { return m_sprite_type; }

private:
  //! @brief One vertex array per sprite in the sheet, each ready to be drawn against m_spritesheet_texture.
  std::vector<sf::VertexArray> m_va_list;

  //! @brief Texture backing every sprite's vertex array in m_va_list. Shared so cheap copies of SpriteSheet
  //! don't duplicate GPU texture data.
  std::shared_ptr<sf::Texture> m_spritesheet_texture;

  //! @brief Build the vertex array(s) for the tile indices picked from the tilemap texture.
  //! @param spritesheet_selections
  //! @return true if the sprite(s) were added successfully
  //! @return false otherwise
  bool add_sprite( const std::vector<uint32_t> &spritesheet_selections );

  //! @brief Apply the collision mask rules from spritesheets.json: an omitted mask stays empty, and a single
  //! entry is expanded to that value for every grid cell.
  //! @param collision_mask The mask as loaded.
  //! @param grid_size The sprite's grid size.
  //! @param type The sprite sheet's type, for the error message.
  //! @param field The JSON property name, for the error message.
  //! @return The mask to store.
  //! @throws std::runtime_error if the mask has more than one entry but fewer than one per grid cell.
  static std::vector<bool> normalise_collision_mask( std::vector<bool> collision_mask, sf::Vector2i grid_size, const Sys::SpriteKey &type,
                                                     std::string_view field );

  //! @brief Apply the framerate rules from spritesheets.json: a sheet with a single animation frame never
  //! animates, so its framerate is 0 whatever was loaded, and a negative framerate is treated as 0.
  //! @param framerate The framerate as loaded, in frames per second.
  //! @param indices_per_sequence The sheet's indices per sequence.
  //! @param type The sprite sheet's type, for the warning message.
  //! @return The framerate to store.
  static float normalise_framerate( float framerate, unsigned int indices_per_sequence, const Sys::SpriteKey &type );

  //! @brief Identifier for this sprite sheet's type.
  Sys::SpriteKey m_sprite_type;

  //! @brief Human-readable name for this sprite sheet, e.g. shown in editor/debug UI.
  std::string m_display_name;

  //! @brief Whether m_zorder_list holds absolute or relative values. See zorder().
  ZOrderMode m_zorder_mode{ ZOrderMode::None };

  //! @brief Draw-order value per sprite index, indexed the same way as m_va_list.
  std::vector<float> m_zorder_list;

  //! @brief width and height grid size for the multi-sprite
  sf::Vector2i m_grid_size{ 1, 1 };

  //! @brief Number of `sprite_indices` entries in a full animation sequence. See indices_per_sequence().
  unsigned int m_indices_per_sequence{ 1 };

  //! @brief Which grid cells of the sprite collide with walking actors, in row-major order. See walking_collision_mask().
  std::vector<bool> m_walking_collision_mask;

  //! @brief Which grid cells of the sprite block flight, in row-major order. See flying_collision_mask().
  std::vector<bool> m_flying_collision_mask;

  //! @brief Door tile position within the sprite grid, if this sheet represents a structure with a door.
  sf::Vector2i m_door_position;

  //! @brief Particle spawn points in pixels, relative to the sprite's top-left corner. See particle_offsets().
  std::vector<sf::Vector2f> m_particle_offsets;

  //! @brief Animation framerate in frames per second. See framerate().
  float m_framerate{ 0.f };
};

} // namespace Game::Sprites

#endif // SRC_SPRITES_SPRITESHEET_HPP__
