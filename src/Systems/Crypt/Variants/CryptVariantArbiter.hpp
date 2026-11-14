#ifndef SRC_SYSTEMS_CRYPT_VARIANTS_CRYPTVARIANTARBITER_HPP__
#define SRC_SYSTEMS_CRYPT_VARIANTS_CRYPTVARIANTARBITER_HPP__

#include <Systems/Crypt/Variants/CryptVariant.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Game::Sys::Crypt
{

//! @brief Registry of crypt variants. Picks one (weighted random, or forced by index) when a CryptScene is created.
class CryptVariantArbiter
{
public:
  //! @brief Creates a variant bound to the crypt/passage toolkits
  using Maker = std::function<std::unique_ptr<CryptVariant>( CryptSystem &, PassageSystem & )>;

  //! @brief Register a variant type. Registration order defines the index used by pick()'s override.
  //! @tparam T A CryptVariant subclass constructible from (CryptSystem &, PassageSystem &)
  //! @param name Display/log name
  //! @param weight Relative chance of being picked; 0 = only reachable through the override
  template <typename T>
  void register_variant( std::string name, unsigned weight )
  {
    m_entries.push_back( Entry{ std::move( name ), weight,
                                []( CryptSystem &crypt, PassageSystem &passages ) -> std::unique_ptr<CryptVariant>
                                { return std::make_unique<T>( crypt, passages ); } } );
  }

  //! @brief Create a variant.
  //! @param crypt Room/content toolkit
  //! @param passages Passage toolkit
  //! @param override_idx Registration index to force; out of range (e.g. -1) = weighted random
  //! @return The new variant
  [[nodiscard]] std::unique_ptr<CryptVariant> pick( CryptSystem &crypt, PassageSystem &passages, int override_idx ) const;

  //! @brief Number of registered variants
  [[nodiscard]] std::size_t size() const { return m_entries.size(); }

  //! @brief An arbiter with every built-in variant registered. Add new variants here.
  [[nodiscard]] static CryptVariantArbiter with_default_variants();

private:
  //! @brief One registered variant
  struct Entry
  {
    //! @brief Display/log name
    std::string name;
    //! @brief Relative chance of being picked
    unsigned weight;
    //! @brief Factory
    Maker make;
  };

  //! @brief Registered variants, in registration order
  std::vector<Entry> m_entries;
};

} // namespace Game::Sys::Crypt

#endif // SRC_SYSTEMS_CRYPT_VARIANTS_CRYPTVARIANTARBITER_HPP__
