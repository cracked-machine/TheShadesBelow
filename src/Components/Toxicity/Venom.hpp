
#ifndef SRC_CMPS_TOXIDROME_VENOM_HPP__
#define SRC_CMPS_TOXIDROME_VENOM_HPP__

#include <Components/Toxicity/TraitsBase.hpp>

namespace Game::Cmp::Toxicity
{

struct Venom
{
};

template <>
struct toxidrome_traits<Venom>
{
  using excludes = make_excludes<Venom>::type;
};

}; // namespace Game::Cmp::Toxicity

#endif // SRC_CMPS_TOXIDROME_VENOM_HPP__