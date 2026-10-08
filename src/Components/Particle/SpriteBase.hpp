#ifndef SRC_COMPONENTS_PARTICLE_SPRITEBASE_HPP__
#define SRC_COMPONENTS_PARTICLE_SPRITEBASE_HPP__

#include <Components/Particle/Concepts.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <random>

namespace Game::Cmp::Particle
{

//! @brief Distinguishes whether a particle sprite is rendered in world space or screen space.
enum class ViewType {
  //! @brief Rendered directly in screen/pixel space (e.g. UI/inventory), unaffected by the camera.
  SCREEN,
  //! @brief Rendered in world space and transformed through the camera view.
  WORLD
};

//! @brief Preset scale for particle sprites rendered in the UI/inventory view (see IParticleSprite::set_scale()).
//! @note Defined in SpriteBase.cpp (not inline/constexpr here) so tuning the value only recompiles that one file,
//        not everything that includes this header.
extern const float kUiScalePreset;
//! @brief Preset scale for particle sprites rendered in the game world view (see IParticleSprite::set_scale()).
//! @note Defined in SpriteBase.cpp (not inline/constexpr here) so tuning the value only recompiles that one file,
//        not everything that includes this header.
extern const float kWorldScalePreset;

// ============================================================
// IParticle — individual particle contract
// ============================================================

//! @brief Interface that all particle types must implement
class IParticle
{
public:
  virtual ~IParticle() = default;

  //! @brief API for emitting the particle. Should be called by `simulate()` function of derived class.
  //! @note Resets the particle's position and rolls a new lifetime, then dispatches to the private
  //!       emit()/idle() implementation depending on whether the particle is active (NVI idiom).
  virtual void do_emit() = 0;

  //! @brief Get the number of times this particle has been re-emitted since the sprite was (re)started.
  //! @return size_t The current generation count.
  virtual size_t generations() = 0;

  //! @brief Set the range particle speed is randomly drawn from on each emit.
  //! @param speed Uniform distribution to sample particle speed from.
  virtual void set_speed( std::uniform_real_distribution<float> speed ) = 0;
  //! @brief Set the range particle launch angle is randomly drawn from on each emit.
  //! @param angle Uniform distribution (degrees) to sample particle angle from.
  virtual void set_angle( std::uniform_real_distribution<float> angle ) = 0;
  //! @brief Set the range particle wave phase is randomly drawn from on each emit.
  //! @param phase Uniform distribution to sample particle phase from.
  virtual void set_phase( std::uniform_real_distribution<float> phase ) = 0;
  //! @brief Set the range particle wave frequency is randomly drawn from on each emit.
  //! @param freq Uniform distribution to sample particle frequency from.
  virtual void set_freq( std::uniform_real_distribution<float> freq ) = 0;
  //! @brief Set the range particle lifetime (ms) is randomly drawn from on each emit.
  //! @param lifetime Uniform distribution to sample particle lifetime (ms) from.
  virtual void set_lifetime( std::uniform_int_distribution<int> lifetime ) = 0;
  //! @brief Set the position particles spawn from.
  //! @param emitter_position World/screen position of the emitter.
  virtual void set_emitter_position( sf::Vector2f emitter_position ) = 0;
  //! @brief Set the size of this particle.
  //! @param size The particle size.
  virtual void set_size( float size ) = 0;

private:
  // See ParticleBase for docstrings
  virtual void emit() = 0;
  virtual void idle() = 0;
};

struct ParticleBase : public Cmp::Particle::IParticle
{
  void do_emit() final
  {
    if ( m_particle_active )
    {
      m_vertex.position = m_emitter_position;
      static std::random_device rd;
      static std::mt19937 rng( rd() );
      m_lifetime = sf::milliseconds( m_lifetime_range( rng ) );
      emit();
      m_generation++;
    }
    else { idle(); }
  }

  //! @brief Disables IParticle::emit if false
  bool m_particle_active{ true };
  void set_speed( std::uniform_real_distribution<float> speed ) override { m_speed_range = speed; }
  void set_angle( std::uniform_real_distribution<float> angle ) override { m_angle_range = angle; }
  void set_phase( std::uniform_real_distribution<float> phase ) override { m_phase_range = phase; }
  void set_freq( std::uniform_real_distribution<float> freq ) override { m_freq_range = freq; }
  void set_emitter_position( sf::Vector2f emitter_position ) override { m_emitter_position = emitter_position; }
  void set_lifetime( std::uniform_int_distribution<int> lifetime ) override { m_lifetime_range = lifetime; }
  void set_size( float size ) override { m_size = size; }

