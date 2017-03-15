#version 430

layout (local_size_x = 8, local_size_y = 1) in;

struct mirror_pixel
  {
    uint x;               
    uint y;        
    vec3 ray_position1;     
    vec3 ray_position2;     
  };
  
layout (std430, binding=1) buffer output_buffer_data   // for compute shaders
  {
    uint number_of_pixels;
    mirror_pixel pixels[];
  } mirror_pixel_buffer;
  
void main()
  {
  }