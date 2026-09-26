#include <Shaders/IShaderSprite.hpp>
#include <Systems/ShaderSystem.hpp>

namespace Game::Sys
{

ShaderSystem::ShaderSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sprite_factory, sound_bank )
{
}

std::vector<entt::entity> ShaderSystem::add_to_registry( Cmp::Shader::SpriteOwner owner, Cmp::ZOrderValue zorder )
{
  std::vector<entt::entity> entt_list;

  auto entt = reg().create();
  reg().emplace<Cmp::Shader::SpriteOwner>( entt, std::move( owner ) );
  reg().emplace<Cmp::ZOrderValue>( entt, zorder );
  entt_list.push_back( entt );
  SPDLOG_DEBUG( "Created ShaderSprite {}", static_cast<uint32_t>( entt ) );

  return entt_list;
}

void ShaderSystem::update( sf::Time dt )
{
  for ( auto [entt, owner] : reg().view<Cmp::Shader::SpriteOwner>().each() )
  {
    owner.sprite->update( reg(), dt );
  }
}

[[nodiscard]] Sprites::IShaderSprite *ShaderSystem::find( entt::registry &reg, const std::string &tag )
{
  for ( auto [entt, owner] : reg.view<Cmp::Shader::SpriteOwner>().each() )
  {
    if ( owner.sprite->get_tag() == tag ) return owner.sprite.get();
  }
  return nullptr;
}

} // namespace Game::Sys