  sf::Vertex m_vertex;
  sf::Vector2f m_velocity;
  sf::Time m_lifetime;
  sf::Vector2f m_emitter_position;
  float m_size{ 1.f };

  size_t generations() override { return m_generation; }
  size_t m_generation{ 0 };

protected:
  std::uniform_real_distribution<float> m_speed_range;
  std::uniform_real_distribution<float> m_angle_range;
  std::uniform_real_distribution<float> m_phase_range;
  std::uniform_real_distribution<float> m_freq_range;
  std::uniform_int_distribution<int> m_lifetime_range;

private:
  //! @brief Run when the particle is enabled and expired. Derived class of ParticleBase must implement it.
  //! @param emitter
  //! @param lifetime
  void emit() override = 0;

  //! @brief Run when the particle is disabled and expired
  //! @param emitter
  void idle() override {}
};

// ============================================================
// IParticleSprite — particle sprite container contract
// ============================================================

//! @brief Non-template abstract base — allows SpriteOwner and find() to work without knowing TParticle
class IParticleSprite : public sf::Drawable, public sf::Transformable
{
public:
  virtual ~IParticleSprite() = default;

  //! @brief  Implements the simulation stage of all particles.
  //          E.g. modify the vertex and velocity members of each particle,
  //          and reset the particle if its lifetime has expired
  //! @param dt
  virtual void simulate( sf::Time dt ) = 0;

  // Check SpriteBase for docstrings
  virtual void set_view_transform( const sf::RenderWindow &, const sf::View & ) = 0;
  virtual void reset_view_transform() = 0;
  virtual void stop() = 0;
  virtual void clear() = 0;
  virtual void restart() = 0;
  virtual void prune_inactive_expired_particles() = 0;
  //! @brief Kill any particle overlapping `target`
  //! @return true if at least one particle collided
  virtual bool check_particle_collision( const sf::FloatRect &target ) = 0;
  virtual bool is_active() = 0;
  virtual void deactivate_extinct_particles() = 0;
  virtual void set_tag( const std::string &tag ) = 0;
  virtual std::string get_tag() const = 0;
  virtual void set_generations( size_t gen ) = 0;
  virtual size_t get_generations() = 0;

  virtual void set_speed( std::uniform_real_distribution<float> speed_dist ) = 0;
  virtual void set_speed( float speed ) = 0;

  virtual void set_angle( std::uniform_real_distribution<float> angle_dist ) = 0;
  virtual void set_angle( float angle ) = 0;

  virtual void set_phase( std::uniform_real_distribution<float> phase_dist ) = 0;
  virtual void set_phase( float phase ) = 0;

  virtual void set_freq( std::uniform_real_distribution<float> freq_dist ) = 0;
  virtual void set_freq( float freq ) = 0;

  virtual void set_emitter_position( sf::Vector2f emitter_position ) = 0;
  virtual sf::Vector2f get_emitter_position() = 0;

  virtual void set_lifetime_ms( sf::Time lifetime ) = 0;
  virtual void set_lifetime_ms( std::uniform_int_distribution<int> life_dist ) = 0;

  virtual void set_view_type( ViewType v ) = 0;
  virtual ViewType get_view_type() = 0;

  virtual void set_scale( float scale ) = 0;

  virtual void set_particle_size_range( std::uniform_real_distribution<float> size_dist ) = 0;

  //! @brief Set the minimum distance a live particle must have travelled from the emitter before the
  //!        next particle may be emitted. 0 (the default) disables the check.
  //! @note Only guaranteed at the moment of emission: particles with differing speeds or sideways
  //!       motion can close the gap afterwards. Measured from the emitter position.
  virtual void set_min_particle_spacing( float spacing ) = 0;

  //! @brief Get the sprite's maximum bounding size (width/height), i.e. the full extent a particle
  //! could reach from the emitter given the sprite's configured max speed, lifetime and particle size.
  //! @note Cached — recomputed only when speed/lifetime/particle-size change, not from live particle
  //!       positions, so it's cheap enough to call every frame (e.g. for view-culling checks).
  //! @return sf::Vector2f The bounding width/height, centred on the emitter position.
  virtual sf::Vector2f get_size() const = 0;

