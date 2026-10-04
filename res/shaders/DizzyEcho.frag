#version 330

// Upper bound on the number of echoes. Must match DizzyEchoShader::kMaxEchoFrames.
const int MAX_ECHO_FRAMES = 8;

// Earlier captures of the sprite layer (the player, NPC and world item sprites on a transparent background, see
// RenderGameSystem::capture_sprite_layer), one echo delay apart, [0] being the most recent. Only the first
// frame_count entries are bound.
uniform sampler2D history[MAX_ECHO_FRAMES];
// Per-echo UV shift from the current view to the view that echo was captured with, which keeps each echo
// anchored to the world as the camera moves
uniform vec2 echo_offset[MAX_ECHO_FRAMES];
// screen dimensions
uniform vec2 resolution;
// how many entries of history to echo (0..MAX_ECHO_FRAMES)
uniform int frame_count;
// alpha multiplier applied per successive echo (0..1): lower means the echoes fade out faster
uniform float alpha_rolloff;
// alpha of the first echo (0..1), derived from the player's vertigo toxicity; later echoes roll off from it
uniform float strength;

out vec4 out_color;

// GLSL 3.30 only allows a sampler array to be indexed by a constant expression, hence the switch
vec4 sample_history( int index, vec2 uv )
{
  switch ( index )
  {
    case 0: return texture2D( history[0], uv );
    case 1: return texture2D( history[1], uv );
    case 2: return texture2D( history[2], uv );
    case 3: return texture2D( history[3], uv );
    case 4: return texture2D( history[4], uv );
    case 5: return texture2D( history[5], uv );
    case 6: return texture2D( history[6], uv );
    default: return texture2D( history[7], uv );
  }
}

void main()
{
  vec2 uv = gl_FragCoord.xy / resolution;

  // composite the echoes oldest first so the more recent, stronger ones sit on top (premultiplied alpha, which
  // is also how the layers are stored)
  vec4 echoes = vec4( 0.0 );
  for ( int i = MAX_ECHO_FRAMES - 1; i >= 0; --i )
  {
    if ( i >= frame_count ) continue;

    // echoes that have scrolled out of their captured layer have nothing to show
    vec2 echo_uv = uv + echo_offset[i];
    if ( any( lessThan( echo_uv, vec2( 0.0 ) ) ) || any( greaterThan( echo_uv, vec2( 1.0 ) ) ) ) continue;

    // pow( 0, 0 ) is undefined, so the first echo skips the roll-off explicitly
    float echo_alpha = ( i == 0 ) ? strength : strength * pow( alpha_rolloff, float( i ) );
    vec4 echo = sample_history( i, echo_uv ) * echo_alpha;
    echoes = echo + echoes * ( 1.0 - echo.a );
  }

  // back to straight alpha for the standard alpha blend onto the window
  out_color = vec4( echoes.rgb / max( echoes.a, 0.0001 ), echoes.a );
}
