#ifndef SRC_COMPONENTS_GRAVE_CONSEQUENCE_HPP__
#define SRC_COMPONENTS_GRAVE_CONSEQUENCE_HPP__

namespace Game::Cmp::Grave
{

class Consequence
{
public:
  //! @brief Random consequence rolled when a grave is fully dug open
  enum class Type {
    //! @brief Spawns a hostile NPC from the grave.
    NPC_TRAP = 1,
    //! @brief Spawns a bomb trap from the grave.
    BOMB_TRAP,
    //! @brief Drops a relic item.
    RELIC,
    //! @brief Drops jewelry loot.
    JEWELRY,
    //! @brief Drops a curse tablet
    CURSE_TABLET
  };

  Consequence( Type type )
      : m_type( type )
  {
  }

  Type get() { return m_type; }

private:
  Type m_type;
};

//! @brief Lets fmt/spdlog print the consequence type as its underlying value
inline int format_as( Consequence::Type type ) { return static_cast<int>( type ); }

} // namespace Game::Cmp::Grave

#endif // SRC_COMPONENTS_GRAVE_CONSEQUENCE_HPP__