  //! @brief Convenience getter combining get_size() with the emitter position: a rect centred on the
  //! emitter, sized to the sprite's maximum extent. See get_size() for what "size" means here.
  //! @note Cached alongside get_size() — recomputed on emitter/speed/lifetime/particle-size changes,
  //!       not iterated from live particle positions, so it's cheap enough to call every frame.
  //! @return sf::FloatRect The cached bounding rect.
  virtual sf::FloatRect get_bounds() const = 0;
};

//! @brief Defines the particle sprite base class template. This renders a list of TParticle vertices.
//! @tparam TParticle The particle object type.
//! @note The derived class must implement the update function and
//        provide its own IParticle implementation that satisfies ParticleConcept.
template <ParticleConcept TParticle>
class SpriteBase : public IParticleSprite
{
public:
  SpriteBase() = delete;

  //! @brief Construct a new Particle Sprite Base object
  //! @param count Number of particles in this sprite
  //! @param lifetime The lifetime of the particles in this sprite
  //! @param emitter_pos The initial position of the emitter for this sprite
  explicit SpriteBase( size_t count )
      : m_max_particles( count ),
        m_particles_list( count )

  {
    SPDLOG_DEBUG( "Created {} particles in sprite", count );
  }

  ~SpriteBase() {}

  //! @brief Replaces the default translation function with a world -> screen translation function
  //! @param window
  //! @param world_view
  //! @note Deliberately reimplements RenderTarget::mapCoordsToPixel() rather than calling it: that
  //        function returns a Vector2i, rounding each particle's position to the nearest whole window
  //        pixel independently. That's invisible for sparse, moving particles (e.g. Flame), but breaks
  //        tightly packed static layouts (e.g. AshPileParticleSprite's pyramid, where particles sit
  //        exact whole-world-unit offsets apart) whenever the camera zoom isn't an integer multiple of
  //        window pixels per world unit — neighbouring particles' spacing becomes uneven once rounded
  //        independently. Keeping the transform in float space preserves relative spacing exactly.
  void set_view_transform( const sf::RenderWindow &window, const sf::View &world_view ) override
  {
    m_world_to_screen = [&window, world_view]( sf::Vector2f world_pos ) -> sf::Vector2f
    {
      const sf::Vector2f normalized = world_view.getTransform().transformPoint( world_pos );
      const sf::FloatRect viewport( window.getViewport( world_view ) );
      return ( normalized.componentWiseMul( { 1.f, -1.f } ) + sf::Vector2f( 1.f, 1.f ) )
                 .componentWiseDiv( { 2.f, 2.f } )
                 .componentWiseMul( viewport.size ) +
             viewport.position;
    };

    // draw() builds each particle's dot from p.m_size, a size expressed in world units. Position
    // already gets magnified by the camera zoom above, via m_world_to_screen — but a raw, unscaled
    // m_size would stay pinned at its designed-zoom pixel footprint while the gaps between particles
    // (their positions) grow with zoom, so a tightly packed static layout (e.g. AshPileParticleSprite's
    // pyramid) visibly pulls apart into a coarse, "world resolution" pattern of dots the further in the
    // camera zooms. Scale it by the same window-pixels-per-world-unit ratio so dot size keeps pace with
    // spacing. Assumes a non-rotated, uniformly scaled view, true for this game's top-down camera.
    const sf::Vector2f viewport_size( window.getViewport( world_view ).size );
    m_world_to_screen_scale = viewport_size.x / world_view.getSize().x;
  }

  void reset_view_transform() override
  {
    m_world_to_screen = []( sf::Vector2f p ) { return p; };
    m_world_to_screen_scale = 1.f;
  }

  //! @brief Disables the particles for this ParticleSprite (they will continue simulating until their lifetimes expire)
  void stop() override
  {
    SPDLOG_DEBUG( "ParticleSprite Stop Signal Received" );

    for ( auto &p : m_particles_list )
    {
      // prevent emit() reseting the particle lifetime
      p.m_particle_active = false;
    }
    SPDLOG_DEBUG( "Particles have been disabled" );
  }

  void clear() override
  {
    for ( auto &p : m_particles_list )
    {
      p.m_particle_active = false;
      p.m_lifetime = sf::Time::Zero;
    }
    prune_inactive_expired_particles();
  }

  //! @brief Remove inactive and dead particles for this ParticleSprite
  void prune_inactive_expired_particles() override
  {

    std::erase_if( m_particles_list, []( const TParticle &p ) { return not p.m_particle_active and p.m_lifetime <= sf::Time::Zero; } );

    if ( m_particles_list.empty() )
    {
      SPDLOG_DEBUG( "Particles have been deleted" );
      // prevent ParticleSystem from calling this->simulate()
      m_sprite_active = false;
      SPDLOG_DEBUG( "ParticleSprite has stopped" );
    }
  }

