
#include <Components/Crypt/RoomOpen.hpp>
#include <Components/Direction.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Wall.hpp>
#include <Systems/ProcGen/PassageAlgorithms.hpp>
#include <Utils/Cardinal.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>
#include <array>

namespace Game::Sys::ProcGen
{

std::optional<Cmp::Crypt::PassageBlock> PassageAlogirthms::place_passage_block( entt::registry &reg, float x, float y,
                                                                                AllowDuplicatePassages duplicates_policy, bool skip_wall_check )
{
  Cmp::Position candidate_passage_block_pos_cmp( Utils::snap_to_grid( { x, y }, Utils::Rounding::TOWARDS_ZERO ), Constants::kGridSizePxF );

  // Check if a block already exists at this position
  auto block_view = reg.view<Cmp::Crypt::PassageBlock>();
  for ( auto [passage_block_entt, passage_block_cmp] : block_view.each() )
  {
    Cmp::Position existing_passageblock_pos_cmp( passage_block_cmp, Constants::kGridSizePxF );
    if ( ( existing_passageblock_pos_cmp.findIntersection( candidate_passage_block_pos_cmp ) ) and
         ( duplicates_policy == AllowDuplicatePassages::NO ) )
    {
      // SPDLOG_INFO( "PassageBlock already exists at {},{}", x, y );
      return std::nullopt;
    }
  }

  // Allow placement in open rooms (passages connect to rooms)
  // Only block placement if there's a wall collision

  if ( !skip_wall_check )
  {
    for ( auto &wall_rects : m_cached_wall_components )
    {
      if ( wall_rects.findIntersection( candidate_passage_block_pos_cmp ) )
      {
        SPDLOG_DEBUG( "Wall collision: Cannot place PassageBlock at {},{}", x, y );
        return std::nullopt;
      }
    }
  }

  SPDLOG_DEBUG( "Placed PassageBlock at {},{} (id:{})", x, y, m_current_passage_id );

  return Cmp::Crypt::PassageBlock( candidate_passage_block_pos_cmp.position, m_current_passage_id );
};

std::vector<Cmp::Crypt::PassageBlock> PassageAlogirthms::create_drunken_walk( entt::registry &reg, Cmp::Crypt::PassageDoor start,
                                                                              sf::FloatRect end_bounds, sf::Vector2f map_size_pixel,

                                                                              const std::set<entt::entity> &exclude_entts,
                                                                              AllowDuplicatePassages duplicates_policy )
{

  //! @brief Prevent infinite walks
  static const int kMaxStepsPerWalk{ 300 };

  //! @brief Prevent drunken walk from getting stuck
  static const int kMaxAttemptsPerStep{ 200 };

  //! @brief Force drunken walk to initially stay in orthogonal direction e.g. north, east, west, south from starting point
  static const int kMinInitialOrthogonalSteps{ 3 };

  //! @brief Minimum passage-to-passage distance
  static const float kMinBlockDistanceBetweenPassages{ Constants::kGridSizePxF.x };

  //! @brief Minimum passage-to-room distance: scale factor of 16x16 block
  static const float kMinPassageRoomsDistanceScaleFactor{ 2.f };

  //! @brief Number of steps to delay 'kMinPassageRoomsDistanceScaleFactor' use
  static const int kMinPassageRoomsDistanceDelay{ 3 };

  //! @brief direction vector quick lookup for passage creation
  static std::unordered_map<Cmp::Crypt::CryptPassageDirection, Cmp::Direction> kDirectionDictionary = {
      { Cmp::Crypt::CryptPassageDirection::NORTH, Cmp::Direction( { 0.f, -1.f } ) },
      { Cmp::Crypt::CryptPassageDirection::EAST, Cmp::Direction( { 1.f, 0.f } ) },
      { Cmp::Crypt::CryptPassageDirection::SOUTH, Cmp::Direction( { 0.f, 1.f } ) },
      { Cmp::Crypt::CryptPassageDirection::WEST, Cmp::Direction( { -1.f, 0.f } ) } };

  //! @brief Drunken walk roulette picker for direction
  //! @note undefined odds are used to select a random direction
  Cmp::RandomInt direction_picker{ 0, 99 };
  Cmp::RandomInt random_cardinal_picker{ 0, 3 };
  //! @brief Drunken walk roulette odds for moving towards target: 60%
  static const float kRouletteTargetBiasOdds = 0.6f;
  //! @brief Drunken walk roulette odds for continuing in the same direction
  static const float kRouletteSameDirectionOdds = 0.1f;
  static const int kRouletteTargetBiasThreshold = static_cast<int>( kRouletteTargetBiasOdds * 100 );
  static const int kRouletteSameDirectionThreshold = static_cast<int>( ( kRouletteTargetBiasOdds + kRouletteSameDirectionOdds ) * 100 );
  static const int kRecentPositionAttemptsThreshold = static_cast<int>( kMaxAttemptsPerStep * 0.7f );

  int walk_step_count = 0;

  const float kMinSpacing = Constants::kGridSizePxF.x * 2.0f;
  float min_x = std::max( 0.f, std::min( start.x, end_bounds.position.x ) - kMinSpacing );
  float min_y = std::max( 0.f, std::min( start.y, end_bounds.position.y ) - kMinSpacing );
  float max_x = std::min( map_size_pixel.x,
                          std::max( start.x + Constants::kGridSizePxF.x, end_bounds.position.x + end_bounds.size.x ) + kMinSpacing );
  float max_y = std::min( map_size_pixel.y,
                          std::max( start.y + Constants::kGridSizePxF.y, end_bounds.position.y + end_bounds.size.y ) + kMinSpacing );
  sf::FloatRect walk_bounds( sf::Vector2f( min_x, min_y ), sf::Vector2f( max_x - min_x, max_y - min_y ) );
  const float walk_right = max_x - Constants::kGridSizePxF.x;
  const float walk_bottom = max_y - Constants::kGridSizePxF.y;

  // cache open room rects once before the walk
  std::vector<sf::FloatRect> open_room_rects;
  open_room_rects.reserve( 32 );
  for ( auto [open_room_entt, open_room_cmp] : reg.view<Cmp::Crypt::RoomOpen>().each() )
  {
    if ( exclude_entts.contains( open_room_entt ) ) continue;
    open_room_rects.emplace_back( open_room_cmp.position, open_room_cmp.size );
  }

  // cache existing passage block positions once before the walk
  // NOTE: new blocks added during the walk use m_current_passage_id so we only need OTHER passages cached
  std::vector<sf::Vector2f> other_passage_block_positions;
  other_passage_block_positions.reserve( 256 );
  for ( auto [block_entt, block_cmp] : reg.view<Cmp::Crypt::PassageBlock>().each() )
  {
    if ( block_cmp.m_passage_id != m_current_passage_id ) other_passage_block_positions.emplace_back( block_cmp.x, block_cmp.y );
  }

  std::vector<Cmp::Crypt::PassageBlock> passage_block_list;
  passage_block_list.reserve( kMaxStepsPerWalk );

  auto maybe_passage_block = place_passage_block( reg, start.x, start.y, duplicates_policy, true );
  if ( maybe_passage_block.has_value() ) { passage_block_list.push_back( maybe_passage_block.value() ); }

  sf::FloatRect current_pos( { start.x, start.y }, Constants::kGridSizePxF );
  sf::Vector2f last_move_direction( 0.f, 0.f );

  std::array<sf::Vector2f, 5> recent_positions{};
  size_t recent_write_idx = 0;
  size_t recent_count = 0;
  static constexpr size_t max_recent_positions = 5;

  while ( not current_pos.findIntersection( end_bounds ) && walk_step_count < kMaxStepsPerWalk )
  {
    sf::FloatRect candidate_pos( { -16.f, -16.f }, Constants::kGridSizePxF );
    int step_attempts = 0;
    bool is_candidate_rejected = false;
    bool found_valid_candidate = false;
    sf::Vector2f target_distance = end_bounds.getCenter() - current_pos.getCenter();
    sf::Vector2f chosen_direction;

    do
    {
      int random_choice = direction_picker.gen();

      if ( walk_step_count < kMinInitialOrthogonalSteps ) { chosen_direction = kDirectionDictionary[start.m_direction]; }
      else
      {
        if ( random_choice < kRouletteTargetBiasThreshold )
        {
          if ( std::abs( target_distance.x ) > std::abs( target_distance.y ) )
            chosen_direction = ( target_distance.x > 0 ) ? sf::Vector2f( 1.f, 0.f ) : sf::Vector2f( -1.f, 0.f );
          else
            chosen_direction = ( target_distance.y > 0 ) ? sf::Vector2f( 0.f, 1.f ) : sf::Vector2f( 0.f, -1.f );
        }
        else if ( random_choice < kRouletteSameDirectionThreshold && ( last_move_direction.x != 0.f || last_move_direction.y != 0.f ) )
        {
          chosen_direction = last_move_direction;
        }
        else { chosen_direction = Utils::Cardinal( static_cast<Utils::Cardinal::Value>( random_cardinal_picker.gen() ) ).vector(); }
      }

      auto chosen_magnitude = chosen_direction.componentWiseMul( Constants::kGridSizePxF );
      candidate_pos.position.x = current_pos.position.x + chosen_magnitude.x;
      candidate_pos.position.y = current_pos.position.y + chosen_magnitude.y;

      if ( candidate_pos.position.x < min_x || candidate_pos.position.x > walk_right || candidate_pos.position.y < min_y ||
           candidate_pos.position.y > walk_bottom )
      {
        step_attempts++;
        continue;
      }

      bool is_recent = false;
      for ( size_t i = 0; i < recent_count; ++i )
      {
        if ( std::abs( recent_positions[i].x - candidate_pos.position.x ) < Constants::kGridSizePxF.x * 0.5f &&
             std::abs( recent_positions[i].y - candidate_pos.position.y ) < Constants::kGridSizePxF.y * 0.5f )
        {
          is_recent = true;
          break;
        }
      }
      if ( is_recent && step_attempts < kRecentPositionAttemptsThreshold )
      {
        step_attempts++;
        continue;
      }

      is_candidate_rejected = false;

      if ( duplicates_policy == AllowDuplicatePassages::NO )
      {
        // use cached other_passage_block_positions instead of registry view
        static const float kMinBlockDistanceSq = kMinBlockDistanceBetweenPassages * kMinBlockDistanceBetweenPassages;
        for ( const auto &block_pos : other_passage_block_positions )
        {
          float dx = candidate_pos.position.x - block_pos.x;
          float dy = candidate_pos.position.y - block_pos.y;
          if ( dx * dx + dy * dy < kMinBlockDistanceSq )
          {
            is_candidate_rejected = true;
            break;
          }
        }

        // use cached open_room_rects instead of registry view
        if ( !is_candidate_rejected && walk_step_count > kMinPassageRoomsDistanceDelay )
        {
          Cmp::RectBounds scaled_candidate_pos( candidate_pos.position, candidate_pos.size, kMinPassageRoomsDistanceScaleFactor );
          for ( const auto &room_rect : open_room_rects )
          {
            if ( scaled_candidate_pos.findIntersection( room_rect ) )
            {
              is_candidate_rejected = true;
              break;
            }
          }
        }
      }

      // use cached wall_rects instead of registry view
      if ( !is_candidate_rejected )
      {
        for ( const auto &wall_rect : m_cached_wall_components )
        {
          if ( candidate_pos.findIntersection( wall_rect ) )
          {
            is_candidate_rejected = true;
            break;
          }
        }
      }

      if ( !is_candidate_rejected )
      {
        found_valid_candidate = true;
        break;
      }
      step_attempts++;

    } while ( step_attempts < kMaxAttemptsPerStep );

    if ( !found_valid_candidate )
    {
      SPDLOG_DEBUG( "createDrunkenWalkPassage (id:{}): No valid candidate found after {} attempts, terminating walk", m_current_passage_id,
                    kMaxAttemptsPerStep );
      return {};
    }

    current_pos.position = candidate_pos.position;
    last_move_direction = chosen_direction;

    recent_positions[recent_write_idx] = current_pos.position;
    recent_write_idx = ( recent_write_idx + 1 ) % max_recent_positions;
    if ( recent_count < max_recent_positions ) recent_count++;

    maybe_passage_block = place_passage_block( reg, current_pos.position.x, current_pos.position.y, duplicates_policy, true );
    if ( maybe_passage_block.has_value() ) { passage_block_list.push_back( maybe_passage_block.value() ); }

    walk_step_count++;
  }

  if ( walk_step_count >= kMaxStepsPerWalk )
  {
    SPDLOG_WARN( "createDrunkenWalkPassage (id:{}): Maximum walk length reached, terminating", m_current_passage_id );
    return {};
  }

  return passage_block_list;
}

std::vector<Cmp::Crypt::PassageBlock> PassageAlogirthms::create_dog_leg( entt::registry &reg, Cmp::Crypt::PassageDoor start, sf::FloatRect end_bounds,
                                                                         AllowDuplicatePassages duplicates_policy )
{

  std::vector<Cmp::Crypt::PassageBlock> passage_block_list;

  SPDLOG_DEBUG( "createDogLegPassage (id:{}) from ({},{}) to ({},{})", m_current_passage_id, start.x, start.y, end_bounds.getCenter().x,
                end_bounds.getCenter().y );

  const auto kSquareSizePx = Constants::kGridSizePxF;

  // always prioritize nearer direction
  if ( start.m_direction == Cmp::Crypt::CryptPassageDirection::EAST or start.m_direction == Cmp::Crypt::CryptPassageDirection::WEST )
  {
    SPDLOG_DEBUG( "First leg: horizontal" );
    // try to prevent hitting another open room before reaching the target room
    float furthest_pos_x;
    if ( start.m_direction == Cmp::Crypt::CryptPassageDirection::EAST )
    {
      furthest_pos_x = end_bounds.position.x + end_bounds.size.x;
      for ( float x = start.x; x <= furthest_pos_x; x += kSquareSizePx.x )
      {
        auto maybe_passage_block = place_passage_block( reg, x, start.y, duplicates_policy );
        if ( maybe_passage_block.has_value() ) { passage_block_list.push_back( maybe_passage_block.value() ); }
      }
    }
    else if ( start.m_direction == Cmp::Crypt::CryptPassageDirection::WEST )
    {
      furthest_pos_x = end_bounds.position.x;
      for ( float x = start.x; x >= furthest_pos_x; x -= kSquareSizePx.x )
      {
        auto maybe_passage_block = place_passage_block( reg, x, start.y, duplicates_policy );
        if ( maybe_passage_block.has_value() ) { passage_block_list.push_back( maybe_passage_block.value() ); }
      }
    }
    SPDLOG_DEBUG( "Second leg: vertical from end.x to end.y" );
    float dy = end_bounds.getCenter().y - start.y;
    if ( dy > 0 )
    {
      for ( float y = start.y + kSquareSizePx.y; y <= end_bounds.getCenter().y; y += kSquareSizePx.y )
      {
        auto maybe_passage_block = place_passage_block( reg, furthest_pos_x, y, duplicates_policy );
        if ( maybe_passage_block.has_value() ) { passage_block_list.push_back( maybe_passage_block.value() ); }
      }
    }
    else
    {
      for ( float y = start.y - kSquareSizePx.y; y >= end_bounds.getCenter().y; y -= kSquareSizePx.y )
      {
        auto maybe_passage_block = place_passage_block( reg, furthest_pos_x, y, duplicates_policy );
        if ( maybe_passage_block.has_value() ) { passage_block_list.push_back( maybe_passage_block.value() ); }
      }
    }
  }
  else
  {
    SPDLOG_DEBUG( "First leg: vertical" );
    // try to prevent hitting another open room before reaching the target room
    float furthest_pos_y = 0;
    if ( start.m_direction == Cmp::Crypt::CryptPassageDirection::SOUTH )
    {
      furthest_pos_y = end_bounds.position.y + end_bounds.size.y;
      for ( float y = start.y; y <= furthest_pos_y; y += kSquareSizePx.y )
      {
        auto maybe_passage_block = place_passage_block( reg, start.x, y, duplicates_policy );
        if ( maybe_passage_block.has_value() ) { passage_block_list.push_back( maybe_passage_block.value() ); }
      }
    }
    else if ( start.m_direction == Cmp::Crypt::CryptPassageDirection::NORTH )
    {
      furthest_pos_y = end_bounds.position.y;
      for ( float y = start.y; y >= furthest_pos_y; y -= kSquareSizePx.y )
      {
        auto maybe_passage_block = place_passage_block( reg, start.x, y, duplicates_policy );
        if ( maybe_passage_block.has_value() ) { passage_block_list.push_back( maybe_passage_block.value() ); }
      }
    }
    SPDLOG_DEBUG( "Second leg: horizontal from start.x to end.x" );
    float dx = end_bounds.getCenter().x - start.x;
    if ( dx > 0 )
    {
      for ( float x = start.x + kSquareSizePx.x; x <= end_bounds.getCenter().x; x += kSquareSizePx.x )
      {
        auto maybe_passage_block = place_passage_block( reg, x, furthest_pos_y, duplicates_policy );
        if ( maybe_passage_block.has_value() ) { passage_block_list.push_back( maybe_passage_block.value() ); }
      }
    }
    else
    {
      for ( float x = start.x - kSquareSizePx.x; x >= end_bounds.getCenter().x; x -= kSquareSizePx.x )
      {
        auto maybe_passage_block = place_passage_block( reg, x, furthest_pos_y, duplicates_policy );
        if ( maybe_passage_block.has_value() ) { passage_block_list.push_back( maybe_passage_block.value() ); }
      }
    }
  }

  return passage_block_list;
}

void PassageAlogirthms::cache_wall_components( entt::registry &reg )
{
  for ( auto [wall_entt, wall_cmp, wall_pos_cmp] : reg.view<Cmp::Wall, Cmp::Position>().each() )
  {
    m_cached_wall_components.emplace_back( wall_pos_cmp.position, wall_pos_cmp.size );
  }
}

} // namespace Game::Sys::ProcGen