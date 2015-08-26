#version 330

in vec3 transformed_normal;
in vec4 transformed_position;
in vec2 uv_coords;

uniform samplerCube texture_cube1;
uniform samplerCube texture_cube2;

uniform samplerCube texture_cube_depth1;
uniform samplerCube texture_cube_depth2;

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

float helper;

void main()
{
  switch (texture_to_display)
      {
        case 1:
          if (texture(texture_stencil, uv_coords) == vec4(1,1,1,0))
            { // drawing mirror here
              normal = texture(texture_normal,uv_coords).xyz;
              position = texture(texture_position,uv_coords).xyz;
              cube_coordinates = reflect(position - camera_position,normalize(normal));
              // cube_coordinates = normalize(normal);
              
              helper = pow(texture(texture_cube_depth1,cube_coordinates).x,256);
              
              fragment_color = vec4(helper,helper,helper,1);
              
              //fragment_color = 
              //  mix
              //    (
              //      texture(texture_cube1,cube_coordinates),
              //      texture(texture_cube2,cube_coordinates),
              //      0.5
              //    );
            }
          else
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
