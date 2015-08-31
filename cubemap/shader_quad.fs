#version 330
#include general.s

in vec3 transformed_normal;
in vec4 transformed_position;
in vec2 uv_coords;

uniform samplerCube texture_cube1;
uniform samplerCube texture_cube2;
uniform samplerCube texture_cube_position1;
uniform samplerCube texture_cube_position2;
uniform vec3 cube_position1;
uniform vec3 cube_position2;

uniform vec3 camera_position;
uniform int texture_to_display;      // which texture to display (1 = color, 2 = normal etc.)
uniform sampler2D texture_color;
uniform sampler2D texture_normal;
uniform sampler2D texture_position;
uniform sampler2D texture_stencil;

out vec4 fragment_color;

vec3 cube_coordinates1, cube_coordinates2, cube_coordinates_current;
vec3 normal;
vec3 position1, position2;
vec3 reflection_vector;

float helper;
float distance;
float best_candidate_distance;
vec4 best_candidate_color;
vec3 tested_point;
vec3 camera_to_position1;

float distance_to_line(vec3 line_point1, vec3 line_point2, vec3 point)
{
  return abs(cross(point - line_point1,point - line_point2)) / abs(line_point2 - line_point1);
}

bool temp;

void main()
{
  switch (texture_to_display)
      {
        case 1:          
          best_candidate_distance = 99999999999.0;
          best_candidate_color = vec4(0.0,0.0,0.0,1.0);
            
          normal = texture(texture_normal,uv_coords).xyz;
          position1 = texture(texture_position,uv_coords).xyz;
          camera_to_position1 = normalize(position1 - camera_position);
          reflection_vector = reflect(camera_to_position1,normal);
          position2 = position1 + reflection_vector * 70;
        
          if (texture(texture_stencil, uv_coords) == vec4(1,1,1,0))
            { // drawing mirror here
              
              // iterate through first cubemap:
              cube_coordinates1 = normalize(position1 - cube_position1);
              cube_coordinates2 = normalize(position2 - cube_position1);
              
              
              for (helper = 0; helper <= 1.0; helper += 0.0025)
                {
                  cube_coordinates_current = mix(cube_coordinates1,cube_coordinates2,helper);
                  tested_point = textureLod(texture_cube_position1,cube_coordinates_current,0).xyz;
                  // we cannot use the texture(...) function because it requires implicit derivatives, we need to use textureLod(...)
                  distance = distance_to_line(position1,position2,tested_point);
                  
                  if (distance < best_candidate_distance)
                    {
                      best_candidate_distance = distance;
                      best_candidate_color = textureLod(texture_cube1,cube_coordinates_current,0);
                    }
                }
              
              fragment_color = best_candidate_color;
              //fragment_color = best_candidate_distance < 0.5 ? best_candidate_color : vec4(0,0,0,1);
              
              //temp = best_candidate_distance < 0.1;
              
              //fragment_color = vec4(map_minus_n_n_0_1(best_candidate_color.xyz,50),1.0);              
              //fragment_color = texture(texture_cube1,cube_coordinates1);      
              //fragment_color = vec4(reflection_vector,1.0);
              //fragment_color = texture(texture_cube1,reflection_vector);
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
