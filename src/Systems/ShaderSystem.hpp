#ifndef SRC_SYSTEMS_SHADERSYSTEM_HPP__
#define SRC_SYSTEMS_SHADERSYSTEM_HPP__

#include <Components/Shader/SpriteOwner.hpp>
#include <Components/ZOrderValue.hpp>
#include <Shaders/IShaderSprite.hpp>
#include <Systems/BaseSystem.hpp>

#include <functional>
#include <optional>
#include <string>
#include <type_traits>

namespace Game::Sys
{

//! @brief Core system for adding and updating IShaderSprite objects
class ShaderSystem : public BaseSystem
{
public:
  //! @brief Construct a new Shader System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  ShaderSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Wrap and add a shader sprite to the registry.
  //! @param shader
  //! @param z_order
  void add( std::unique_ptr<Sprites::IShaderSprite> shader, Cmp::ZOrderValue z_order )
  {
    add_to_registry( Cmp::Shader::SpriteOwner( std::move( shader ) ), z_order );
  }

  //! @brief Calls IShaderSprite::update() function within all added ShaderSpriteBase<T>
  //! Skipped while the player has a Cmp::Player::CameraHold, so the effects stay in step with the held camera.
  void update( sf::Time dt );

  //! @brief Find a shader sprite by tag.
  //! @tparam T The shader type to return, e.g. Sprites::ScreenShudderShader. Defaults to the Sprites::IShaderSprite
  //! interface; a more derived type is checked with dynamic_cast.
  //! @param reg
  //! @param tag
  //! @return A reference to the first shader with this tag, or std::nullopt if none is registered or it is not a T.
  //! @code
  //! if ( auto shudder = Sys::ShaderSystem::find<Sprites::ScreenShudderShader>( reg(), "ScreenShudder" ) )
  //!   shudder->get().trigger( sf::seconds( 2.5f ) );
  //! @endcode
  template <typename T = Sprites::IShaderSprite>
  [[nodiscard]] static std::optional<std::reference_wrapper<T>> find( entt::registry &reg, const std::string &tag )
  {
    for ( auto [entt, owner] : reg.view<Cmp::Shader::SpriteOwner>().each() )
    {
      if ( not owner.sprite or owner.sprite->get_tag() != tag ) continue;
      if constexpr ( std::is_same_v<T, Sprites::IShaderSprite> ) { return std::ref( *owner.sprite ); }
      else
      {
        if ( auto *typed = dynamic_cast<T *>( owner.sprite.get() ) ) return std::ref( *typed );
        return std::nullopt;
      }
    }
    return std::nullopt;
  }

private:
  //! @brief Add a wrapped shader sprite and zorder component to the registry
  //! @param owner
  //! @param zorder
  //! @return std::vector<entt::entity>
  std::vector<entt::entity> add_to_registry( Cmp::Shader::SpriteOwner owner, Cmp::ZOrderValue zorder );
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_SHADERSYSTEM_HPP__
