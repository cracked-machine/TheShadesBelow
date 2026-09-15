#version 330

uniform sampler2D texture;
uniform float time;
uniform vec2 resolution;
uniform vec2 viewTopLeft;
uniform vec2 viewSize;
uniform vec2 mapSize;

out vec4 outColor;

float rand( float a, float b )
{
  uvec2 p = uvec2( uint( a ) * 2654435761u, uint( b ) * 2246822519u );
  p.x ^= p.y;
  p.x *= 2654435761u;
  p.x ^= p.x >> 16u;
  return float( p.x ) / float( 0xFFFFFFFFu );
}
float driftNoise( float layerSeed, float t )
{
  float i = floor( t );
  float f = fract( t );
  f = f * f * ( 3.0 - 2.0 * f );
  float a = fract( sin( dot( vec2( layerSeed, i ), vec2( 127.1, 311.7 ) ) ) * 43758.5453 );
  float b = fract( sin( dot( vec2( layerSeed, i + 1.0 ), vec2( 127.1, 311.7 ) ) ) * 43758.5453 );
  return mix( a, b, f ) * 2.0 - 1.0;
}

float sine_wave( float x ) { return sin( x ); }
float sawtooth_wave( float x ) { return fract( x / 6.28318 ) * 2.0 - 1.0; }

vec3 waveColor( int layer, int totalLayers )
{
  float t = float( layer ) / float( totalLayers - 1 );
  vec3 dark = vec3( 0.04, 0.12, 0.08 );  // dark murky green
  vec3 mid = vec3( 0.10, 0.22, 0.15 );   // muddy green
  vec3 light = vec3( 0.18, 0.28, 0.18 ); // dull olive green
  if ( t < 0.5 )
    return mix( dark, mid, t * 2.0 );
  else
    return mix( mid, light, ( t - 0.5 ) * 2.0 );
}

void main()
{
  vec2 normalizedScreen = gl_FragCoord.xy / resolution;
  normalizedScreen.y = 1.0 - normalizedScreen.y;
  vec2 worldPos = viewTopLeft + normalizedScreen * viewSize;

  float normX = worldPos.x / mapSize.x;
  float normY = worldPos.y / mapSize.y;

  const int NUM_LAYERS = 160;
  float layerHeight = 1.0 / float( NUM_LAYERS );

  vec3 finalColor = vec3( 0.05, 0.35, 0.55 );

  for ( int i = 0; i < NUM_LAYERS; i++ )
  {
    float fi = float( i );

    // Affect x/y drift of the overal wave layer
    float staticBand = 1.0 - fi * layerHeight;
    float driftSpeed = 0.02 + rand( fi, 8.0 ) * 0.08;
    float driftPhase = rand( fi, 9.0 ) * 6.28318;
    float driftFreq = 1.0 + rand( fi, 80.0 ) * 4.0;
    float driftAmp = layerHeight * 0.5;
    float bandTop = staticBand + sin( time * driftSpeed * driftFreq + driftPhase ) * driftAmp;

    // layer min/max random ranges
    float speed = ( rand( fi, 1.0 ) * 2.0 - 1.0 ) * 0.6;
    float freq = 75.0 + rand( fi, 2.0 ) * 40.0;
    float phase = rand( fi, 4.0 ) * 6.28318;
    float phase2 = rand( fi, 5.0 ) * 6.28318;
    float phase3 = rand( fi, 6.0 ) * 6.28318;

    // Per-layer scale of the waves
    float amp = 0.002 + rand( fi, 3.0 ) * 0.001;

    // Wave swell throughout the layer
    float ampSpeed = 0.5 + rand( fi, 1.0 ) * 1.0;
    float ampPhase = rand( fi, 12.0 ) * 6.28318;
    float ampFreq = 5.0 + rand( fi, 13.0 ) * 10.0;
    float ampMod = 0.5 + 0.2 * sin( normX * ampFreq * 6.28 + time * ampSpeed + ampPhase );

    float finalAmp = amp * ampMod;

    // harmonic content for natural gradient effect
    float crest = bandTop + sine_wave( normX * freq * 6.28 + time * speed + phase ) * finalAmp +
                  sawtooth_wave( normX * freq * 4.71 - time * speed * 0.7 + phase2 ) * finalAmp * 0.05 * driftPhase +
                  sawtooth_wave( normX * freq * 9.42 + time * speed * 1.3 + phase3 ) * finalAmp * 0.03 * driftPhase;

    if ( normY < crest )
    {
      vec3 layerColor = waveColor( i, NUM_LAYERS );
      float distToCrest = crest - normY;
      float highlight = smoothstep( 0.01, 0.0, distToCrest );
      layerColor += vec3( 0.3, 0.6, 0.6 ) * highlight; // dull olive highlight

      float layerAlpha = 0.4 + rand( fi, 5.7 ) * 0.3;
      finalColor = mix( layerColor, finalColor, layerAlpha );
    }
  }

  outColor = vec4( finalColor, 0.85 );
}