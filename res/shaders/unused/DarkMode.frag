#version 330

uniform vec2 local_resolution;
uniform vec2 aperture_half_size;
uniform vec2 display_resolution;
uniform float time;

out vec4 outColor;

// Simple noise function for flickering effect
float noise( float t ) { return sin( t * 6.28318 ) * 0.5 + 0.5; }

void main()
{
  vec2 fragCoord = gl_FragCoord.xy;

  // This shader is drawn in "local view" coord system, but uses world coords input (player position, etc...).
  // So we must transform fragment coordinates from display space to local view space
  vec2 normalized_coord = fragCoord / display_resolution;
  vec2 local_coord = normalized_coord * local_resolution;

  vec2 center = local_resolution * 0.5;

  vec2 dist_from_center = local_coord - center;
  // Calculate circular distance
  float distance_from_center = length( dist_from_center );
  // Use the smaller aperture dimension as the radius
  float aperture_radius = min( aperture_half_size.x, aperture_half_size.y );

  // Red outline thickness
  float outline_thickness = 3.0;

  // Check if we're on the border of the shader
  // bool on_border = ( local_coord.x < outline_thickness || local_coord.x > local_resolution.x - outline_thickness ||
  //                    local_coord.y < outline_thickness || local_coord.y > local_resolution.y - outline_thickness );

  // if ( on_border )
  // {
  //   Red outline
  //   gl_FragColor = vec4( 1.0, 0.0, 0.0, 1.0 );
  // }
  // else
  // {
  // Candlelight flickering effect
  float flicker_speed = 3.0;
  float flicker_intensity = 0.05;

  // Combine multiple noise frequencies for more realistic flicker
  float flicker = noise( time * flicker_speed ) * 0.5 + noise( time * flicker_speed * 2.3 ) * 0.3 + noise( time * flicker_speed * 0.7 ) * 0.2;

  // Apply flicker to adjust the effective aperture radius
  float flickered_radius = aperture_radius * ( 1.0 + flicker * flicker_intensity );

  // Blur edge width
  float blur_width = 20.0;

  // Create smooth transition from transparent to black using the flickered radius
  float alpha = smoothstep( flickered_radius - blur_width, flickered_radius + blur_width, distance_from_center );

  // Warm white tint (candlelight color)
  vec3 warm_white = vec3( 1.0, 0.9, 0.1 ); // Slightly orange-tinted white
  float tint_intensity = 0.5;              // How strong the tint is

  // Calculate how much tint to apply (inverse of alpha - more tint where it's more transparent)
  float tint_amount = ( 1.0 - alpha ) * tint_intensity;

  // Mix warm white tint with black based on the aperture area
  vec3 color = mix( vec3( 0.0 ), warm_white, tint_amount );

  outColor = vec4( color, alpha );
  // }
}