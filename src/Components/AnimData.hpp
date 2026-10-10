#ifndef SRC_COMPONENTS_ANIMDATA_HPP__
#define SRC_COMPONENTS_ANIMDATA_HPP__

#include <SFML/System/Time.hpp>
#include <Systems/Stores/StoreKey.hpp>

namespace Game::Cmp
{

//! @brief How an animation sequence behaves once it reaches its last frame.
enum class AnimType
{
  //! @brief Restart the sequence from its base frame and keep animating indefinitely.
  LOOP,
  //! @brief Play once, then deactivate and reset the current frame back to the base frame.
  ONESHOTRESET,
  //! @brief Play once, then deactivate and hold on the last frame reached.
  ONESHOTHOLD
};

//! @brief Component holding sprite-sheet animation state for an entity (current frame, framerate,
//!        looping behaviour, etc).
struct AnimData
{
  //! @brief Construction parameters for AnimData.
  struct Config
  {
    //! @brief The SpriteSheet object type (string) that will be animated.
    Sys::SpriteKey sprite_type;
    //! @brief Initial frame position in the animation sequence.
    unsigned int current_frame = 0;
    //! @brief Frame the sequence starts at, and resets to for ONESHOTRESET animations.
    unsigned int base_frame = 0;
    //! @brief Fixed offset into the sprite sheet for multi-block sprites; see AnimData::m_frame_index_offset.
    size_t frame_index_offset = 0;
    //! @brief Seconds between frame changes.
    float framerate = 0.1f;
    //! @brief Whether the animation is initially advancing.
    bool enabled = true;
    //! @brief Behaviour of the animation once it reaches its last frame.
    AnimType anim_type = AnimType::LOOP;
  };

  //! @brief Construct from a Config, initialising all animation state.
  //! @param cfg Construction parameters.
  explicit AnimData( const Config &cfg )
      : m_current_frame( cfg.current_frame ),
        m_base_frame( cfg.base_frame ),
        m_enabled( cfg.enabled ),
        m_sprite_type( cfg.sprite_type ),
        m_anim_type( cfg.anim_type ),
        m_framerate( cfg.framerate ),
        m_frame_index_offset( cfg.frame_index_offset )
  {
  }

  //! @brief Tracks the frame position in the animation sequence, as an index into the sprite sheet's
  //! `sprite_indices`.
  //! @note It advances by the sheet's `indices_per_frame` each frame. That is 1 for every current sheet, single
  //!       or multi-block, so 0 is the first frame, 1 is the second frame, etc.
  unsigned int m_current_frame;

  //! @brief The start frame in the animation sequence.
  unsigned int m_base_frame;

  //! @brief Elapsed time since the last frame change.
  sf::Time m_elapsed_time{ sf::Time::Zero };

  //! @brief Whether the animation is currently advancing.
  bool m_enabled;

  //! @brief The SpriteSheet object type (string) that we are animating
  Sys::SpriteKey m_sprite_type;

  //! @brief Get the frame index offset for multi-block sprites
  //! @return unsigned int
  unsigned int getFrameIndexOffset() const { return m_frame_index_offset; }

  //! @brief Behaviour of the animation once it reaches its last frame.
  AnimType m_anim_type;

  //! @brief Get the seconds between frame changes.
  //! @return float Framerate in seconds per frame.
  float get_framerate() { return m_framerate; }

  //! @brief Set the seconds between frame changes.
  void set_framerate( float framerate ) { m_framerate = framerate; }

private:
  //! @brief Seconds between frame changes.
  float m_framerate;
  //! @brief Get the relative frame index offset within multi-block sprites
  //! This should be immutable for the lifetime of the component
  //! @return unsigned int
  size_t m_frame_index_offset;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_ANIMDATA_HPP__
