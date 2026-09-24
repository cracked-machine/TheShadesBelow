#version 330

// The already-rendered game frame, captured into the shader's render texture by RenderGameSystem
// before this pass runs, and re-sampled here with a displaced UV to warp the whole screen.
uniform sampler2D texture;
// elapsed time
uniform float time;
// screen dimensions
uniform vec2 resolution;
// normalised player toxicity (0..1), drives how large/frequent/intense the hallucination patches are
uniform float toxicity;
// player position in the same normalised [0,1] screen space as gl_FragCoord.xy/resolution, set by
// CircularDistortionShader::update from the current world view. Every arc curves around this point.
uniform vec2 player_uv;

out vec4 out_color;

// Sibling of CircularDistortion.frag: that version warps the *entire* frame, which reads as a general
// "queasy" wobble rather than a hallucination. This variant instead warps a handful of wavy arcs
// curving around the player at random distances - like rings of heat-haze radiating outward - while
// the rest of the frame stays sharp. Both take the same uniforms, so switching between them is just
// the .frag path passed to Factory::Shader::add_circular_distortion.

// How many haze patches are active at toxicity=0: zero, so there's no hallucination at all until toxicity
// rises (see MAX_EXTRA_HAZE_REGIONS).
const int BASE_HAZE_REGIONS = 0;
// One additional patch slot becomes eligible for every 10% of toxicity (toxicity=10% -> 1 slot, toxicity=20% -> 2,
// ... toxicity=100% -> all 10), via region_existence below. An eligible slot cycles through its own
// spawn/despawn rhythm (see PATCH_LIFETIME) the whole time toxicity holds it eligible; once toxicity drops
// back below its threshold that slot (and only it) stops appearing, mid-life or not.
const int MAX_EXTRA_HAZE_REGIONS = 10;
const int MAX_HAZE_REGIONS = BASE_HAZE_REGIONS + MAX_EXTRA_HAZE_REGIONS;
// Slot i's base bearing around the player is i * GOLDEN_ANGLE (see sector_base in main()), not
// i * (360/MAX_HAZE_REGIONS): slots also activate in index order as toxicity rises (region_existence
// below gates slot 0 first, then 1, then 2...), so a plain even division would mean the active
// subset at low/mid toxicity is always one contiguous wedge of the circle rather than spread around the
// player. The golden angle (~137.5 degrees) is the classic phyllotaxis trick for exactly this: ANY
// prefix of points placed at multiples of it - not just the full set - stays well-spread around the
// circle, so however many slots happen to be active, they cover the player's surroundings evenly.
const float GOLDEN_ANGLE = 2.399963;

// UV displacement (as a fraction of screen size) inside a patch. Bigger than CircularDistortion.frag's
// MAX_AMPLITUDE since only a small area carries it, not the whole screen. Fixed regardless of toxicity:
// amplitude is a multiplier on the ripple's sine wave, so scaling it with toxicity would scale the wave's
// on-screen velocity too (d(offset)/dt is proportional to amplitude) - i.e. it would visibly speed
// the ripple up even though WAVE_FREQUENCY/WAVE_SPEED never changed. Only patch count and duration
// carry the toxicity signal now.
const float AMPLITUDE = 0.018;
// Ripple frequency across a patch. Fixed regardless of toxicity - only how many patches appear, how
// strongly they displace, and how long each one lingers scale with toxicity, not the ripple itself.
const float WAVE_FREQUENCY = 8.0;
// Fixed animation rate. This must NOT depend on `toxicity`, for the same reason as CircularDistortion.frag's
// WAVE_SPEED: toxicity changes in discrete steps mid-game, and `speed` scales the ever-growing `time`
// rather than an independently accumulated phase, so any toxicity-dependent multiplier would retroactively
// snap the whole ripple to a different point in its cycle. toxicity only scales amplitude/size/timing below.
const float WAVE_SPEED = 3.4;

