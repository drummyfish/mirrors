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
    samplerCube texture_distance;     // contains distance to cubemap center
    vec3 position;                    // cubemap world position
  };

uniform environment_cubemap cubemaps[NUMBER_OF_CUBEMAPS];

uniform vec3 camera_position;
uniform int texture_to_display;      // which texture to display (1 = color, 2 = normal etc.)
uniform sampler2D texture_color;
uniform sampler2D texture_normal;
uniform sampler2D texture_position;
uniform sampler2D texture_stencil;

uniform sampler2D acceleration_textures[NUMBER_OF_CUBEMAPS];

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

bool intersection_found;

vec4 best_candidate_color;
vec3 distance_to_center;      // fragment (texel) distance to cubemap center 
vec3 tested_point2;
vec3 camera_to_position1;
vec2 min_max;

// Gets a (min,max) value from the acceleration texture. Level starts with 0 for the highest resolution.

vec2 get_acceleration_pixel(int texture_index, int side, vec2 coordinates, int level)
  {
    float x = fract(coordinates.x); 
    float y = 1.0 - fract(coordinates.y); 
    float offset_x, offset_y;                  // offset for given side
    
    switch (side)
      {
        case 0: offset_x = 0.0;       offset_y = 0.0;   break;  // front
        case 1: offset_x = 1/3.0;     offset_y = 0.0;   break;  // back
        case 2: offset_x = 2/3.0;     offset_y = 0.0;   break;  // left
        case 3: offset_x = 0.0;       offset_y = 1/3.0; break;  // right
        case 4: offset_x = 1/3.0;     offset_y = 1/3.0; break;  // top
        case 5: offset_x = 2/3.0;     offset_y = 1/3.0; break;  // bottom
        default: break;
      }
     
    // this could be fetched from a constant array maybe, to save time:
    
    float half_to = 1.0/pow(2,level + 1);
    vec2 relative_level_resolution = vec2(1/3.0 * half_to, 1/2.0 * half_to);
  
    vec2 coordinates_start_max = vec2(offset_x + relative_level_resolution.x,offset_y);
    vec2 coordinates_start_min = vec2(coordinates_start_max.x,coordinates_start_max.y + 0.5);
 
    vec2 relative_coordinates = vec2(x * relative_level_resolution.x,y * relative_level_resolution.y);
    
    vec2 texture_size = textureSize(acceleration_textures[texture_index],0);
    
    ivec2 coordinates_min = ivec2((coordinates_start_min + relative_coordinates) * texture_size);
    ivec2 coordinates_max = ivec2((coordinates_start_max + relative_coordinates) * texture_size);
    
    
    return vec2
      (
        texelFetch(acceleration_textures[texture_index],coordinates_min,0).x,
        texelFetch(acceleration_textures[texture_index],coordinates_max,0).x
      );
  }

// from http://www.nvidia.com/object/cube_map_ogl_tutorial.html
  
vec3 cubemap_coordinates_to_2D_coordinates(vec2 cubemap_coordinates)
  {
    return vec3(1,1,1);
  }
  
// Same as get_acceleration_pixel but coordinates are cubemap coordinates.

vec2 get_acceleration_pixel_by_cubemap_coordinates(int texture_index, vec2 cubemap_coordinates ,int level)
  {
    return vec2(1.0,0.5);
  }
  
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
                
                intersection_found = false;
                
                for (i = 0; i < NUMBER_OF_CUBEMAPS; i++)  // iterate through cubemaps
                  {
                    position1_to_cube_center = cubemaps[i].position - position1;
                    angle1 = acos(dot(normalize(position1_to_cube_center),normalize(position1_to_position2)));
                    side1 = length(position1_to_cube_center);               
                    cube_coordinates1 = normalize(position1 - cubemaps[i].position);
                    cube_coordinates2 = normalize(position2 - cubemaps[i].position);   
                    interpolation_step = 0.001;
   
                    for (helper = 0; helper <= 1.0; helper += interpolation_step)
                      {                   
                        tested_point2 = mix(position1,position2,helper);
                        cube_coordinates_current = normalize(tested_point2 - cubemaps[i].position);
             
                        if (i == 0)   // for some reason cubemaps[i] causes and error - resolve this later
                          distance_to_center = textureLod(cubemaps[0].texture_distance,cube_coordinates_current,0).xyz;
                        else
                          distance_to_center = textureLod(cubemaps[1].texture_distance,cube_coordinates_current,0).xyz;
                        
                        distance = abs(distance_to_center.x - length(cubemaps[i].position - tested_point2));
                  
                        if (distance < best_candidate_distance)
                          {
                            best_candidate_distance = distance;
                            
                            if (i == 0)
                              best_candidate_color = textureLod(cubemaps[0].texture_color,cube_coordinates_current,0);
                            else
                              best_candidate_color = textureLod(cubemaps[1].texture_color,cube_coordinates_current,0);
                              
                            if (distance <= INTERSECTION_LIMIT)  // first hit -> stop
                              {
                                intersection_found = true;
                                break;
                              }
                          }
                      }
                      
                    if (intersection_found)
                      break;
                  }
                
                fragment_color =
                (
                best_candidate_distance <= INTERSECTION_LIMIT ? best_candidate_color * 0.75 : vec4(1,0,0,1)
                )
                + 0.001 * vec4(texture(acceleration_textures[0],uv_coords).x) + 0.001 * vec4(texture(acceleration_textures[1],uv_coords).x);
                
                // TEMP
                float coooool = get_acceleration_pixel(0,0,position1.xy / 10.0,4).x; //    texture(acceleration_textures[0],position1.xy / 20.0).x;
                fragment_color = 0.01 * fragment_color + vec4(coooool,coooool,coooool,0); //vec4(get_acceleration_pixel(0,0,uv_coords.x,uv_coords.y,0),0,0);
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