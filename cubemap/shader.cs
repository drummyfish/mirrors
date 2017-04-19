#version 430
#include ../shader_log_include.txt

layout (local_size_x = 8, local_size_y = 4) in;

// 32 (kvuli ballotu) x 8

layout(rgba32f, binding = 0) uniform writeonly image2D image_color;

struct mirror_pixel
  {
    uint x;               
    uint y;        
    vec3 ray_position1;     
    vec3 ray_position2;     
  };
  
layout (std430, binding=1) buffer input_buffer_data   // for compute shaders
  {
    uint number_of_pixels;
    mirror_pixel pixels[];
  } mirror_pixel_buffer;
  
void main()
  {
    mirror_pixel my_pixel = mirror_pixel_buffer.pixels[gl_WorkGroupID[0]];
    shader_log_write_int(3);
    imageStore(image_color,ivec2(my_pixel.x,my_pixel.y),vec4(1,0,0,1));
  }
