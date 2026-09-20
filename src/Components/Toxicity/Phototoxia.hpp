
#ifndef SRC_CMPS_TOXIDROME_PHOTTOXIA_HPP__
#define SRC_CMPS_TOXIDROME_PHOTTOXIA_HPP__

#include <Components/Toxicity/TraitsBase.hpp>

namespace Game::Cmp::Toxicity
{

struct Phototoxia
{
};

template <>
struct toxidrome_traits<Phototoxia>
{
  using excludes = make_excludes<Phototoxia>::type;
};

}; // namespace Game::Cmp::Toxicity

#endif // SRC_CMPS_TOXIDROME_PHOTTOXIA_HPP__