  //! @brief Creates a new particle list, enables the particles, resets the generation counter and restarts the simulation.
  void restart() override
  {
    if ( m_sprite_active ) return;
    SPDLOG_DEBUG( "Restarting ParticleSprite" );

    m_particles_list = std::vector<TParticle>( m_max_particles );
    static std::random_device rd;
    static std::mt19937 rng( rd() );
    for ( auto &p : m_particles_list )
    {
      p.set_speed( m_cached_speed_range );
      p.set_angle( m_cached_angle_range );
      p.set_phase( m_cached_phase_range );
      p.set_freq( m_cached_freq_range );
      p.set_emitter_position( m_emitter_position );
      p.set_lifetime( m_cached_lifetime_range );
      p.set_size( m_particle_size_range( rng ) );
      p.m_particle_active = true;
    }
    m_sprite_active = true;
    SPDLOG_DEBUG( "ParticleSprite is running." );
  }

  //! @brief Check if the Particles from this ParticleSprite colide with the target
  //! @param target Rectangle collision area
  bool check_particle_collision( const sf::FloatRect &target ) override
  {
    bool collided = false;
    for ( auto &p : m_particles_list )
    {
      if ( not target.contains( p.m_vertex.position ) ) continue;
      p.m_lifetime = sf::Time::Zero;
      collided = true;
    }
    return collided;
  }

  //! @brief Is simulation running for this ParticleSprite?
  //! @return true
  //! @return false
  bool is_active() override { return m_sprite_active; }

  //! @brief Increase the generation count when the ParticleSprite lifetime has expired
  void deactivate_extinct_particles() override
  {
    if ( m_max_generations == 0 ) return;

    bool all_particles_last_generation = true;
    for ( auto &p : m_particles_list )
    {
      if ( p.m_generation < m_max_generations ) { all_particles_last_generation = false; }
    }

    if ( all_particles_last_generation ) { stop(); }
  }

  //! @brief Allows this sprite to be passed into RenderWindow.draw()
  //! @param target
  //! @param states
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override
  {
    states.texture = nullptr;
    states.blendMode = sf::BlendAlpha;

    // project out just the vertices for drawing
    std::vector<sf::Vertex> verts;
    verts.reserve( m_particles_list.size() );
    SPDLOG_DEBUG( "Drawing {} particles", m_particles.size() );

    constexpr int kSides = 4;
    for ( const auto &p : m_particles_list )
    {

      // map world -> screen
      const auto pos = m_world_to_screen( p.m_vertex.position );
      // p.m_size is a world-unit size; scale it by the same camera zoom applied to pos above (see
      // set_view_transform()) so dot size keeps pace with the growing/shrinking gaps between particles.
      const float size = p.m_size * m_world_to_screen_scale;

      const auto colour = p.m_vertex.color;

      // unit circle divided into wedges
      constexpr float kAngleStep = 2.f * std::numbers::pi_v<float> / static_cast<float>( kSides );
      // offset for edge-orientated rather than vertex-orientated
      constexpr float kAngleOffset = kAngleStep / 2.f;

      for ( int i = 0; i < kSides; ++i )
      {
        // the two outside facing (polar) angles for the current wedge
        const float a0 = kAngleOffset + ( kAngleStep * static_cast<float>( i ) );
        const float a1 = kAngleOffset + ( kAngleStep * static_cast<float>( i + 1 ) );

        // the centre position is always the middle of the polygon
        verts.push_back( { pos, colour } );
        // get the vertex positions by converting the polar coords to cartesian coords (euler's formula).
        verts.push_back( { pos + sf::Vector2f( size, sf::radians( a0 ) ), colour } );
        verts.push_back( { pos + sf::Vector2f( size, sf::radians( a1 ) ), colour } );
      }
    }

    target.draw( verts.data(), verts.size(), sf::PrimitiveType::Triangles, states );
  }

  //! @brief Max size of `m_particles_list`
  size_t m_max_particles;

  //! @brief The list of particles in this sprite
  std::vector<TParticle> m_particles_list;

  //! @brief The lifetime of the particles in this sprite
  sf::Time m_lifetime{ sf::Time::Zero };

  void set_tag( const std::string &tag ) override { m_tag = tag; }
  std::string get_tag() const override { return m_tag; }

  void set_generations( size_t gen ) override { m_max_generations = gen; }
  size_t get_generations() override { return m_max_generations; }

