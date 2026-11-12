#ifndef SRC_COMPONENTS_PERSISTENT_NPCRENDEROFFSET_HPP__
#define SRC_COMPONENTS_PERSISTENT_NPCRENDEROFFSET_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Resting vertical render offset (px, negative is up) of NPC sprites, applied via Cmp::AbsoluteRenderOffset.
//! Lifts NPC sprites so they look centred on their grid cell. Visual only - the NPC position is unaffected.
struct NpcRenderOffset : public BasePersistent<float>
{
  //! @brief Construct with an initial offset and optional min/max bounds.
  //! @param value initial offset
  //! @param min_value minimum allowed offset
  //! @param max_value maximum allowed offset
  NpcRenderOffset( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "NpcRenderOffset"
  virtual std::string class_name() const override { return "NpcRenderOffset"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_NPCRENDEROFFSET_HPP__
