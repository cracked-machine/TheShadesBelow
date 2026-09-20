
#ifndef SRC_CMPS_TOXIDROME_HYPOXIA_HPP__
#define SRC_CMPS_TOXIDROME_HYPOXIA_HPP__

#include <Components/Toxicity/TraitsBase.hpp>

namespace Game::Cmp::Toxicity
{

struct Hypoxia
{
};

template <>
struct toxidrome_traits<Hypoxia>
{
  using excludes = make_excludes<Hypoxia>::type;
};

}; // namespace Game::Cmp::Toxicity

#endif // SRC_CMPS_TOXIDROME_HYPOXIA_HPP__