  void set_speed( std::uniform_real_distribution<float> speed_dist ) override
  {
    m_cached_speed_range = speed_dist;
    for ( auto &p : m_particles_list )
    {
      p.set_speed( m_cached_speed_range );
    }
    update_cached_size();
  }

  void set_speed( float speed ) override
  {
    m_cached_speed_range = std::uniform_real_distribution<float>( speed, speed );
    for ( auto &p : m_particles_list )
    {
      p.set_speed( m_cached_speed_range );
    }
    update_cached_size();
  }

  void set_angle( std::uniform_real_distribution<float> angle_dist ) override
  {
    m_cached_angle_range = angle_dist;
    for ( auto &p : m_particles_list )
    {
      p.set_angle( m_cached_angle_range );
    }
  }

  void set_angle( float angle ) override
  {
    m_cached_angle_range = std::uniform_real_distribution<float>( angle, angle );
    for ( auto &p : m_particles_list )
    {
      p.set_angle( m_cached_angle_range );
    }
  }

  void set_phase( std::uniform_real_distribution<float> phase_dist ) override
  {
    m_cached_phase_range = phase_dist;
    for ( auto &p : m_particles_list )
    {
      p.set_phase( m_cached_phase_range );
    }
  }

  void set_phase( float phase ) override
  {
    m_cached_phase_range = std::uniform_real_distribution<float>( phase, phase );
    for ( auto &p : m_particles_list )
    {
      p.set_phase( m_cached_phase_range );
    }
  }

  void set_freq( std::uniform_real_distribution<float> freq_dist ) override
  {
    m_cached_freq_range = freq_dist;
    for ( auto &p : m_particles_list )
    {
      p.set_freq( m_cached_freq_range );
    }
  }

  void set_freq( float freq ) override
  {
    m_cached_freq_range = std::uniform_real_distribution<float>( freq, freq );
    for ( auto &p : m_particles_list )
    {
      p.set_freq( m_cached_freq_range );
    }
  }

  //! @brief
  //! @param position
  void set_emitter_position( sf::Vector2f emitter_position ) override
  {
    m_emitter_position = emitter_position;
    for ( auto &p : m_particles_list )
    {
      p.set_emitter_position( emitter_position );
    }
    update_cached_bounds();
  }

  sf::Vector2f get_emitter_position() override { return m_emitter_position; }

  void set_lifetime_ms( std::uniform_int_distribution<int> lifetime_dist ) override
  {
    m_cached_lifetime_range = lifetime_dist;
    m_max_lifetime = sf::milliseconds( lifetime_dist.max() );
    static std::random_device rd;
    static std::mt19937 rng( rd() );
    m_lifetime = sf::milliseconds( m_cached_lifetime_range( rng ) );
    for ( auto &p : m_particles_list )
    {
      p.set_lifetime( lifetime_dist );
    }
    update_cached_size();
  }
  void set_lifetime_ms( sf::Time lifetime ) override
  {
    m_lifetime = lifetime;
    m_max_lifetime = lifetime;
    m_cached_lifetime_range = std::uniform_int_distribution<int>( lifetime.asMilliseconds(), lifetime.asMilliseconds() );
    for ( auto &p : m_particles_list )
    {
      p.set_lifetime( m_cached_lifetime_range );
    }
    update_cached_size();
  }

  void set_view_type( ViewType v ) override { m_view_type = v; };
  ViewType get_view_type() override { return m_view_type; };

  void set_particle_size_range( std::uniform_real_distribution<float> size_dist ) override
  {
    m_particle_size_range = size_dist;
    static std::random_device rd;
    static std::mt19937 rng( rd() );
    for ( auto &p : m_particles_list )
    {
      p.set_size( m_particle_size_range( rng ) );
    }
    update_cached_size();
  }

  void set_scale( float scale ) override { m_scale = scale; }

  void set_min_particle_spacing( float spacing ) override { m_min_particle_spacing = spacing; }

  //! @brief See IParticleSprite::get_size().
  sf::Vector2f get_size() const override { return m_cached_size; }

  //! @brief See IParticleSprite::get_bounds().
  sf::FloatRect get_bounds() const override { return m_cached_bounds; }

protected:
  //! @brief Emit an expired particle unless a live particle is still within the minimum spacing of
  //!        the emitter (see set_min_particle_spacing()). A blocked particle is hidden and retried
  //!        next frame.
  //! @return false if the particle is waiting and should be skipped this frame
  bool try_emit( TParticle &p )
  {
    // stopped particles bypass the check so they can still idle and be pruned
    if ( p.m_particle_active and not emitter_is_clear() )
    {
      p.m_vertex.color.a = 0;
      return false;
    }
    p.do_emit();
    return true;
  }

