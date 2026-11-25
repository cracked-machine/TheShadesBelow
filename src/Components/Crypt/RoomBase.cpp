#include <Components/Crypt/RoomBase.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Utils.hpp>

namespace Game::Cmp::Crypt
{

void RoomBase::init_passage_connection( bool north, bool south, bool west, bool east )
{
  float half_width = size.x / 2;
  float half_height = size.y / 2;

  // clang-format off
    if(north) {
      m_connectors[CryptPassageDirection::NORTH] = 
        PassageDoor(Utils::snap_to_grid( sf::Vector2f{ position.x + half_width, position.y - Constants::kGridSizePxF.y },
                                              Utils::Rounding::TOWARDS_ZERO ), 
                                              false, 
                                              CryptPassageDirection::NORTH );
    }
    if( south) {
      m_connectors[CryptPassageDirection::EAST] = 
        PassageDoor(Utils::snap_to_grid( sf::Vector2f{ position.x + size.x, position.y + half_height }, 
                                              Utils::Rounding::TOWARDS_ZERO ), 
                                              false,
                                              CryptPassageDirection::EAST );
    }
    if(west) {
      m_connectors[CryptPassageDirection::WEST] = 
        PassageDoor(Utils::snap_to_grid( sf::Vector2f{ position.x - Constants::kGridSizePxF.x, position.y + half_height },
                                              Utils::Rounding::TOWARDS_ZERO ),
                                              false, 
                                              CryptPassageDirection::WEST );
    }
    if(east) {
      m_connectors[CryptPassageDirection::SOUTH] = 
        PassageDoor(Utils::snap_to_grid( sf::Vector2f( position.x + half_width, position.y + size.y ), 
                                              Utils::Rounding::TOWARDS_ZERO ), 
                                              false,
                                              CryptPassageDirection::SOUTH );
    }
    // clang-format on  
    
  }

} // namespace Game::Cmp