// Each patch is a partial ring - an arc of a circle centered on player_uv - rather than a straight
// line, at a random radius out from the player (see arc_radius in main()) so it reads as something
// curving around the player rather than a flat crack floating in front of them.
// How far out from the player an arc can sit, as a fraction of the shorter screen dimension
// (aspect-corrected the same way arc distances are, so this reads as a true radius around the
// player, not an ellipse). MIN_ARC_RADIUS keeps every arc's inner edge - even with ARC_THICKNESS'
// feathering - well clear of the player, so no arc ever touches them.
const float MIN_ARC_RADIUS = 0.16;
const float MAX_ARC_RADIUS = 0.55;
// Angular half-width of one arc, in radians - how much of the circle around the player it sweeps.
const float ARC_HALF_SPAN = 0.35;
// Radial half-thickness of the arc band, same units as MIN_ARC_RADIUS/MAX_ARC_RADIUS.
const float ARC_THICKNESS = 0.05;
// How far the arc's actual radius wanders above/below its base arc_radius as you sweep along it,
// same units as MIN_ARC_RADIUS/MAX_ARC_RADIUS - this is what makes the ring wavy rather than a clean
// circle segment.
const float ARC_WAVE_AMPLITUDE = 0.02;
// Spatial frequency of that waviness along the arc's own sweep (cycles per radian).
const float ARC_WAVE_FREQUENCY = 10.0;
// Width of the soft feathered edge at the arc's ends/sides, as a fraction of ARC_HALF_SPAN/
// ARC_THICKNESS respectively. Closer to 1.0 leaves less of a solid core and more of a gradual taper,
// softening how sharply the arc's edges cut off.
const float REGION_FEATHER = 0.85;
// How far an arc's center angle/radius wander from their spawned values, in radians and the same
// units as MIN_ARC_RADIUS/MAX_ARC_RADIUS respectively.
const float ANGLE_DRIFT = 0.05;
const float RADIUS_DRIFT = 0.02;
// Fixed rate (radians/sec) of that wander. This must NOT depend on `toxicity`, for the same reason as
// WAVE_SPEED above: it's a multiplier on the ever-growing wrapped_time, not a divisor, so unlike the
// old per-window cycle length this is safe to compute continuously - but it still shouldn't track
// toxicity, since patches are now persistent and their drift should just look like idle life, not
// another dial that visibly speeds up or slows down as toxicity moves.
const float DRIFT_SPEED = 1.57;
// Fixed lifetime each individual spawn of a patch lives before despawning and respawning fresh (new
// angle/radius within the same angular sector, new waviness). This is a *slot's* rhythm, independent
// of whether the slot is even active - `region_existence` above still separately gates that. It must
// NOT depend on `toxicity`, for the same reason WAVE_SPEED mustn't: (wrapped_time + seed) / PATCH_LIFETIME
// is used below to derive both which life we're in and how far through it we are, and wrapped_time
// keeps growing - a toxicity-dependent divisor there would retroactively reinterpret all of elapsed time
// whenever toxicity moved, not just time going forward, causing a visible jump (this bit the old
// toxicity-scaled HAZE_CYCLE_LENGTH design; see CircularDistortionShader::update's toxicity smoothing for the
// same principle applied to the toxicity value itself).
const float PATCH_LIFETIME = 5.0;
// Fraction of PATCH_LIFETIME spent easing a patch's strength in, and again easing it out, around each
// spawn/despawn.
const float PATCH_FADE_FRACTION = 0.25;

// Sickly violet tint blended into each patch's own shape (see total_tint in main()); reads as "something here is wrong" rather than just dark.
const vec3 HAZE_TINT_COLOR = vec3( 0.45, 0.1, 0.55 );
// Strength of that tint at a patch's peak strength (fully faded in, mid-life, toxicity well past its
// threshold). Overlapping patches take the strongest single patch at that pixel (see total_tint)
// rather than stacking, so this is also the effective ceiling with any number of patches overlapping.
const float MAX_HAZE_TINT = 0.1;

