#ifndef SRC_CMPS_TOXIDROME_VERTIGO_HPP__
#define SRC_CMPS_TOXIDROME_VERTIGO_HPP__

#include <Components/Toxicity/TraitsBase.hpp>

namespace Game::Cmp::Toxicity
{

struct Vertigo
{
};

template <>
struct toxidrome_traits<Vertigo>
{
  using excludes = make_excludes<Vertigo>::type;
};

}; // namespace Game::Cmp::Toxicity

#endif // SRC_CMPS_TOXIDROME_VERTIGO_HPP__
