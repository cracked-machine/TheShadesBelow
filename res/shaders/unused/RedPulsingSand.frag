#version 330

// Sampled texture (passed in by SFML)
uniform sampler2D texture;
// elapsed time
uniform float time;
// effect opacity
uniform float alpha;
// screen dimensions
uniform vec2 resolution;

// final output pixel color
out vec4 outColor;

// pseudo-random noise using a hash function
float noise( vec2 st ) { return fract( sin( dot( st.xy, vec2( 12.9898, 78.233 ) ) ) * 43758.5453123 ); }

// Bilinear interpolation between noise values for smooth transitions
float smoothNoise( vec2 st )
{
  vec2 i = floor( st );
  vec2 f = fract( st );

  float a = noise( i );
  float b = noise( i + vec2( 1.0, 0.0 ) );
  float c = noise( i + vec2( 0.0, 1.0 ) );
  float d = noise( i + vec2( 1.0, 1.0 ) );

  vec2 u = f * f * ( 3.0 - 2.0 * f );

  return mix( a, b, u.x ) + ( c - a ) * u.y * ( 1.0 - u.x ) + ( d - b ) * u.x * u.y;
}

void main()
{
  // Normalise screen coords to UV coords for the current pixel
  vec2 texCoord = gl_FragCoord.xy / resolution;

  // Sample the base texture color at current pixel
  vec4 color = texture2D( texture, texCoord );

  // Create a mighnight blue color tint (39,39,87)
  vec4 tint = vec4( 0.11, 0.11, 0.34, 0.0 ) * 0.75;
  color += tint;

  // Add grain noise variation
  vec2 simpleCoord = texCoord * 5.0 + time * 1.0;
  float simpleNoise = smoothNoise( simpleCoord );

  // mix noise: brightness + noise * intensity
  color.rgb *= 0.1 + simpleNoise * 0.8;

  // Apply the alpha uniform
  color.a *= alpha;

  outColor = color;
}