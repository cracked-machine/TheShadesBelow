#ifndef SRC_CMPS_TOXIDROME_TACHYCARDIA_HPP__
#define SRC_CMPS_TOXIDROME_TACHYCARDIA_HPP__

#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/TraitsBase.hpp>

namespace Game::Cmp::Toxicity
{
//! @brief Tag component
struct Tachycardia
{
};

struct Bradycardia;

//! @brief Exclusion rules
//! @tparam
template <>
struct toxidrome_traits<Tachycardia> : make_excludes<Tachycardia, Bradycardia>
{
  using excludes = make_excludes<Tachycardia, Bradycardia>::type;
};

} // namespace Game::Cmp::Toxicity

#endif // SRC_CMPS_TOXIDROME_TACHYCARDIA_HPP__
