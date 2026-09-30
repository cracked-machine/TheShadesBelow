#ifndef SRC_SYSTEMS_RENDER_RENDERPASSTYPES_HPP__
#define SRC_SYSTEMS_RENDER_RENDERPASSTYPES_HPP__

#include <Components/Altar/MultiBlock.hpp>
#include <Components/Crypt/BuildingMultiBlock.hpp>
#include <Components/Crypt/InteriorMultiBlock.hpp>
#include <Components/Grave/ExitMultiBlock.hpp>
#include <Components/Grave/MultiBlock.hpp>
#include <Components/Moveable.hpp>
#include <Components/NoRender.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/ObstacleCap.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Ruin/BuildingMultiBlock.hpp>
#include <Components/Spring/HealingSpringBuildingMultiBlock.hpp>
#include <Components/Weapons/Arrow.hpp>
#include <Components/Wormhole/MultiBlock.hpp>

#include <entt/core/type_traits.hpp>
#include <entt/entity/fwd.hpp>

namespace Game::Sys::RenderPass
{

//! @brief Multiblock root components. Each gets its own z-order pass that visibility-tests the whole
//! multiblock rect (preventing pop-in at the view edge), so every other Cmp::Position pass must exclude them
//! or the root is queued, and drawn, twice.
using MultiBlockRoots = entt::type_list<Cmp::Altar::MultiBlock, Cmp::Crypt::BuildingMultiBlock, Cmp::Grave::MultiBlock,
                                        Cmp::HealingSpringBuildingMultiBlock, Cmp::Crypt::InteriorMultiBlock,
                                        Cmp::Ruin::BuildingMultiBlock, Cmp::Grave::ExitMultiBlock, Cmp::Wormhole::MultiBlock>;

//! @brief Renderable entities that move after creation, so they are never indexed in the static render position
//! grid and must be scanned every frame instead.
using Movers = entt::type_list<Cmp::Player::Character, Cmp::Npc::NPC, Cmp::Weapons::Projectiles::Arrow, Cmp::Moveable, Cmp::ObstacleCap>;

namespace detail
{
template <typename>
struct ExcludeOf;

template <typename... Ts>
struct ExcludeOf<entt::type_list<Ts...>>
{
  static constexpr entt::exclude_t<Ts...> value{};
};
} // namespace detail

//! @brief entt::exclude of Cmp::NoRender plus every type in the given type lists
template <typename... Lists>
inline constexpr auto exclude = detail::ExcludeOf<entt::type_list_cat_t<entt::type_list<Cmp::NoRender>, Lists...>>::value;

} // namespace Game::Sys::RenderPass

#endif // SRC_SYSTEMS_RENDER_RENDERPASSTYPES_HPP__
