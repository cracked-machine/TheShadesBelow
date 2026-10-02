#ifndef SRC_SYSTEMS_STORES_SPRITESTORE_HPP__
#define SRC_SYSTEMS_STORES_SPRITESTORE_HPP__

#include <Sprites/SpriteMetaType.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/Stores/StoreSingleton.hpp>

#include <source_location>
#include <string>
#include <unordered_set>
#include <vector>

namespace Game::Sys
{

//! @brief Singleton store of Sprites::SpriteSheet objects (keyed by Sprites::SpriteMetaType) loaded from res/json/spritesheets.json.
class SpriteStore : public StoreSingleton<SpriteStore, Sprites::SpriteSheet>
{
public:
  //! @brief Construct a new Sprite Store object
  //! @param json_file_path Path of the JSON file to load; override to load a fixture (e.g. in tests).
  explicit SpriteStore( std::filesystem::path json_file_path = "res/json/spritesheets.json" );

  //! @brief Destroy the Sprite Store object
  ~SpriteStore() {}

  //! @brief  Selects a random sprite type and texture index.
  //! This function randomly selects a sprite type from the given list of types and returns
  //! it along with a corresponding texture index.
  //! @param type_list A selection list of Sprites::SpriteMetaType values
  //! @return A pair containing the selected Sprites::SpriteMetaType and its associated texture index
  std::pair<Sprites::SpriteMetaType, std::size_t> get_random_type_and_texture_index( std::vector<Sprites::SpriteMetaType> type_list );

  //! @brief Selects a random sprite type from the given list of types.
  //! @param type_list A selection list of Sprites::SpriteMetaType values
  //! @return The selected Sprites::SpriteMetaType, or "ERROR_SPRITE" if `type_list` is empty.
  Sprites::SpriteMetaType get_random_type( std::vector<Sprites::SpriteMetaType> type_list );

  //! @brief Get the all sprite types by pattern object
  //! Supports regex and plain text matching.
  //!
  //! @param pattern partial string pattern to match sprite types
  //! @return std::vector<Sprites::SpriteMetaType>
  std::vector<Sprites::SpriteMetaType> get_all_sprite_types_by_pattern( const std::string &pattern );

  //! @brief  Retrieves a Sprites::SpriteSheet object based on the specified sprite meta type.
  //! This method searches for and returns a Sprites::SpriteSheet that corresponds to the given
  //! Sprites::SpriteMetaType. If the type is not found, it returns the error sprite.
  //! @param type The Sprites::SpriteMetaType to search for
  //! @param loc Call site captured for the error message if the type is not found
  //! @return Sprites::SpriteSheet& const& The Sprites::SpriteSheet object if found
  //! @throws std::runtime_error if the type is not found
  const Sprites::SpriteSheet &get_spritesheet_by_type( const Sprites::SpriteMetaType &type,
                                                       std::source_location loc = std::source_location::current() );

  //! @brief Get a vector of all Sprites::SpriteMetaType objects
  //! @return std::vector<Sprites::SpriteMetaType>
  std::vector<Sprites::SpriteMetaType> get_all_sprite_types();

  //! @brief Get the set of all known Sprites::SpriteMetaType objects.
  //! @return std::unordered_set<Sprites::SpriteMetaType>
  std::unordered_set<Sprites::SpriteMetaType> get_all_sprite_types_set();

  //! @brief Returns the pixel bounds of the first sprite in the array. Assumes that all sprites in the
  //! multi-sprite have the same size.
  //! @param type The Sprites::SpriteMetaType to search for
  //! @param loc Call site captured for the error message if the type is not found
  //! @return sf::Vector2f Pixel size of the sprite
  sf::Vector2f get_sprite_size_by_type( const Sprites::SpriteMetaType &type, std::source_location loc = std::source_location::current() )
  {
    return get_spritedata_by_type( type, loc ).get_sprite_size();
  }

  //! @brief Get the display name of a sprite type.
  //! @param type The Sprites::SpriteMetaType to search for
  //! @param loc Call site captured for the error message if the type is not found
  //! @return std::string Display name of the sprite
  std::string get_display_name_by_type( const Sprites::SpriteMetaType &type, std::source_location loc = std::source_location::current() )
  {
    return get_spritedata_by_type( type, loc ).get_display_name();
  }

private:
  //! @brief Populates m_store with Sprites::SpriteSheet objects and creates the error sprite
  void init_store();

  //! @brief Create a error sprite object
  //! This function creates a special error sprite to prevent engine crashes when handling invalid sprite data.
  void create_error_sprite();

  //! @brief Retrieves sprite metadata by sprite type
  //! Searches for and returns the sprite metadata associated with the specified
  //! sprite type. This method allows lookup of sprite configuration data such as
  //! texture coordinates, dimensions, and other properties based on the sprite type.
  //! @param type The Sprites::SpriteMetaType to search for
  //! @param loc Call site captured for the error message if the type is not found
  //! @return SpriteMetaData& The SpriteMetaData if found
  //! @throws std::runtime_error if the type is not found
  const Sprites::SpriteSheet &get_spritedata_by_type( const Sprites::SpriteMetaType &type,
                                                      std::source_location loc = std::source_location::current() );

  //! @brief Get the Sprites::SpriteSheet for a randomly chosen type from `type_list`.
  //! @param type_list A selection list of Sprites::SpriteMetaType values
  //! @return The randomly selected Sprites::SpriteSheet, or the error sprite if `type_list` is empty.
  const Sprites::SpriteSheet &get_random_spritedata( std::vector<Sprites::SpriteMetaType> type_list );

  //! @brief Error texture for missing sprites
  sf::Texture m_error_texture;

  //! @brief Metadata for the error texture
  //! This contains information about the error texture's properties
  Sprites::SpriteSheet m_error_metadata;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_STORES_SPRITESTORE_HPP__
