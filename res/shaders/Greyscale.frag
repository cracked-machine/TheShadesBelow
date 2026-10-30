#version 330 compatibility

// Desaturates a world sprite tagged with Cmp::Greyscale (see RenderGameSystem::draw_animated_sprite). The vertex
// colour is applied after desaturating, so Cmp::AbsoluteTint and Cmp::AbsoluteAlpha still tint and fade the grey result.

// The sprite sheet texture
uniform sampler2D texture;

out vec4 out_color;

// Rec. 601 luma weights: perceived brightness of each rgb channel
const vec3 LUMA_WEIGHTS = vec3( 0.299, 0.587, 0.114 );
// Stretches the grey levels away from mid-grey: 1.0 = plain greyscale, higher = darker darks and brighter lights
const float CONTRAST = 1.5;

void main()
{
  vec4 texel = texture2D( texture, gl_TexCoord[0].xy );
  float luma = dot( texel.rgb, LUMA_WEIGHTS );
  luma = clamp( ( luma - 0.5 ) * CONTRAST + 0.5, 0.0, 1.0 );
  out_color = vec4( vec3( luma ), texel.a ) * gl_Color;
}
