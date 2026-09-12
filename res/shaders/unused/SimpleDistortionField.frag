#version 330 compatibility

uniform sampler2D tex;
uniform float time;
uniform vec2 screenSize;
uniform vec2 centerPosition;

out vec4 fragColor;

void main()
{
  // Normalize to [0,1] using the actual texture/screen size
  vec2 texCoord = gl_TexCoord[0].xy / screenSize;

  vec2 uv = ( texCoord - 0.5 ) * 2.0;

  float t = time * 0.3;

  float dist = length( uv );
  float angle = atan( uv.y, uv.x );

  float wave1 = sin( dist * 12.0 - t * 2.0 ) * 0.15;
  float wave2 = sin( dist * 20.0 + t * 1.5 ) * 0.08;
  float wave3 = cos( dist * 30.0 - t * 3.0 ) * 0.05;

  float radial_distort = sin( angle * 4.0 + t * 1.5 ) * 0.12;
  float spiral_distort = sin( angle * 3.0 + dist * 10.0 + t * 2.0 ) * 0.10;

  vec2 distortion = vec2( wave1 + wave2 + radial_distort + spiral_distort, wave1 + wave3 - radial_distort + spiral_distort );

  float polar_distort = sin( dist * 8.0 + angle * 2.0 + t * 2.0 ) * 0.08;
  distortion += vec2( cos( angle ), sin( angle ) ) * polar_distort;

  vec2 distorted_texCoord = texCoord + distortion * 0.1;

  vec4 color;
  if ( distorted_texCoord.x >= 0.0 && distorted_texCoord.x <= 1.0 && distorted_texCoord.y >= 0.0 && distorted_texCoord.y <= 1.0 )
  {
    color = texture( tex, distorted_texCoord );
  }
  else { color = texture( tex, texCoord ); }

  fragColor = color;
}