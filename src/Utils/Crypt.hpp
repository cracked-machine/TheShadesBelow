#ifndef SRC_UTILS_CRYPT_HPP__
#define SRC_UTILS_CRYPT_HPP__

// clang-format off
namespace Game::Cmp::Crypt { class ShuffleTimer; }
// clang-format on

namespace Game::Utils::Crypt
{

//! @brief Reset every Cmp::Crypt::ShuffleTimer's elapsed time back to zero.
//! @param reg reference to the entt registry
void reset_crypt_shuffle_timer( entt::registry &reg );

//! @brief Advance every Cmp::Crypt::ShuffleTimer's elapsed time by `dt`.
//! @param reg reference to the entt registry
//! @param dt Time elapsed since the last update.
void update_crypt_shuffle_timer( entt::registry &reg, sf::Time dt );

//! @brief Check whether every Cmp::Crypt::ShuffleTimer in the registry has reached its timeout.
//! @param reg reference to the entt registry
//! @return bool true if there are no timers, or all timers' elapsed time has reached their timeout
bool is_crypt_shuffle_timer_expired( entt::registry &reg );

//! @brief Get the crypt's shuffle timer component.
//! @return Cmp::Crypt::ShuffleTimer& Reference to the shuffle timer.
Cmp::Crypt::ShuffleTimer &get_shuffle_timer();

} // namespace Game::Utils::Crypt

#endif // SRC_UTILS_CRYPT_HPP__
