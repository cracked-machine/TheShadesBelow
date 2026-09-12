#version 330

// The already-rendered game frame, captured into the shader's render texture by RenderGameSystem
// before this pass runs, and re-sampled here with a displaced UV to warp the whole screen.
uniform sampler2D texture;
// elapsed time
uniform float time;
// screen dimensions
uniform vec2 resolution;
// normalised player fear (0..1), drives the wave amplitude/speed
uniform float fear;

out vec4 out_color;

// Max UV displacement (as a fraction of screen size) at full fear
const float MAX_AMPLITUDE = 0.005;
// Wave frequency across the screen, at zero fear
const float FREQUENCY_BASE = 4.0;
// Additional frequency added on top of FREQUENCY_BASE at full fear
const float FREQUENCY_RANGE = 10.0;
// Curve steepness for the fear -> frequency ramp (see fear_curve in main()): higher means more of
// the range is saved for fear values close to 1, i.e. a more gradual start
const float FREQUENCY_EXP_K = 3.5;
// Fixed animation rate. This must NOT depend on `fear`: fear changes in discrete steps mid-game
// (proximity/tick-based fear gain), and since `speed` scales the ever-growing `time` rather than
// accumulating an independent phase, any change to the multiplier would retroactively snap the
// whole animation to a different point in the cycle. Fear instead only scales `amplitude` below,
// so the wave motion itself stays a smooth, continuous loop no matter how fear changes.
const float WAVE_SPEED = 2.6;
// Strength of the darkened, reddish vignette at full fear
const float MAX_VIGNETTE = 0.55;
// Curve steepness for the fear -> vignette ramp (see vignette_curve in main()): higher means more
// of the range is reached early, i.e. a faster rise that then tapers off (logarithmic, not linear)
const float VIGNETTE_LOG_K = 9.0;
const float TWO_PI = 6.28318530718;

void main()
{
  // No y-flip needed here: `texture` was itself rendered (via gl_FragCoord-based rasterization)
  // into the same kind of target this pass is now drawing into, so both use the same orientation.
  vec2 uv = gl_FragCoord.xy / resolution;

  float amplitude = fear * MAX_AMPLITUDE;
  float speed = time * WAVE_SPEED;

  // Exponential ease-in from 0 at fear=0 to 1 at fear=1: low fear barely raises the frequency,
  // then it climbs sharply as fear approaches its max.
  float fear_curve = ( exp( FREQUENCY_EXP_K * fear ) - 1.0 ) / ( exp( FREQUENCY_EXP_K ) - 1.0 );
  float wave_frequency = FREQUENCY_BASE + fear_curve * FREQUENCY_RANGE;

  // Wrap each trig call's full argument (after its own scaling) into [0, 2*PI) independently.
  // sin(x) == sin(mod(x, 2*PI)) exactly, so this introduces no seam, but only if the wrap is
  // applied AFTER the term-specific multiplier: wrapping `speed` once and then scaling the
  // wrapped result by 1.3 would wrap it at the wrong period for that term (2*PI/1.3, not 2*PI)
  // and produce a visible jump every cycle.
  vec2 offset;
  offset.x = sin( mod( uv.y * wave_frequency + speed, TWO_PI ) ) * amplitude;
  offset.y = cos( mod( uv.x * wave_frequency * 0.85 + speed * 1.3, TWO_PI ) ) * amplitude;

  vec2 sample_uv = uv + offset;

  vec4 color;
  if ( sample_uv.x >= 0.0 && sample_uv.x <= 1.0 && sample_uv.y >= 0.0 && sample_uv.y <= 1.0 ) { color = texture2D( texture, sample_uv ); }
  else { color = texture2D( texture, uv ); }

  // subtle reddish vignette that intensifies with fear: logarithmic ease-out from 0 at fear=0 to 1
  // at fear=1, rising quickly at first then tapering off as fear approaches its max
  float dist_from_center = length( uv - 0.5 );
  float vignette_curve = log( 1.0 + VIGNETTE_LOG_K * fear ) / log( 1.0 + VIGNETTE_LOG_K );
  float vignette = smoothstep( 0.2, 0.9, dist_from_center ) * vignette_curve * MAX_VIGNETTE;
  color.rgb = mix( color.rgb, vec3( 0.2, 0.0, 0.0 ), vignette );

  out_color = color;
}
