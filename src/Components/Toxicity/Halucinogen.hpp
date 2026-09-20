
#ifndef SRC_CMPS_TOXIDROME_HALUCINOGEN_HPP__
#define SRC_CMPS_TOXIDROME_HALUCINOGEN_HPP__

#include <Components/Toxicity/TraitsBase.hpp>

namespace Game::Cmp::Toxicity
{

struct Hallucinogen
{
};

template <>
struct toxidrome_traits<Hallucinogen>
{
  using excludes = make_excludes<Hallucinogen>::type;
};

}; // namespace Game::Cmp::Toxicity

#endif // SRC_CMPS_TOXIDROME_HALUCINOGEN_HPP__