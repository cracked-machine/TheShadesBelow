#ifndef SRC_CMPS_PLAYER_GRAVETRAIL_HPP__
#define SRC_CMPS_PLAYER_GRAVETRAIL_HPP__

namespace Game::Cmp::Player
{

class GraveTrail
{
public:
  GraveTrail() { m_data.clear(); }
  entt::entity at( size_t idx )
  {
    if ( m_data.empty() ) return entt::null;
    return m_data.at( idx );
  }
  [[nodiscard]] size_t size() const { return m_data.size(); }
  void push_back( entt::entity entt ) { m_data.push_back( entt ); }
  auto begin() { return m_data.begin(); }
  auto end() { return m_data.end(); }

private:
  std::vector<entt::entity> m_data;
};

} // namespace Game::Cmp::Player

#endif // SRC_CMPS_PLAYER_GRAVETRAIL_HPP__