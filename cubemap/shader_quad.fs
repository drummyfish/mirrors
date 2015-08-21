#version 330

in vec3 transformed_normal;
in vec4 transformed_position;
in vec2 uv_coords;

uniform samplerCube texture_cube;
uniform vec3 camera_position;
uniform int texture_to_display;      // which texture to display (1 = color, 2 = normal etc.)
uniform sampler2D texture_color;
uniform sampler2D texture_normal;
uniform sampler2D texture_position;
uniform sampler2D texture_stencil;

out vec4 fragment_color;

vec3 cube_coordinates;
vec3 normal;
vec3 position;

vec3 map_0_1_minus_n_n(vec3 input_vec, float n)
  {
    return (input_vec - vec3(0.5,0.5,0.5)) * 2 * n;
  }

vec3 map_minus_n_n_0_1(vec3 input_vec, float n)   // for mapping <-n,n> to <0,1> due to OpenGL clamping texture values
  {
    return (input_vec / n + vec3(1.0,1.0,1.0)) / 2.0;
  }

void main()
{
  if (texture(texture_stencil, uv_coords) == vec4(1,1,1,0))
    {
      normal = map_0_1_minus_n_n(texture(texture_normal, uv_coords).xyz,1.0);
      position = map_0_1_minus_n_n(texture(texture_position, uv_coords).xyz,16);
      cube_coordinates = reflect(position - camera_position,normalize(normal));
     // cube_coordinates = normalize(normal);
      fragment_color = texture(texture_cube,cube_coordinates);
    }
  else
    switch (texture_to_display)
      {
        case 1:
          fragment_color = texture(texture_color, uv_coords);
          break;  
        
        case 2:
          fragment_color = texture(texture_normal, uv_coords);
          break;
          
        case 3:
          fragment_color = texture(texture_position, uv_coords);
          break;
          
        case 4:
          fragment_color = texture(texture_stencil, uv_coords);
          break;
        
        default:
          fragment_color = texture(texture_color, uv_coords);
          break;
      }
}
