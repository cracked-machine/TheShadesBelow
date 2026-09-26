#version 330

// The already-rendered game frame (or the previous post-process pass's output, see
// RenderGameSystem::render_game), captured into the shader's render texture before this pass runs.
uniform sampler2D texture;
// screen dimensions
uniform vec2 resolution;
// normalised player severity (0..1), drives how strong the vignette is
uniform float severity;

out vec4 out_color;

// Strength of the darkened, reddish vignette at full severity
const float MAX_VIGNETTE = 0.80;
// Curve steepness for the severity -> vignette ramp (see vignette_curve in main()): higher means more
// of the range is reached early, i.e. a faster rise that then tapers off (logarithmic, not linear)
const float VIGNETTE_LOG_K = 9.0;

void main()
{
  vec2 uv = gl_FragCoord.xy / resolution;
  vec4 color = texture2D( texture, uv );

  // subtle reddish vignette that intensifies with severity: logarithmic ease-out from 0 at severity=0 to 1
  // at severity=1, rising quickly at first then tapering off as severity approaches its max
  float dist_from_center = length( uv - 0.5 );
  float vignette_curve = log( 1.0 + VIGNETTE_LOG_K * severity ) / log( 1.0 + VIGNETTE_LOG_K );
  float vignette = smoothstep( 0.2, 0.9, dist_from_center ) * vignette_curve * MAX_VIGNETTE;
  color.rgb = mix( color.rgb, vec3( 0.2, 0.0, 0.0 ), vignette );

  out_color = color;
}
