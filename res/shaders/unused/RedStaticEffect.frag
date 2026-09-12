#version 330

// the rendered scene texture to apply the effect to
uniform sampler2D texture;

// used to animate the noise (changes every frame)
uniform float time;

//  controls overall transparency of the effect
uniform float alpha;

// screen dimensions, used to convert pixel coords to UV coords
uniform vec2 resolution;

out vec4 outColor;

float noise( vec2 st ) { return fract( sin( dot( st.xy, vec2( 12.9898, 78.233 ) ) + time * 10.0 ) * 43758.5453123 ); }

void main()
{
  // Convert the fragment's pixel position into normalized [0.0, 1.0] UV space.
  vec2 uv = gl_FragCoord.xy / resolution;

  // generate independent noise for each RGB channel using unique UV offsets
  float rn = noise( uv + vec2( 0.0, 0.0 ) );
  float gn = noise( uv + vec2( 0.1, 0.7 ) );
  float bn = noise( uv + vec2( 0.4, 0.2 ) );

  // get the color pixel from the texture
  vec4 color = texture2D( texture, uv );

  // Use noise value to vary the next pixel color
  vec3 staticColor = vec3( 0.7 + 0.3 * rn, 0.7 + 0.1 * gn, 0.7 + 0.1 * bn );

  float threshold = 0.85;
  float staticAmount = smoothstep( threshold, 1.0, rn );
  color.rgb = mix( color.rgb, staticColor, staticAmount );

  // set the alpha
  color.a *= alpha;

  // return the output
  outColor = color;
}