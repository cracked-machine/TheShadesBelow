#ifndef SRC_COMPONENTS_ALTAR_KIND_HPP__
#define SRC_COMPONENTS_ALTAR_KIND_HPP__

namespace Game::Cmp::Altar
{

//! @brief Which kind of altar an offering is made at. A sacrifice reward can be restricted to one kind via its
//! "location" in items.json (see Cmp::ActionReward::location).
enum class Kind {
  //! @brief The relic altars in the graveyard.
  GRAVEYARD,
  //! @brief The altar inside the crypt.
  CRYPT,
  //! @brief The altar inside the shop.
  SHOP
};

} // namespace Game::Cmp::Altar

#endif // SRC_COMPONENTS_ALTAR_KIND_HPP__