const float TWO_PI = 6.28318530718;
// `time` grows unbounded for as long as this shader instance stays alive (see BaseShaderSprite's
// m_clock) and feeds directly into `speed` below - scaled again by up to 1.3x for the y term. Left
// unbounded, float32 precision degrades as `time` grows: the per-frame phase step gets coarser,
// which over a long enough session makes the ripple look increasingly jerky/faster, independent of
// `toxicity`. Wrapping it into an hour-long window keeps that precision effectively perfect for any
// realistic session; the single-frame seam this introduces once an hour is imperceptible against the
// already-continuous drift/ripple motion.
const float TIME_WRAP_PERIOD = 3600.0;

// Cheap 1D/2D hash returning pseudo-random values in [0, 1). GLSL 330 has no built-in noise/random.
float hash11( float n ) { return fract( sin( n ) * 43758.5453123 ); }
vec2 hash21( float n ) { return vec2( hash11( n ), hash11( n + 17.17 ) ); }

void main()
{
  // No y-flip needed here: `texture` was itself rendered (via gl_FragCoord-based rasterization)
  // into the same kind of target this pass is now drawing into, so both use the same orientation.
  vec2 uv = gl_FragCoord.xy / resolution;
  float aspect = resolution.x / resolution.y;
  float wrapped_time = mod( time, TIME_WRAP_PERIOD );

  // How many patches are active at the current toxicity level, as a continuous (not stepped) value: each
  // patch beyond BASE_HAZE_REGIONS ramps in/out smoothly across its own 10%-of-toxicity band via
  // `region_existence` below, rather than snapping into/out of existence the instant its threshold is
  // crossed - the same reason CircularDistortionShader::update smooths `toxicity` itself before it reaches
  // this uniform.
  float active_regions = float( BASE_HAZE_REGIONS ) + toxicity * float( MAX_EXTRA_HAZE_REGIONS );

  // Pixel's position relative to the player, in polar form: pixel_radius is its aspect-corrected
  // distance from the player, pixel_angle its bearing. Every arc below is compared against these.
  vec2 player_delta = uv - player_uv;
  player_delta.x *= aspect;
  float pixel_radius = length( player_delta );
  float pixel_angle = atan( player_delta.y, player_delta.x );
  // Average angular spacing between slots' golden-angle base bearings - used below purely as the
  // jitter width around each slot's own base, not as a sector boundary.
  float arc_sector_size = TWO_PI / float( MAX_HAZE_REGIONS );

  vec2 total_offset = vec2( 0.0 );
  float total_tint = 0.0;

  for ( int i = 0; i < MAX_HAZE_REGIONS; ++i )
  {
    float region_existence = clamp( active_regions - float( i ), 0.0, 1.0 );
    if ( region_existence <= 0.0 ) continue;

    float seed = float( i ) * 91.7;

    // Which fixed-length life of this slot we're currently in, and how far through it we are. This
    // cycles regardless of region_existence - a slot that isn't active right now still keeps its own
    // rhythm running in the background, so when toxicity brings it back on it starts mid-life rather than
    // always popping in at the start of a fresh one.
    float life_index = floor( ( wrapped_time + seed ) / PATCH_LIFETIME );
    float local_t = fract( ( wrapped_time + seed ) / PATCH_LIFETIME );
    float life_seed = seed + life_index * 13.7;

    // Center angle, radius, drift phase and waviness phase are all re-rolled once per life (life_seed
    // only changes when life_index does) - each despawn/respawn looks like a fresh arc. The center
    // angle is a small random jitter (+/- half the average slot spacing) around this slot's own
    // golden-angle base bearing (see GOLDEN_ANGLE above), not anywhere around the player, so active
    // arcs stay spread out around them rather than clustering. The radius is independently random
    // between MIN_ARC_RADIUS and MAX_ARC_RADIUS for every arc.
    float sector_base = mod( float( i ) * GOLDEN_ANGLE, TWO_PI );
    float center_angle = sector_base + ( hash11( life_seed ) - 0.5 ) * arc_sector_size;
    float arc_radius = mix( MIN_ARC_RADIUS, MAX_ARC_RADIUS, hash11( life_seed * 2.3 ) );
    float drift_phase = hash11( life_seed * 3.1 ) * TWO_PI;
    float wave_phase = hash11( life_seed * 7.9 ) * TWO_PI;

    // Ease this life's strength in, hold, then ease it out; the next life's fade-in starts at 0
    // exactly where this one's fade-out ends at 0, so despawn/respawn never pops even though the
    // arc's position itself jumps between lives.
    float fade_in = smoothstep( 0.0, PATCH_FADE_FRACTION, local_t );
    float fade_out = 1.0 - smoothstep( 1.0 - PATCH_FADE_FRACTION, 1.0, local_t );
    float lifetime_envelope = fade_in * fade_out;

    // Continuous, non-repeating wander so a persistent arc doesn't sit perfectly still.
    float effective_angle = center_angle + sin( wrapped_time * DRIFT_SPEED + drift_phase ) * ANGLE_DRIFT;
    float effective_radius = arc_radius + cos( wrapped_time * DRIFT_SPEED * 1.3 + drift_phase ) * RADIUS_DRIFT;

    // Bearing from this arc's own center angle, wrapped to (-PI, PI] so the shortest-way-round
    // distance is used even when the arc sits near the +/-PI seam.
    float angle_diff = mod( pixel_angle - effective_angle + TWO_PI * 0.5, TWO_PI ) - TWO_PI * 0.5;

    // Perturb the arc's target radius as a sine function of bearing along it, bending what would
    // otherwise be a clean ring segment into a wavy one; `radial_diff` is then this pixel's distance
    // from that wavy target radius, playing the same role thickness distance would for a straight line.
    float wavy_radius = effective_radius + ARC_WAVE_AMPLITUDE * sin( angle_diff * ARC_WAVE_FREQUENCY + wave_phase );
    float radial_diff = pixel_radius - wavy_radius;

    float length_falloff = 1.0 - smoothstep( ARC_HALF_SPAN * ( 1.0 - REGION_FEATHER ), ARC_HALF_SPAN, abs( angle_diff ) );
    float thickness_falloff = 1.0 - smoothstep( ARC_THICKNESS * ( 1.0 - REGION_FEATHER ), ARC_THICKNESS, abs( radial_diff ) );
    float radial_falloff = length_falloff * thickness_falloff;

    float strength = radial_falloff * AMPLITUDE * region_existence * lifetime_envelope;

    // Tint uses the same patch shape/fade as the displacement above but not AMPLITUDE - color isn't a
    // UV offset, so it has its own ceiling (MAX_HAZE_TINT). Overlapping patches take the strongest
    // single one at this pixel rather than summing, so many overlapping patches can't blow the tint
    // out past a single patch's own peak.
    float patch_tint = radial_falloff * region_existence * lifetime_envelope;
    total_tint = max( total_tint, patch_tint );

    // Wrap each trig call's full argument (after its own scaling) into [0, 2*PI) independently, for
    // the same reason as CircularDistortion.frag: sin(x) == sin(mod(x, 2*PI)) exactly, so this introduces
    // no seam, but only because the wrap is applied after the term-specific multiplier.
    float speed = ( wrapped_time + seed ) * WAVE_SPEED;
    total_offset.x += sin( mod( uv.y * WAVE_FREQUENCY + speed, TWO_PI ) ) * strength;
    total_offset.y += cos( mod( uv.x * WAVE_FREQUENCY * 0.85 + speed * 1.3, TWO_PI ) ) * strength;
  }

  vec2 sample_uv = uv + total_offset;

  vec4 color;
  if ( sample_uv.x >= 0.0 && sample_uv.x <= 1.0 && sample_uv.y >= 0.0 && sample_uv.y <= 1.0 ) { color = texture2D( texture, sample_uv ); }
  else { color = texture2D( texture, uv ); }

  // Sickly violet tint over each patch's own shape.
  color.rgb = mix( color.rgb, HAZE_TINT_COLOR, total_tint * MAX_HAZE_TINT );

  out_color = color;
}