  //! @brief Default translation function is a noop. See set_view_transform()
  std::function<sf::Vector2f( sf::Vector2f )> m_world_to_screen = []( sf::Vector2f p ) { return p; };
  //! @brief Window-pixels-per-world-unit ratio computed alongside m_world_to_screen (see
  //!        set_view_transform()); scales each particle's world-unit m_size in draw() so dot size keeps
  //!        pace with the camera zoom already applied to particle position. 1.f (no scaling) by default.
  float m_world_to_screen_scale{ 1.f };

  //! @brief Disables IParticleSprite::simulate() if false
  bool m_sprite_active{ true };

  //! @brief Max lifetime of the Particles
  sf::Time m_max_lifetime;

  //! @brief Scales the overall spatial extent of the flame (sway width, rise height) — independent of per-particle dot size.
  //! @note World-view flames pass through a camera transform that magnifies this trajectory; screen-view (UI) flames don't,
  //        so the same raw amplitude/speed values read as differently sized. Tune this per view rather than the shared constants.
  float m_scale;

private:
  //! @brief True if no live particle is within the minimum spacing of the emitter.
  bool emitter_is_clear() const
  {
    if ( m_min_particle_spacing <= 0.f ) return true;
    const float min_spacing_squared = m_min_particle_spacing * m_min_particle_spacing;
    return std::ranges::none_of( m_particles_list, [&]( const TParticle &p )
    {
      return p.m_lifetime > sf::Time::Zero and ( p.m_vertex.position - m_emitter_position ).lengthSquared() < min_spacing_squared;
    } );
  }

  //! @brief See set_min_particle_spacing().
  float m_min_particle_spacing{ 0.f };

  //! @brief Max generations for the Particles in this sprite.
  size_t m_max_generations{ 0 };

  //! @brief Particle velocity range
  //! @note  Cache the init value so we can reset it later
  std::uniform_real_distribution<float> m_cached_speed_range{ 0.f, 1.f };

  //! @brief Particle angle range
  //! @note  Cache the init value so we can reset it later
  std::uniform_real_distribution<float> m_cached_angle_range{ 0.f, 360.f };

  //! @brief Particle phase range
  //! @note  Cache the init value so we can reset it later
  std::uniform_real_distribution<float> m_cached_phase_range{ 0.f, 1.f };

  //! @brief Particle frequency range
  //! @note  Cache the init value so we can reset it later
  std::uniform_real_distribution<float> m_cached_freq_range{ 0.f, 1.f };

  //! @brief Particle lifetime range
  //! @note  Cache the init value so we can reset it later
  std::uniform_int_distribution<int> m_cached_lifetime_range{ 0, 1 };

  std::uniform_real_distribution<float> m_particle_size_range{ 0, 1 };

  //! @brief Recomputes m_cached_size from the sprite's configured max speed, max lifetime and max
  //! particle size — the farthest a particle could travel from the emitter before expiring. Called
  //! whenever one of those parameters changes, so get_size() itself stays O(1).
  void update_cached_size()
  {
    const float radius = ( m_cached_speed_range.max() * m_max_lifetime.asSeconds() ) + m_particle_size_range.max();
    m_cached_size = sf::Vector2f( radius * 2.f, radius * 2.f );
    update_cached_bounds();
  }

  //! @brief Recomputes m_cached_bounds from m_cached_size and m_emitter_position. Called whenever
  //! either changes, so get_bounds() itself stays O(1).
  void update_cached_bounds() { m_cached_bounds = sf::FloatRect( m_emitter_position - m_cached_size / 2.f, m_cached_size ); }

  //! @brief Cached bounding width/height, see get_size() / update_cached_size().
  sf::Vector2f m_cached_size{ 0.f, 0.f };

  //! @brief Cached bounding rect centred on the emitter, see get_bounds() / update_cached_bounds().
  sf::FloatRect m_cached_bounds;

  //! @brief convenience identifier for searching the registry
  std::string m_tag;
  //! @brief The emitter position
  sf::Vector2f m_emitter_position{ 0, 0 };
  //! @brief Support world vs screen rendering
  ViewType m_view_type{ ViewType::WORLD };
};
} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_SPRITEBASE_HPP__
