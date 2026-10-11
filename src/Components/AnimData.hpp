#ifndef SRC_COMPONENTS_ANIMDATA_HPP__
#define SRC_COMPONENTS_ANIMDATA_HPP__

#include <SFML/System/Time.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <optional>

namespace Game::Cmp
{

//! @brief How an animation sequence behaves once it reaches its last frame.
enum class AnimType {
  //! @brief Restart the sequence from its base frame and keep animating indefinitely.
  LOOP,
  //! @brief Play once, then deactivate and reset the current frame back to the base frame.
  ONESHOTRESET,
  //! @brief Play once, then deactivate and hold on the last frame reached.
  ONESHOTHOLD
};

//! @brief Component holding sprite-sheet animation state for an entity (current frame, looping behaviour, etc).
//! @note The animation framerate comes from the sprite sheet (`framerate` in res/json/spritesheets.json).
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
        m_frame_index_offset( cfg.frame_index_offset )
  {
  }

  //! @brief Tracks the frame position in the animation sequence, as an index into the sprite sheet's
  //! `sprite_indices`.
  //! @note It advances by 1 each frame for every sheet, single or multi-block, so 0 is the first frame, 1 is
  //!       the second frame, etc.
  unsigned int m_current_frame;

  //! @brief The start frame in the animation sequence.
  unsigned int m_base_frame;

  //! @brief Elapsed time since the last frame change.
  sf::Time m_elapsed_time{ sf::Time::Zero };

  //! @brief Whether the animation is currently advancing.
  bool m_enabled;

  //! @brief The SpriteSheet object type (string) that we are animating
  Sys::SpriteKey m_sprite_type;

  //! @brief Draw the sprite mirrored horizontally, in place. For sprites with a single side-facing sheet.
  bool m_flip_x{ false };

  //! @brief Draw the sprite mirrored vertically, in place.
  bool m_flip_y{ false };

  //! @brief Get the frame index offset for multi-block sprites
  //! @return unsigned int
  [[nodiscard]] unsigned int getFrameIndexOffset() const { return m_frame_index_offset; }

  //! @brief Behaviour of the animation once it reaches its last frame.
  AnimType m_anim_type;

  //! @brief Get the framerate override, if one has been set.
  //! @return std::optional<float> Framerate in frames per second, or empty to use the sprite sheet's framerate.
  [[nodiscard]] std::optional<float> framerate_override() const { return m_framerate_override; }

  //! @brief Replace the sprite sheet's framerate for this entity. Used for movement-driven animation.
  //! @param framerate Frames per second. 0 = not animated.
  void set_framerate_override( float framerate ) { m_framerate_override = framerate; }

private:
  //! @brief Replaces the sprite sheet's framerate when set, in frames per second.
  std::optional<float> m_framerate_override;
  //! @brief Get the relative frame index offset within multi-block sprites
  //! This should be immutable for the lifetime of the component
  //! @return unsigned int
  size_t m_frame_index_offset;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_ANIMDATA_HPP__
