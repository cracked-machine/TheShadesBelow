#ifndef SRC_COMPONENTS_CRYPT_ROOMEND_HPP__
#define SRC_COMPONENTS_CRYPT_ROOMEND_HPP__

#include <Components/Crypt/RoomBase.hpp>
#include <SFML/Graphics/Rect.hpp>
namespace Game::Cmp::Crypt
{

//! @brief Marks the single terminal/goal room of the crypt maze that the player must reach.
//! @note Systems::CryptSystem::get_crypt_room_end() looks up the one entity carrying this component.
class RoomEnd : public RoomBase
{
public:
  //! @brief Construct the end room covering the given rectangle.
  //! @param bounds World-space bounds of the room.
  RoomEnd( sf::FloatRect bounds )
      : RoomBase( bounds )
  {
    init_passage_connection( false, true, true, true );
  };
  //! @brief Construct the end room covering the given rectangle.
  //! @param position Top-left world position of the room.
  //! @param size Pixel dimensions of the room.
  RoomEnd( sf::Vector2f position, sf::Vector2f size )
      : RoomBase( position, size )
  {
    init_passage_connection( false, true, true, true );
  };
};

} // namespace Game::Cmp::Crypt

#endif // SRC_COMPONENTS_CRYPT_ROOMEND_HPP__
