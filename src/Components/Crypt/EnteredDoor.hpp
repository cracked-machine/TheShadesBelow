#ifndef SRC_COMPONENTS_CRYPT_ENTEREDDOOR_HPP__
#define SRC_COMPONENTS_CRYPT_ENTEREDDOOR_HPP__

namespace Game::Cmp::Crypt
{

//! @brief Tags the graveyard crypt door (Cmp::Crypt::Entrance) the player last walked through into the crypt.
//! @note Sys::CryptSystem::lock_entered_crypt_doors() locks the tagged door again when the player returns to the
//! graveyard, then removes the tag.
class EnteredDoor
{
};

} // namespace Game::Cmp::Crypt

#endif // SRC_COMPONENTS_CRYPT_ENTEREDDOOR_HPP__
