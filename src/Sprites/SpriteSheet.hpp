#ifndef SRC_SPRITES_SPRITESHEET_HPP__
#define SRC_SPRITES_SPRITESHEET_HPP__

#include <SFML/System/Vector2.hpp>
#include <Sprites/SpriteMetaType.hpp>

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
  //! @param tilemap_path
  //! @param tilemap_picks
  //! @param grid_size
  //! @param sprites_per_frame
  //! @param sprites_per_sequence
  //! @param solid_mask
  //! @param door_position
  explicit SpriteSheet( SpriteMetaType type, std::string display_name, const std::vector<float> &zorder_list,
                        const std::filesystem::path &tilemap_path, const std::vector<uint32_t> &tilemap_picks, sf::Vector2i grid_size = { 1, 1 },
                        unsigned int sprites_per_frame = 1, unsigned int sprites_per_sequence = 1, std::vector<bool> solid_mask = {},
                        sf::Vector2i door_position = {} );

  //! @brief Construct a new Multi Sprite object using texture object
  //! @param type
  //! @param display_name
  //! @param zorder_list
  //! @param tilemap_texture
  //! @param tilemap_picks
  //! @param grid_size
  //! @param sprites_per_frame
  //! @param sprites_per_sequence
  //! @param solid_mask
  //! @param door_position
  explicit SpriteSheet( SpriteMetaType type, std::string display_name, const std::vector<float> &zorder_list, sf::Texture tilemap_texture,
                        const std::vector<uint32_t> &tilemap_picks, sf::Vector2i grid_size = { 1, 1 }, unsigned int sprites_per_frame = 1,
                        unsigned int sprites_per_sequence = 1, std::vector<bool> solid_mask = {}, sf::Vector2i door_position = {} );

  SpriteSheet( SpriteSheet && ) = default;
  SpriteSheet &operator=( SpriteSheet && ) = default;
  ~SpriteSheet() = default;

  //! @brief Get the grid size object
  //! @return SpriteSize
  [[nodiscard]] sf::Vector2i get_grid_size() const { return m_grid_size; }

  //! @brief Get the sprite size in pixels, derived from the grid size and the game's grid cell size.
  //! @return sf::Vector2f
  [[nodiscard]] sf::Vector2f get_px_size() const
  {
    return { static_cast<float>( m_grid_size.x ) * Constants::kGridSizePxF.x, static_cast<float>( m_grid_size.y ) * Constants::kGridSizePxF.y };
  }

  //! @brief Get the sprite count object
  //! @return std::size_t
  [[nodiscard]] std::size_t get_sprite_count() const { return m_va_list.size(); }

  //! @brief Get the sprites per frame object
  //! @return unsigned int
  [[nodiscard]] unsigned int get_sprites_per_frame() const { return m_sprites_per_frame; }

  //! @brief Get the sprites per sequence object
  //! @return unsigned int
  [[nodiscard]] unsigned int get_sprites_per_sequence() const { return m_sprites_per_sequence; }

  //! @brief Get the solid mask object
  //! @return const std::vector<bool>&
  [[nodiscard]] const std::vector<bool> &get_solid_mask() const { return m_solid_mask; }

  //! @brief Get the door tile position within the sprite grid.
  //! @return sf::Vector2i
  [[nodiscard]] sf::Vector2i get_door_position() const { return m_door_position; }

  //! @brief Get the door tile position within the sprite grid, as a float vector.
  //! @return sf::Vector2f
  [[nodiscard]] sf::Vector2f get_door_position_f() const { return sf::Vector2f( m_door_position ); }

  //! @brief Get the texture object
  //! @return const sf::Texture&
  [[nodiscard]] const sf::Texture &get_texture() const { return *m_tilemap_texture; }

  //! @brief Get the display name object
  //! @return std::string
  [[nodiscard]] std::string get_display_name() const { return m_display_name; }

  //! @brief Get the zorder object.
  //! @note This will return zero if the sprite sheet is missing an entry at `idx`
  //! @param idx
  //! @return float
  [[nodiscard]] float get_zorder( size_t idx ) const
  {
    assert( not m_zorder_list.empty() );
    if ( idx >= m_zorder_list.size() )
    {
      SPDLOG_DEBUG( "Could not find zorder for {} at index {}, defaulting to 0 index.", std::string( m_sprite_type ), std::to_string( idx ) );
      return 0;
    }
    return m_zorder_list.at( idx );
  }

  //! @brief Get the Sprite Size Pixels object
  //! @return sf::Vector2f
  [[nodiscard]] sf::Vector2f get_sprite_size() const { return m_va_list[0].getBounds().size; }

  //! @brief Get the sprite type object
  //! @return SpriteMetaType
  [[nodiscard]] SpriteMetaType get_sprite_type() const { return m_sprite_type; }

  //! @brief Set the sprite type object
  //! @param type
  void set_sprite_type( const SpriteMetaType &type ) { m_sprite_type = type; }

  //! @brief
  // sf::VertexArray m_selected_vertices;

  //! @brief One vertex array per sprite in the sheet, each ready to be drawn against m_tilemap_texture.
  std::vector<sf::VertexArray> m_va_list;

private:
  //! @brief Texture backing every sprite's vertex array in m_va_list. Shared so cheap copies of SpriteSheet
  //! don't duplicate GPU texture data.
  std::shared_ptr<sf::Texture> m_tilemap_texture;

  //! @brief Build the vertex array(s) for the tile indices picked from the tilemap texture.
  //! @param tilemap_picks
  //! @return true if the sprite(s) were added successfully
  //! @return false otherwise
  bool add_sprite( const std::vector<uint32_t> &tilemap_picks );

  //! @brief Identifier for this sprite sheet's type.
  SpriteMetaType m_sprite_type;

  //! @brief Human-readable name for this sprite sheet, e.g. shown in editor/debug UI.
  std::string m_display_name;

  //! @brief Draw-order value per sprite index, indexed the same way as m_va_list.
  std::vector<float> m_zorder_list;

  //! @brief width and height grid size for the multi-sprite
  sf::Vector2i m_grid_size{ 1, 1 };

  //! @brief number of sprites per animation frame
  unsigned int m_sprites_per_frame{ 1 };

  //! @brief Number of animation frames per full animation sequence.
  unsigned int m_sprites_per_sequence{ 1 };

  //! @brief Indicates which 'sprite_indices' the player cannot traverse. Array size must match sprite_indices size.
  std::vector<bool> m_solid_mask;

  //! @brief Door tile position within the sprite grid, if this sheet represents a structure with a door.
  sf::Vector2i m_door_position;
};

} // namespace Game::Sprites

#endif // SRC_SPRITES_SPRITESHEET_HPP__
