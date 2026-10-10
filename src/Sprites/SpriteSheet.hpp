#ifndef SRC_SPRITES_SPRITESHEET_HPP__
#define SRC_SPRITES_SPRITESHEET_HPP__

#include <SFML/System/Vector2.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <Utils/Constants.hpp>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Game::Sprites
{

//! @brief Non-mutable container for multiple sprites loaded from a single tile sheet.
class SpriteSheet
{
public:
  //! @brief Used by SpriteStore::SpriteMetaData struct declaration
  SpriteSheet() = default;

  //! @brief Construct a new Multi Sprite object using path to texture file
  //! @param type
  //! @param display_name
  //! @param zorder_list
  //! @param spritesheet_path
  //! @param spritesheet_selections
  //! @param grid_size
  //! @param indices_per_frame
  //! @param indices_per_sequence
  //! @param collision_mask
  //! @param door_position
  explicit SpriteSheet( Sys::SpriteKey type, std::string display_name, const std::vector<float> &zorder_list,
                        const std::filesystem::path &spritesheet_png, const std::vector<uint32_t> &spritesheet_selections,
                        sf::Vector2i grid_size = { 1, 1 }, unsigned int indices_per_frame = 1, unsigned int indices_per_sequence = 1,
                        std::vector<bool> collision_mask = {}, sf::Vector2i door_position = {} );

  //! @brief Construct a new Multi Sprite object using texture object
  //! @param type
  //! @param display_name
  //! @param zorder_list
  //! @param spritesheet_texture
  //! @param spritesheet_selections
  //! @param grid_size
  //! @param indices_per_frame
  //! @param indices_per_sequence
  //! @param collision_mask
  //! @param door_position
  explicit SpriteSheet( Sys::SpriteKey type, std::string display_name, const std::vector<float> &zorder_list, sf::Texture spritesheet_texture,
                        const std::vector<uint32_t> &spritesheet_selections, sf::Vector2i grid_size = { 1, 1 }, unsigned int indices_per_frame = 1,
                        unsigned int indices_per_sequence = 1, std::vector<bool> collision_mask = {}, sf::Vector2i door_position = {} );

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

  //! @brief How many `sprite_indices` entries make one animation frame. Each entry already selects a whole
  //! `grid_size` area, so this is normally 1.
  //! @return unsigned int
  [[nodiscard]] unsigned int indices_per_frame() const { return m_indices_per_frame; }

  //! @brief How many `sprite_indices` entries the animation loops over. The number of animation frames is this
  //! divided by indices_per_frame().
  //! @return unsigned int
  [[nodiscard]] unsigned int indices_per_sequence() const { return m_indices_per_sequence; }

  //! @brief Which cells of the sprite collide, one bool per grid cell in row-major order.
  //! @return Either empty, meaning every cell collides, or at least one entry per `grid_size` cell.
  [[nodiscard]] const std::vector<bool> &collision_mask() const { return m_collision_mask; }

  //! @brief Get the door tile position within the sprite grid.
  //! @return sf::Vector2i
  [[nodiscard]] sf::Vector2i door_position() const { return m_door_position; }

  //! @brief Get the texture for the whole spritesheet
  //! @return const sf::Texture&
  [[nodiscard]] const sf::Texture &texture() const { return *m_spritesheet_texture; }

  //! @brief Get the display name for the spritesheet
  //! @return std::string
  [[nodiscard]] std::string display_name() const { return m_display_name; }

  //! @brief Get the zorder value for a specific sprite in this sheet.
  //! @note This will return zero if the sprite sheet is missing an entry at `idx`
  //! @param idx
  //! @return float
  [[nodiscard]] float zorder( size_t idx ) const
  {
    assert( not m_zorder_list.empty() );
    if ( idx >= m_zorder_list.size() )
    {
      SPDLOG_DEBUG( "Could not find zorder for {} at index {}, defaulting to 0 index.", m_sprite_type.str(), std::to_string( idx ) );
      return 0;
    }
    return m_zorder_list.at( idx );
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

  //! @brief Apply the collision mask rules from spritesheets.json: an omitted mask stays empty (every cell
  //! collides), and a single entry is expanded to that value for every grid cell.
  //! @param collision_mask The mask as loaded.
  //! @param grid_size The sprite's grid size.
  //! @param type The sprite sheet's type, for the error message.
  //! @return The mask to store.
  //! @throws std::runtime_error if the mask has more than one entry but fewer than one per grid cell.
  static std::vector<bool> normalise_collision_mask( std::vector<bool> collision_mask, sf::Vector2i grid_size, const Sys::SpriteKey &type );

  //! @brief Identifier for this sprite sheet's type.
  Sys::SpriteKey m_sprite_type;

  //! @brief Human-readable name for this sprite sheet, e.g. shown in editor/debug UI.
  std::string m_display_name;

  //! @brief Draw-order value per sprite index, indexed the same way as m_va_list.
  std::vector<float> m_zorder_list;

  //! @brief width and height grid size for the multi-sprite
  sf::Vector2i m_grid_size{ 1, 1 };

  //! @brief Number of `sprite_indices` entries per animation frame. See indices_per_frame().
  unsigned int m_indices_per_frame{ 1 };

  //! @brief Number of `sprite_indices` entries in a full animation sequence. See indices_per_sequence().
  unsigned int m_indices_per_sequence{ 1 };

  //! @brief Which grid cells of the sprite collide, in row-major order. See collision_mask().
  std::vector<bool> m_collision_mask;

  //! @brief Door tile position within the sprite grid, if this sheet represents a structure with a door.
  sf::Vector2i m_door_position;
};

} // namespace Game::Sprites

#endif // SRC_SPRITES_SPRITESHEET_HPP__
