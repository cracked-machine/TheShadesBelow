#version 330

// Sampled texture (passed in by SFML)
uniform sampler2D texture;
// elapsed time
uniform float time;

// screen dimensions
uniform vec2 resolution;

// world-space top-left of the current view
uniform vec2 view_top_left;
// world-space size of the current view
uniform vec2 view_size;

// The overal size of the torchlight radius
uniform float torch_radius;

// normalised player fear (0..1), pushes NIGHT_ALPHA_BASE up towards NIGHT_ALPHA_BASE + NIGHT_ALPHA_FEAR_RANGE
uniform float fear;

// player position in world space
const int MAX_TORCH_COUNT = 40;
uniform int torch_count;
uniform vec2 torch_world_pos[MAX_TORCH_COUNT];
// per-torch light color, RGB normalized (0..1) not 8bit RGB; alpha is the light's strength (0.0 == off, 1.0 == full),
// scaling both how much it lifts the darkness and how strongly the color tints the pixel
uniform vec4 torch_color[MAX_TORCH_COUNT];

const float M_PI = 3.1415926535897932384626433832795;
const float M_2PI = 6.28318;

const vec3 NIGHT_COLOR = vec3( 0.1, 0.1, 0.44 );

const float TORCH_ALPHA = 0.25;
// 0.0 lighter, 1.0 darker. Baseline outside torchlight; scaled up towards NIGHT_ALPHA_FEAR_RANGE by fear, see main()
const float NIGHT_ALPHA_BASE = 0.7;
const float NIGHT_ALPHA_FEAR_RANGE = 0.2;
// Curve steepness for the fear -> alpha ramp (see fear_curve in main()): higher means more of the
// range is reached early, i.e. a faster rise that then tapers off (logarithmic, not linear)
const float NIGHT_ALPHA_LOG_K = 9.0;

// The max frequency of the torchlight flicker
const float TORCH_EDGE_FLICKER_FREQ = 2.0;
// The percentage of the radius that is flickering
const float TORCH_EDGE_FLICKER_PERCENT = 0.1;
// Transition band width inside/outside of radius

// night static graininess, at zero fear
const float NIGHT_STATIC_CONTRAST_BASE = 11.0;
// additional graininess added on top of NIGHT_STATIC_CONTRAST_BASE at full fear
const float NIGHT_STATIC_CONTRAST_RANGE = 5.0;
// curve steepness for the fear -> contrast ramp (see contrast_curve in main()): higher means more
// of the range is saved for fear values close to 1, i.e. a more gradual start (exponential ease-in)
const float NIGHT_STATIC_CONTRAST_EXP_K = 3.5;

// NPC (Watchman) searchlight cones
uniform int npc_count;
uniform vec2 npc_positions[MAX_TORCH_COUNT];
uniform vec2 npc_directions[MAX_TORCH_COUNT];
// per-cone light color, same format as torch_color
uniform vec4 npc_color[MAX_TORCH_COUNT];
uniform float npc_torch_length; // shared reach for all Watchman cones, world units
uniform float npc_torch_angle;  // shared half-angle for all Watchman cones, radians

// final output pixel sampled_color
out vec4 out_color;

float noise( float a, float b )
{
  uvec2 p = uvec2( uint( a ) * 2654435761u, uint( b ) * 2246822519u );
  p.x ^= p.y;
  p.x *= 2654435761u;
  p.x ^= p.x >> 16u;
  return float( p.x ) / float( 0xFFFFFFFFu );
}

// Create a normalised (0..1) sine wave position from t
float norm_sin( float t ) { return sin( t * M_2PI ) * 0.5 + 0.5; }

// Determine if the current frag is within the player torch radius
float player_torch_pixel( vec2 frag_coord, int torch_idx )
{

  // Create a natural looking flicker on the radius. Multipliers are for
  // Speed/Dominance. Speed multiplers should have Relative Irrationality.
  // Dominanace multiplers should have Relative Sum of 1.0
  float flicker1 = norm_sin( time * TORCH_EDGE_FLICKER_FREQ * 1.0 ) * 0.5;
  float flicker2 = norm_sin( time * TORCH_EDGE_FLICKER_FREQ * 2.3 ) * 0.3;
  float flicker3 = norm_sin( time * TORCH_EDGE_FLICKER_FREQ * 0.7 ) * 0.2;
  float flickered_radius = torch_radius * ( 1.0 + ( flicker1 + flicker2 + flicker3 ) * TORCH_EDGE_FLICKER_PERCENT );

  float dist_to_player = length( frag_coord - torch_world_pos[torch_idx] );

  // return 0..1 between flickered_radius and 0
  // return smoothstep( flickered_radius, 0.0, dist_to_player );
  return clamp( 1.0 - ( dist_to_player / flickered_radius ), 0.0, 1.0 );
}

