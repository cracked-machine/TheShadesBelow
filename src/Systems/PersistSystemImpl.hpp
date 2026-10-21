#ifndef SRC_SYSTEMS_PERSISTSYSTEMIMPL_HPP__
#define SRC_SYSTEMS_PERSISTSYSTEMIMPL_HPP__

#include "PersistSystem.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <typeinfo>

namespace Game::Sys
{

//! @copydoc PersistSystem::add(entt::registry&)
template <typename T>
T &PersistSystem::add( entt::registry &reg )
{
  if ( not reg.ctx().contains<T>() ) { reg.ctx().emplace<T>(); }
  return reg.ctx().get<T>();
}

//! @copydoc PersistSystem::add(entt::registry&,Args&&...)
template <typename T, typename... Args>
T &PersistSystem::add( entt::registry &reg, Args &&...args )
{
  if ( not reg.ctx().contains<T>() ) { reg.ctx().emplace<T>( std::forward<Args>( args )... ); }
  return reg.ctx().get<T>();
}

//! @copydoc PersistSystem::get(entt::registry&)
template <typename T>
T &PersistSystem::get( entt::registry &reg )
{
  if ( not reg.ctx().contains<T>() )
  {
    SPDLOG_CRITICAL( "Attempting to access non-existent persistent component: {}", typeid( T ).name() );
    throw std::runtime_error( "Persistent component not found: " + std::string( typeid( T ).name() ) );
  }
  return reg.ctx().get<T>();
}

//! @copydoc PersistSystem::add_component()
template <typename T>
void PersistSystem::save_component( entt::registry &reg )
{
  auto json_data = load_json_file( kPersistFilePath );
  if ( not json_data )
  {
    SPDLOG_ERROR( "Failed to save {}: could not read {}", T{}.class_name(), kPersistFilePath );
    return;
  }
  ( *json_data )[T{}.class_name()] = get<T>( reg ).serialize();
  write_json_file( kPersistFilePath, *json_data );
}

template <typename T>
void PersistSystem::add_component()
{
  const std::string name = T{}.class_name();
  m_components[name].initialise = [this, name]( const nlohmann::json &json )
  {
    auto &cmp = Sys::PersistSystem::add<T>( reg() );
    cmp.deserialize( json );

    m_components[name].deserialiser = [this]( const nlohmann::json &j ) { Sys::PersistSystem::get<T>( reg() ).deserialize( j ); };
    m_components[name].serialiser = [this]() -> nlohmann::json { return Sys::PersistSystem::get<T>( reg() ).serialize(); };
  };
}

} // namespace Game::Sys

#endif // SRC_SYSTEMS_PERSISTSYSTEMIMPL_HPP__
