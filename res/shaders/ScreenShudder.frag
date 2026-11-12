#version 330

// The already-rendered game frame (or the previous post-process pass's output, see
// RenderGameSystem::render_game), captured into the shader's render texture before this pass runs.
uniform sampler2D texture;
// screen dimensions
uniform vec2 resolution;
// current shake offset as screen UV, y already flipped to match gl_FragCoord. Already scaled by the shudder's
// envelope and amplitude, see ScreenShudderShader::update
uniform vec2 offset;
// shudder envelope (0..1): 0 while idle or fully settled, 1 at the height of the rumble
uniform float strength;

out vec4 out_color;

// How far back along the shake the motion blur taps reach, as a multiple of the current offset
const float BLUR_SPREAD = 0.6;
// How much the frame darkens at full strength, like dust shaken loose from the ceiling
const float MAX_DARKEN = 0.06;

// Mirror the frame back on itself past its edges rather than smearing its edge pixels
vec2 mirror( vec2 uv ) { return 1.0 - abs( 1.0 - abs( uv ) ); }

void main()
{
  vec2 uv = gl_FragCoord.xy / resolution;
  vec2 sample_uv = uv + offset;

  // cheap 3-tap motion blur back along the direction of the shake, which sells the weight of the movement
  vec2 blur_step = offset * BLUR_SPREAD * strength;
  vec4 color = texture2D( texture, mirror( sample_uv ) ) * 0.5;
  color += texture2D( texture, mirror( sample_uv - blur_step ) ) * 0.3;
  color += texture2D( texture, mirror( sample_uv - 2.0 * blur_step ) ) * 0.2;

  color.rgb *= 1.0 - MAX_DARKEN * strength;
  out_color = color;
}
