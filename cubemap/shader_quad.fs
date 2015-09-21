#version 330
#include general.s
#define INTERSECTION_LIMIT 2 // what distance means intersection
#define NUMBER_OF_CUBEMAPS 2

in vec3 transformed_normal;
in vec4 transformed_position;
in vec2 uv_coords;

struct environment_cubemap
  {
    samplerCube texture_color;        // contains color
    samplerCube texture_position;     // contains world position
    vec3 position;                            // cubemap world position
  };

uniform environment_cubemap cubemaps[NUMBER_OF_CUBEMAPS];

uniform vec3 camera_position;
uniform int texture_to_display;      // which texture to display (1 = color, 2 = normal etc.)
uniform sampler2D texture_color;
uniform sampler2D texture_normal;
uniform sampler2D texture_position;
uniform sampler2D texture_stencil;

out vec4 fragment_color;

vec3 cube_coordinates1, cube_coordinates2, cube_coordinates_current;
vec3 position1_to_cube_center, position1_to_position2;
vec3 normal;
vec3 position1, position2;
vec3 reflection_vector;

float helper;
float distance;
float best_candidate_distance;
float interpolation_step;
float angle1, angle2, side1;
int i;

vec4 best_candidate_color;
vec3 tested_point, tested_point2;
vec3 camera_to_position1;

float distance_to_line(vec3 line_point1, vec3 line_point2, vec3 point)
  {
    /* this is too computationally intense, let's fix using this method by
       using "distance" texture instead of world-position texture and simply
       computing distance of two points in 3D */
    return length(cross(point - line_point1,point - line_point2)) / length(line_point2 - line_point1);
  }

float decide_interpolation_step(vec3 coordinates1, vec3 coordinates2)
  {
    float angle = dot(coordinates1,coordinates2);
    return 1.0 / (-255 * angle + 256);
  }
  
vec3 get_point_on_line_by_vector(vec3 vector)
  {
    float angle2 = acos(dot(normalize(vector),cube_coordinates1));
    float angle3 = 3.1415926535 - angle2 - angle1;
    float side2 = side1 * angle2 / angle3;
    return position1 + normalize(position1_to_position2) * side2;
  }
  
void main()
  {
    switch (texture_to_display)
        {
          case 1:
            if (texture(texture_stencil, uv_coords) == vec4(1,1,1,0))
              { // drawing mirror here
              
                best_candidate_distance = 99999999999.0;
                best_candidate_color = vec4(0.0,0.0,0.0,1.0);
       
                // we cannot use the texture(...) function because it requires implicit derivatives, we need to use textureLod(...)
                  
                normal = textureLod(texture_normal,uv_coords,0).xyz;
                position1 = textureLod(texture_position,uv_coords,0).xyz;
                camera_to_position1 = normalize(position1 - camera_position);
                reflection_vector = reflect(camera_to_position1,normal);
                position2 = position1 + reflection_vector * 1000;
                position1_to_position2 = position2 - position1;
                
                for (i = 0; i < NUMBER_OF_CUBEMAPS; i++)  // iterate through cubemaps
                  {
                    position1_to_cube_center = cubemaps[i].position - position1;
                    angle1 = acos(dot(normalize(position1_to_cube_center),normalize(position1_to_position2)));
                    side1 = length(position1_to_cube_center);               
                    cube_coordinates1 = normalize(position1 - cubemaps[i].position);
                    cube_coordinates2 = normalize(position2 - cubemaps[i].position);
                    interpolation_step = decide_interpolation_step(cube_coordinates1,cube_coordinates2);
      
                    for (helper = 0; helper <= 1.0; helper += interpolation_step)
                      {
                        cube_coordinates_current = mix(cube_coordinates1,cube_coordinates2,helper);
                        // we cannot use the texture(...) function because it requires implicit derivatives, we need to use textureLod(...)
                        
                        if (i == 0)   // for some reason cubemaps[i] causes and error - resolve this later
                          tested_point = textureLod(cubemaps[0].texture_position,cube_coordinates_current,0).xyz;
                        else
                          tested_point = textureLod(cubemaps[1].texture_position,cube_coordinates_current,0).xyz;
                        
                        tested_point2 = get_point_on_line_by_vector(cube_coordinates_current); 
                        distance = length(tested_point - tested_point2);
                  
                        if (distance < best_candidate_distance)
                          {
                            best_candidate_distance = distance;
                            
                            if (i == 0)
                              best_candidate_color = textureLod(cubemaps[0].texture_color,cube_coordinates_current,0);
                            else
                              best_candidate_color = textureLod(cubemaps[1].texture_color,cube_coordinates_current,0);
                              
                    
                            if (distance <= INTERSECTION_LIMIT)  // first hit -> stop
                              break;
                          }
                      }
                  }
                
                fragment_color = best_candidate_distance <= INTERSECTION_LIMIT ? best_candidate_color * 0.75 : vec4(1,0,0,1);
                //fragment_color = best_candidate_color * 0.75;
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