// Returns the strongest cone contribution at frag_coord, and writes that cone's color to light_color.
float npc_light_pixel( vec2 frag_coord, out vec4 light_color )
{
  // NPC cones: apex at NPC, spreads outward in facing direction
  float in_npc_light = 0.0;
  light_color = vec4( 0.0 );
  for ( int i = 0; i < npc_count; i++ )
  {
    vec2 to_pixel = frag_coord - npc_positions[i];
    float dist = length( to_pixel );
    // guard normalize() against a zero-length vector (pixel exactly at the NPC's own position)
    if ( dist < 0.0001 || dist >= npc_torch_length ) continue;

    float cos_angle = dot( to_pixel / dist, npc_directions[i] );
    if ( cos_angle > cos( npc_torch_angle ) )
    {
      // Fade toward edges of cone and toward tip
      float angular_fade = smoothstep( cos( npc_torch_angle ), cos( npc_torch_angle * 0.5 ), cos_angle );
      float dist_fade = smoothstep( npc_torch_length, npc_torch_length * 0.2, dist );
      float contribution = angular_fade * dist_fade;
      if ( contribution > in_npc_light )
      {
        in_npc_light = contribution;
        light_color = npc_color[i];
      }
    }
  }
  return in_npc_light;
}

vec2 convert_to_world_space( vec2 screen_coord )
{
  // Normalise screen coords to UV coords for the current pixel.
  vec2 uv_coord = screen_coord.xy / resolution;
  // Flip the y-axis for SFML/OpenGL.
  uv_coord.y = 1.0 - uv_coord.y;
  // Convert to world space so shader is anchored to the world, not the screen.
  return view_top_left + uv_coord * view_size;
}

void main()
{

  vec2 frag_coord = convert_to_world_space( gl_FragCoord.xy );
  vec4 sampled_color = texture2D( texture, frag_coord );

  // ── Torch / lighting ────────────────────────────────────────────────────────
  // Accumulate the strongest light contribution from all torches, and track that light's color
  // (the color of whichever torch/cone contributes the most at this pixel).
  float frag_coord_light_amount = 0.0;
  vec4 frag_light_color = vec4( 0.0 );
  for ( int i = 0; i < torch_count; i++ )
  {
    float torch_light_amount = player_torch_pixel( frag_coord, i ) * torch_color[i].a;
    if ( torch_light_amount > frag_coord_light_amount )
    {
      frag_coord_light_amount = torch_light_amount;
      frag_light_color = torch_color[i];
    }
  }
  vec4 npc_light_color;
  float npc_light_amount = npc_light_pixel( frag_coord, npc_light_color );
  if ( npc_light_amount > frag_coord_light_amount )
  {
    frag_coord_light_amount = npc_light_amount;
    frag_light_color = npc_light_color;
  }
  // ── Static effect ────────────────────────────────────────────────────────────

  sampled_color.rgb = mix( sampled_color.rgb * NIGHT_COLOR, sampled_color.rgb, frag_coord_light_amount );

  // Sample the noise to create moving static.
  // using large prime, cap the offset before it gets large enough to lose precision, while avoiding obvious periodicity
  float static_noise_refresh_rate = mod( floor( time * 120.0 ), 997.0 );

  // Sample the noise used for night static
  float static_noise = noise( frag_coord.x + static_noise_refresh_rate * 7919.0, frag_coord.y + static_noise_refresh_rate * 6271.0 );

  // Apply night static only outside the torch radius, fade out quickly at the edge
  // Exponential ease-in from 0 at fear=0 to 1 at fear=1: low fear barely raises the graininess,
  // then it climbs sharply as fear approaches its max.
  float contrast_curve = ( exp( NIGHT_STATIC_CONTRAST_EXP_K * fear ) - 1.0 ) / ( exp( NIGHT_STATIC_CONTRAST_EXP_K ) - 1.0 );
  float night_static_contrast = NIGHT_STATIC_CONTRAST_BASE + contrast_curve * NIGHT_STATIC_CONTRAST_RANGE;
  float noise_mask = 1.0 - smoothstep( 0.0, 0.2, frag_coord_light_amount );
  float noise_factor = mix( 1.0, static_noise * night_static_contrast, noise_mask );
  sampled_color.r *= noise_factor;
  sampled_color.g *= noise_factor;
  sampled_color.b *= noise_factor;

  // Apply the colors for inside/outside the torch radius. Alpha scales how strongly the light's
  // color tints the pixel (1.0 == fully applied, matching the old fixed TORCH_COLOR behaviour).
  sampled_color.rgb = mix( sampled_color.rgb * NIGHT_COLOR, sampled_color.rgb + frag_light_color.rgb * frag_light_color.a, frag_coord_light_amount );

  // ── Output ───────────────────────────────────────────────────────────────────
  // Logarithmic ease-out from 0 at fear=0 to 1 at fear=1: rises quickly at first, then tapers off
  // as fear approaches its max (the mirror image of fear_distortion.frag's exponential ease-in).
  float fear_curve = log( 1.0 + NIGHT_ALPHA_LOG_K * fear ) / log( 1.0 + NIGHT_ALPHA_LOG_K );
  float night_alpha = NIGHT_ALPHA_BASE + fear_curve * NIGHT_ALPHA_FEAR_RANGE;
  sampled_color.a *= mix( night_alpha, TORCH_ALPHA, frag_coord_light_amount );

  out_color = sampled_color;
}