
#ifndef SRC_CMPS_TOXIDROME_BRADYCARDIA_HPP__
#define SRC_CMPS_TOXIDROME_BRADYCARDIA_HPP__

#include <Components/Toxicity/TraitsBase.hpp>

namespace Game::Cmp::Toxicity
{

struct Bradycardia
{
};

struct Tachycardia;

template <>
struct toxidrome_traits<Bradycardia>
{
  using excludes = make_excludes<Bradycardia, Tachycardia>::type;
};

}; // namespace Game::Cmp::Toxicity

#endif // SRC_CMPS_TOXIDROME_BRADYCARDIA_HPP__