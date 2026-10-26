#ifndef SRC_COMPONENTS_ARMED_HPP__
#define SRC_COMPONENTS_ARMED_HPP__

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Time.hpp>

namespace Game::Cmp
{

//! @brief Component marking an entity as an armed bomb block, tracking its fuse/warning timers
//!        and the appearance it should be rendered with while armed.
class Armed
{
public:
  //! @brief Whether an armed entity is the blast epicenter (the entity the bomb was placed on)
  //!        or a linked block that was chained off it.
  enum class EpiCenter
  {
    //! @brief This entity is the blast epicenter; it displays the bomb sprite/z-order.
    YES,
    //! @brief This entity is a linked/chained block, not the epicenter.
    NO
  };

  //! @brief Construct an Armed component and start its fuse/warning timers at zero.
  //! @param fuse_delay Time until the entity detonates.
  //! @param warning_delay Time before detonation to start showing the armed warning colours.
  //! @param display_bomb_sprite Whether the bomb sprite should be displayed on this entity.
  //! @param armed_color_border Border colour to render while armed.
  //! @param armed_color_fill Fill colour to render while armed.
  //! @param index Chain position of this entity among linked armed entities.
  //! @param epicenter Whether this entity is the blast epicenter.
  Armed( sf::Time fuse_delay, sf::Time warning_delay, bool display_bomb_sprite, sf::Color armed_color_border, sf::Color armed_color_fill, int index,
         EpiCenter epicenter = EpiCenter::NO )
      : m_fuse_delay( fuse_delay ),
        m_warning_delay( warning_delay ),
        m_display_bomb_sprite( display_bomb_sprite ),
        m_armed_color_border( armed_color_border ),
        m_armed_color_fill( armed_color_fill ),
        m_index( index ),
        m_epicenter( epicenter )
  {
  }

  //! @brief Time until the entity detonates.
  sf::Time m_fuse_delay;
  //! @brief Time before detonation to start showing the armed warning colours.
  sf::Time m_warning_delay;
  //! @brief Whether the bomb sprite should be displayed on this entity.
  bool m_display_bomb_sprite;
  //! @brief Border colour to render while armed.
  sf::Color m_armed_color_border;
  //! @brief Fill colour to render while armed.
  sf::Color m_armed_color_fill;
  //! @brief Chain position of this entity among linked armed entities.
  int m_index;

  //! @brief Get elapsed time since arming, for comparison against m_fuse_delay.
  //! @return sf::Time Elapsed fuse time.
  sf::Time getElapsedFuseTime() const { return m_elapsed; }
  //! @brief Get elapsed time since arming, for comparison against m_warning_delay.
  //! @return sf::Time Elapsed warning time.
  sf::Time getElapsedWarningTime() const { return m_elapsed; }
  //! @brief Gameplay time elapsed since arming. Accumulated by BombSystem::update().
  sf::Time m_elapsed{};

  //! @brief Whether this entity is the blast epicenter or a linked/chained block.
  EpiCenter m_epicenter = EpiCenter::NO;

private:
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_ARMED_HPP__
