#version 330

// Frost overlay drawn over a toxidrome UI meter while the player has toxic immunity
// (see RenderOverlaySystem::render_ui_meters). Draws a frost haze and crystal ridges that thicken
// towards the meter edges, plus a few slowly twinkling glints.

// meter bounds in render target pixels: x, y (from the top), width, height
uniform vec4 rect;
// render target height, to flip gl_FragCoord.y (bottom-up) into rect's top-down space
uniform float target_height;
// seconds, drives the glint twinkle
uniform float time;
// per-meter offset so each meter gets its own frost pattern
uniform float seed;
// base wash colour (Cmp::Persist::ToxicImmunityMeterTint)
uniform vec4 frost;
// crystal ridge and glint colour (Cmp::Persist::ToxicImmunityMeterShimmer)
uniform vec4 glint;

out vec4 out_color;

// Scale of the frost haze noise (smaller = larger, softer clouds)
const float HAZE_SCALE = 0.15;
// Scale of the crystal cells (smaller = larger crystals)
const float CRYSTAL_SCALE = 0.12;
// How far in from the edges (as a fraction of the meter) the frost thickens
const float EDGE_REACH = 0.35;
// Only cells hashing above this get a glint, i.e. ~7% of crystals twinkle
const float GLINT_THRESHOLD = 0.93;
// Glint twinkle speed and sharpness
const float GLINT_SPEED = 2.0;
const float GLINT_SHARPNESS = 8.0;

float hash( vec2 p ) { return fract( sin( dot( p, vec2( 127.1, 311.7 ) ) ) * 43758.5453 ); }

vec2 hash2( vec2 p )
{
  return fract( sin( vec2( dot( p, vec2( 127.1, 311.7 ) ), dot( p, vec2( 269.5, 183.3 ) ) ) ) * 43758.5453 );
}

float value_noise( vec2 p )
{
  vec2 i = floor( p );
  vec2 f = fract( p );
  vec2 u = f * f * ( 3.0 - 2.0 * f );
  return mix( mix( hash( i ), hash( i + vec2( 1.0, 0.0 ) ), u.x ), mix( hash( i + vec2( 0.0, 1.0 ) ), hash( i + vec2( 1.0, 1.0 ) ), u.x ), u.y );
}

float fbm( vec2 p )
{
  float sum = 0.0;
  float amplitude = 0.5;
  for ( int octave = 0; octave < 4; ++octave )
  {
    sum += amplitude * value_noise( p );
    p *= 2.0;
    amplitude *= 0.5;
  }
  return sum;
}

// Cellular noise: returns the distances to the nearest (F1) and second nearest (F2) cell points, and the
// nearest cell's id in zw
vec4 voronoi( vec2 p )
{
  vec2 cell = floor( p );
  vec2 f = fract( p );
  float f1 = 8.0;
  float f2 = 8.0;
  vec2 nearest_cell = cell;
  for ( int y = -1; y <= 1; ++y )
  {
    for ( int x = -1; x <= 1; ++x )
    {
      vec2 offset = vec2( float( x ), float( y ) );
      float d = length( offset + hash2( cell + offset ) - f );
      if ( d < f1 )
      {
        f2 = f1;
        f1 = d;
        nearest_cell = cell + offset;
      }
      else if ( d < f2 ) { f2 = d; }
    }
  }
  return vec4( f1, f2, nearest_cell );
}

void main()
{
  vec2 p = vec2( gl_FragCoord.x, target_height - gl_FragCoord.y ) - rect.xy;
  vec2 uv = clamp( p / rect.zw, 0.0, 1.0 );

  // frost creeps in from the edges; the y distance is doubled as meters are wide and short
  float edge_dist = min( min( uv.x, 1.0 - uv.x ), min( uv.y, 1.0 - uv.y ) * 2.0 );
  float edge = 1.0 - smoothstep( 0.0, EDGE_REACH, edge_dist );

  float haze = fbm( p * HAZE_SCALE + seed );

  // thin bright lines where two crystal cells meet
  vec4 cells = voronoi( p * CRYSTAL_SCALE + seed );
  float ridges = 1.0 - smoothstep( 0.0, 0.08, cells.y - cells.x );
  ridges *= 0.4 + 0.6 * edge;

  // a few crystals twinkle, each at its own phase
  float cell_hash = hash( cells.zw );
  float glints = 0.0;
  if ( cell_hash > GLINT_THRESHOLD )
  {
    float twinkle = pow( max( sin( time * GLINT_SPEED + cell_hash * 6.2831 ), 0.0 ), GLINT_SHARPNESS );
    glints = twinkle * ( 1.0 - smoothstep( 0.0, 0.25, cells.x ) );
  }

  vec3 rgb = mix( frost.rgb, glint.rgb, clamp( ridges * 0.6 + glints, 0.0, 1.0 ) );
  float alpha = clamp( frost.a + haze * edge * 0.35 + ridges * glint.a * 0.5 + glints * glint.a, 0.0, 1.0 );
  out_color = vec4( rgb, alpha );
}
