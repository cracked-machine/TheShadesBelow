#ifndef SRC_COMPONENTS_FRACTALCURVE_HPP__
#define SRC_COMPONENTS_FRACTALCURVE_HPP__

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/System/Time.hpp>

#include <vector>

namespace Game::Cmp
{

//! @brief A pair of vertices that are continually split into more pairs of vertices
//!        with a perpendicular offset at each vertex.
struct FractalCurve
{
  //! @brief Max random perpendicular offset (degrees) applied when subdividing a segment
  struct AngleDeviations
  {
    //! @brief Deviation applied to the first (main-line) vertex of each subdivided segment
    float inner;
    //! @brief Deviation applied to the remaining forked/branch vertices of each subdivided segment
    float outer;
  } m_deviations;

  //! @brief Construct a curve as a single segment between two endpoints; call the subdivision
  //!        logic externally to generate further generations of `sequence`.
  //! @param start World-space start point of the curve.
  //! @param end World-space end point of the curve.
  //! @param deviations Max perpendicular angle deviations applied when subdividing.
  //! @param duration Lifetime of the curve; destruction is managed externally once `timer` exceeds it.
  FractalCurve( sf::Vector2f start, sf::Vector2f end, AngleDeviations deviations, sf::Time duration )
  {
    sequence.push_back( { sf::Vertex( start ) } );
    sequence.push_back( { sf::Vertex( end ) } );
    m_deviations = deviations;
    this->duration = duration;
    timer.stop();
  }

  //! @brief Rows of vertices produced by successive subdivisions; each row is one generation of split points
  std::vector<std::vector<sf::Vertex>> sequence;

  //! @brief Colour the curve is rendered with
  sf::Color color;

  //! @brief Lifetime. Destruction logic should be managed externally.
  sf::Time duration{ sf::Time::Zero };

  //! @brief Tracks elapsed time since the curve was created, checked against `duration`
  sf::Clock timer;

  //! @brief A single world-space line between two vertices of `sequence`
  struct Segment
  {
    sf::Vector2f start;
    sf::Vector2f end;
    //! @brief True if this segment is part of the main line (zero-index), false for aux/branch lines
    bool is_main;
  };

  //! @brief Flatten `sequence` into world-space line segments by connecting each row to the next.
  //!        Main line is index zero. Aux lines converge back to / diverge out from the main line.
  [[nodiscard]] std::vector<Segment> segments() const
  {
    std::vector<Segment> result;
    for ( std::size_t row = 0; row + 1 < sequence.size(); ++row )
    {
      const auto &curr_row = sequence[row];
      const auto &next_row = sequence[row + 1];

      // next row's convergence point - all vertices in the current row connect to this
      const sf::Vector2f converge_pos = next_row.at( 0 ).position;
      const sf::Vector2f main_pos = curr_row.at( 0 ).position;

      // always draw main line on zero-index
      result.push_back( { main_pos, converge_pos, true } );

      // always converge non-zero index vertex back to the main line (zero-index)
      for ( std::size_t idx = 1; idx < curr_row.size(); ++idx )
      {
        result.push_back( { curr_row[idx].position, converge_pos, false } );
      }

      // always diverge zero-index vertex out to available non-zero index vertex on next row
      for ( std::size_t idx = 1; idx < next_row.size(); ++idx )
      {
        result.push_back( { main_pos, next_row[idx].position, false } );
      }
    }
    return result;
  }

  sf::Color m_main_strike_line_color = sf::Color( 0, 255, 255, 255 );
  sf::Color m_aux_strike_line_color = sf::Color( 255, 255, 255, 255 );
  float m_main_line_thickness = 10.f;
  float m_aux_line_thickness = 3.f;
};

//! @brief Type of FractalCurve used for LightningStrikes
struct LightningStrike : public FractalCurve
{
  //! @brief Construct a lightning strike curve between two endpoints.
  //! @param start World-space start point (typically the strike origin).
  //! @param end World-space end point (typically the strike target).
  //! @param deviations Max perpendicular angle deviations applied when subdividing.
  //! @param duration Lifetime of the strike before it should be destroyed.
  LightningStrike( sf::Vector2f start, sf::Vector2f end, AngleDeviations deviations, sf::Time duration )
      : FractalCurve( start, end, deviations, duration )
  {
    m_main_strike_line_color = sf::Color( 0, 255, 255, 255 );
    m_aux_strike_line_color = sf::Color( 255, 255, 255, 255 );
    m_main_line_thickness = 10.f;
    m_aux_line_thickness = 3.f;
  }
};

//! @brief Type of FractalCurve used for ObstacleCracks
struct ObstacleCrack : public FractalCurve
{
  //! @brief Construct an obstacle crack curve between two endpoints.
  //! @param start World-space start point of the crack.
  //! @param end World-space end point of the crack.
  //! @param deviations Max perpendicular angle deviations applied when subdividing.
  //! @param duration Lifetime of the crack before it should be destroyed.
  ObstacleCrack( sf::Vector2f start, sf::Vector2f end, AngleDeviations deviations, sf::Time duration )
      : FractalCurve( start, end, deviations, duration )
  {
    m_aux_strike_line_color = sf::Color( 0, 0, 0, 255 );
    m_main_strike_line_color = sf::Color( 0, 0, 0, 255 );
    m_main_line_thickness = 3.f;
    m_aux_line_thickness = 2.f;
  }
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_FRACTALCURVE_HPP__
