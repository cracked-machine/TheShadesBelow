#ifndef SRC_CMPS_TOXIDROME_TRAITSBASE_HPP__
#define SRC_CMPS_TOXIDROME_TRAITSBASE_HPP__

#include <entt/fwd.hpp>
#include <type_traits>

namespace Game::Cmp::Toxicity
{

// Default: no exclusions.
template <typename T>
struct toxidrome_traits
{
  using excludes = entt::exclude_t<>;
};

// Builds an exclusion list and rejects self-exclusion at compile time.
template <typename Self, typename... Ex>
struct make_excludes
{
  static_assert( ( not std::is_same_v<Self, Ex> && ... ), "A toxidrome cannot exclude itself" );
  using type = entt::exclude_t<Ex...>;
};

// Ready-to-use exclude list value for a given toxidrome.
template <typename T>
inline constexpr typename toxidrome_traits<T>::excludes excluded_by{};

// True if X appears in an exclude_t<Ex...> list.
template <typename X, typename ExcludeList>
struct excludes_type;

template <typename X, typename... Ex>
struct excludes_type<X, entt::exclude_t<Ex...>> : std::disjunction<std::is_same<X, Ex>...>
{
};

template <typename X, typename ExcludeList>
inline constexpr bool excludes_type_v = excludes_type<X, ExcludeList>::value;

// True if A and B are declared as excluding one another, checked in either direction.
template <typename A, typename B>
inline constexpr bool are_mutually_exclusive_v = excludes_type_v<B, typename toxidrome_traits<A>::excludes> ||
                                                 excludes_type_v<A, typename toxidrome_traits<B>::excludes>;

// Rejects, at compile time, any pack containing two mutually exclusive toxidromes.
template <typename... Ts>
struct no_conflicting_toxidromes : std::true_type
{
};

template <typename T, typename... Rest>
struct no_conflicting_toxidromes<T, Rest...>
    : std::conjunction<std::negation<std::disjunction<std::bool_constant<are_mutually_exclusive_v<T, Rest>>...>>, no_conflicting_toxidromes<Rest...>>
{
};

template <typename... Ts>
inline constexpr bool no_conflicting_toxidromes_v = no_conflicting_toxidromes<Ts...>::value;

} // namespace Game::Cmp::Toxicity

#endif // SRC_CMPS_TOXIDROME_TRAITSBASE_HPP__