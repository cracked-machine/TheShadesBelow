#ifndef SRC_COMPONENTS_DROPCOUNT_HPP__
#define SRC_COMPONENTS_DROPCOUNT_HPP__

namespace Game::Cmp
{

//! @brief How many times a world item's action reward has dropped over the item's lifetime, picked up or not
//! (see Cmp::ActionReward::max_instances).
struct DropCount
{
  //! @brief Number of successful drops so far.
  int m_count{ 0 };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_DROPCOUNT_HPP__
