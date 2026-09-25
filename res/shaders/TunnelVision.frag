#version 330

// The already-rendered game frame (or the previous post-process pass's output, see
// RenderGameSystem::render_game), captured into the shader's render texture before this pass runs.
uniform sampler2D texture;
// screen dimensions
uniform vec2 resolution;
// player position as screen UV (0..1), y already flipped to match gl_FragCoord
uniform vec2 player_uv;
// normalised player tachycardia/bradycardia toxicity (0..1), drives how far the aperture constricts
uniform float toxicity;

out vec4 out_color;

// Aperture radius at zero toxicity, in height-normalised units (1.0 == screen height). Large enough that
// the whole screen is visible even when the player is at the edge of the view.
const float APERTURE_RADIUS_MAX = 1.5;
// Aperture radius at full toxicity
const float APERTURE_RADIUS_MIN = 0.8;
// Width of the soft transition band at the aperture edge, as a fraction of the current radius
const float APERTURE_EDGE_SOFTNESS = 0.75;
// Curve steepness for the toxicity -> constriction ramp (see constrict_curve in main()): higher means more
// of the range is reached early, i.e. a faster rise that then tapers off (logarithmic, not linear)
const float APERTURE_LOG_K = 4.0;

void main()
{
  vec2 uv = gl_FragCoord.xy / resolution;
  vec4 color = texture2D( texture, uv );

  // aspect-corrected distance from the player so the aperture is a circle, not an ellipse
  vec2 delta = uv - player_uv;
  delta.x *= resolution.x / resolution.y;
  float dist_from_player = length( delta );

  // logarithmic ease-out from 0 at toxicity=0 to 1 at toxicity=1
  float constrict_curve = log( 1.0 + APERTURE_LOG_K * toxicity ) / log( 1.0 + APERTURE_LOG_K );
  float radius = mix( APERTURE_RADIUS_MAX, APERTURE_RADIUS_MIN, constrict_curve );

  // 1.0 inside the aperture, fading to 0.0 (black) at its edge
  float visibility = 1.0 - smoothstep( radius * ( 1.0 - APERTURE_EDGE_SOFTNESS ), radius, dist_from_player );
  color.rgb *= visibility;

  out_color = color;
}
