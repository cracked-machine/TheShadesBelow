#version 330

// The already-rendered game frame (or the previous post-process pass's output, see
// RenderGameSystem::render_game), captured into the shader's render texture before this pass runs.
uniform sampler2D texture;
// screen dimensions
uniform vec2 resolution;
// player position as screen UV (0..1), y already flipped to match gl_FragCoord
uniform vec2 player_uv;
// current zoom amount (-0.5..0.5): how far the scene beyond the player is pulled in towards them (positive,
// zoomed in) or pushed away (negative, zoomed out). Already scaled by the player's vertigo toxicity and the zoom
// cycle, see DollyZoomShader::update
uniform float zoom;
// current tilt of the zoom, in radians: how far the scene beyond the player is twisted around them. Signed, and
// already scaled by the player's vertigo toxicity and its own cycle, independent of zoom
uniform float skew;

out vec4 out_color;

// Distance from the player, in height-normalised units (1.0 == screen height), inside which the scene keeps
// its size: the "subject" of the dolly zoom
const float SUBJECT_RADIUS = 0.1;
// Distance from the player at which the zoom reaches its full amount
const float BACKGROUND_RADIUS = 0.9;

void main()
{
  vec2 uv = gl_FragCoord.xy / resolution;

  // aspect-corrected distance from the player so the unzoomed subject area is a circle, not an ellipse
  float aspect = resolution.x / resolution.y;
  vec2 delta = uv - player_uv;
  delta.x *= aspect;
  float dist_from_player = length( delta );

  // how much of the effect applies here: none on the subject, all of it out in the background
  float background = smoothstep( SUBJECT_RADIUS, BACKGROUND_RADIUS, dist_from_player );

  // Sample closer to the player the further out we are (or further away, when zooming out): that magnifies
  // (or shrinks) the background around the player while leaving the subject area untouched.
  delta *= 1.0 - zoom * background;

  // Twist the same sample around the player, so the zoom comes in at a tilt and the subject stays upright
  float angle = skew * background;
  float s = sin( angle );
  float c = cos( angle );
  delta = vec2( c * delta.x - s * delta.y, s * delta.x + c * delta.y );

  delta.x /= aspect;

  // Zooming out, and the twist at the corners, reach past the edge of the frame; mirror the frame back on
  // itself there rather than smearing its edge pixels
  vec2 sample_uv = player_uv + delta;
  sample_uv = 1.0 - abs( 1.0 - abs( sample_uv ) );
  out_color = texture2D( texture, sample_uv );
}
