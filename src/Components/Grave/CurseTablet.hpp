#ifndef SRC_CMPS_GRAVE_CURSETABLET_HPP__
#define SRC_CMPS_GRAVE_CURSETABLET_HPP__

namespace Game::Cmp::Grave
{

class CurseTablet
{
public:
  //! @brief The NPC to banish
  enum class Type { GHOST, SKELETON, ADDER };

  CurseTablet( int required_hits, Type type )
      : m_required_hits( required_hits ),
        m_type( type )
  {
  }

  //! @brief Number of hits required to destroy the tablet and number of NPC to banish
  int m_required_hits;
  //! @brief The NPC to banish
  Type m_type;
};

} // namespace Game::Cmp::Grave

#endif // SRC_CMPS_GRAVE_CURSETABLET_HPP__