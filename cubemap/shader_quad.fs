#version 330
#include general.s
#define INTERSECTION_LIMIT 2   // what distance means intersection
#define NUMBER_OF_CUBEMAPS 2
#define ACCELERATION_LEVELS 9
#define USE_ACCELERATION_LEVELS 3 // how many levels in acceleration texture to use
#define INFINITY_T 999999         // infinite value for t (line parameter) 

in vec3 transformed_normal;
in vec4 transformed_position;
in vec2 uv_coords;

struct environment_cubemap
  {
    samplerCube texture_color;        // contains color
    samplerCube texture_distance;     // contains distance to cubemap center, this is NOT a depth texture
    vec3 position;                    // cubemap world position
  };

uniform environment_cubemap cubemaps[NUMBER_OF_CUBEMAPS];

uniform vec3 camera_position;
uniform int texture_to_display;       // which texture to display (1 = color, 2 = normal etc.)
uniform int acceleration_on;
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

float t;
float distance;
float best_candidate_distance;
float interpolation_step;
int i, j;

bool intersection_found;
bool skipped;

vec4 best_candidate_color;
vec3 tested_point2;
vec3 camera_to_position1;
vec2 min_max;

float next_acceleration_bounds[USE_ACCELERATION_LEVELS];   // contains a value of t of the next acceleration boundary for each acceleration level used

float get_plane_line_intersection_parametric(vec3 line_point1, vec3 line_point2, vec3 plane_point1, vec3 plane_point2, vec3 plane_point3)
  {
    // a, b, c:
    vec3 plane_normal = normalize(cross(plane_point2 - plane_point1,plane_point3 - plane_point1));    
    
    vec3 line_direction_vector = line_point2 - line_point1;
    // d:
    float d = plane_normal.x * plane_point1.x + plane_normal.y * plane_point1.y + plane_normal.z * plane_point1.z;
    float t = (d - line_point1.x * plane_normal.x - line_point1.y * plane_normal.y - line_point1.z * plane_normal.z) / (line_direction_vector.x * plane_normal.x + line_direction_vector.y * plane_normal.y + line_direction_vector.z * plane_normal.z);

    return t;
  }

float correct_intersection(vec2 correct_values_interval, float t)
  {
    if (t > correct_values_interval.y)
      return -1 * INFINITY_T;
    else if (t < correct_values_interval.x)
      return INFINITY_T;
    
    return t;
  }

float get_distance_to_center(int cubemap_index, vec3 cubemap_coordinates)
  {
    float distance_to_center;
  
    // note: the reason is explained here - https://www.opengl.org/discussion_boards/showthread.php/171328-Issues-with-sampler-arrays-and-unrolling
    if (i == 0)   // for some reason cubemaps[i] causes and error - resolve this later
      distance_to_center = textureLod(cubemaps[0].texture_distance,cubemap_coordinates,0).x;
    else
      distance_to_center = textureLod(cubemaps[1].texture_distance,cubemap_coordinates,0).x;

    return distance_to_center; 
  }
  
vec2 get_next_prev_acceleration_bound(vec3 cubemap_center, vec3 line_point1, vec3 line_point2, int current_side, int current_x, int current_y, int level)
  {
    vec3 vector_start, vector_right, vector_up, forward_vector;
    
    vec2 correct_t_values;           // interval of t values for valid intersections
    
    switch (current_side)
      {
        case 0: vector_start = vec3(0.5,-0.5,-0.5); vector_right = vec3(-1,0,0); vector_up = vec3(0,1,0); forward_vector = vec3(0,0,-1); break;  // front
        case 1: vector_start = vec3(-0.5,-0.5,0.5); vector_right = vec3(1,0,0); vector_up = vec3(0,1,0); forward_vector = vec3(0,0,1); break;    // back
        case 2: vector_start = vec3(-0.5,-0.5,-0.5); vector_right = vec3(0,0,1); vector_up = vec3(0,1,0); forward_vector = vec3(-1,0,0); break;  // left
        case 3: vector_start = vec3(0.5,-0.5,0.5); vector_right = vec3(0,0,-1); vector_up = vec3(0,1,0); forward_vector = vec3(1,0,0); break;    // right
        case 4: vector_start = vec3(-0.5,0.5,0.5); vector_right = vec3(1,0,0); vector_up = vec3(0,0,-1); forward_vector = vec3(0,1,0); break;    // top
        case 5: vector_start = vec3(-0.5,-0.5,-0.5); vector_right = vec3(1,0,0); vector_up = vec3(0,0,1); forward_vector = vec3(0,-1,0); break;  // bottom
        default: break;
      }
   
    // compute interval of correct t valus:
   
    float helper_t = get_plane_line_intersection_parametric(line_point1,line_point2,cubemap_center,cubemap_center + vector_right,cubemap_center + vector_up);   // intersection with critical plane

    if (isinf(helper_t))  // parallel
      correct_t_values = vec2(-1 * INFINITY_T,INFINITY_T);
    else if (dot(line_point2 - line_point1,forward_vector) > 0)
      correct_t_values = vec2(helper_t,INFINITY_T);      // greater t gets us further away from the critical plane
    else
      correct_t_values = vec2(-1 * INFINITY_T,helper_t);
    
    vec3 start = cubemap_center + vector_start;
  
    float level_step =  1 / pow(2,level);
    
    vec3 point1 = cubemap_center + vector_start + vector_right * (level_step * current_x) + vector_up * (level_step * current_y);
    vec3 point2 = cubemap_center + vector_start + vector_right * (level_step * (current_x + 1)) + vector_up * (level_step * current_y);
    vec3 point3 = cubemap_center + vector_start + vector_right * (level_step * current_x) + vector_up * (level_step * (current_y + 1));
    vec3 point4 = cubemap_center + vector_start + vector_right * (level_step * (current_x + 1)) + vector_up * (level_step * (current_y + 1));
     
    float t1 = correct_intersection(correct_t_values,get_plane_line_intersection_parametric(line_point1,line_point2,cubemap_center,point1,point2));
    float t2 = correct_intersection(correct_t_values,get_plane_line_intersection_parametric(line_point1,line_point2,cubemap_center,point3,point4));
    float t3 = correct_intersection(correct_t_values,get_plane_line_intersection_parametric(line_point1,line_point2,cubemap_center,point1,point3));
    float t4 = correct_intersection(correct_t_values,get_plane_line_intersection_parametric(line_point1,line_point2,cubemap_center,point2,point4));

    float prev_t = max(min(t1,t2),min(t3,t4));
    float next_t = min(max(t1,t2),max(t3,t4));
    
    return vec2(next_t,prev_t);
  }
  
// Gets a (min,max) value from the acceleration texture. Level starts with 0 for the 1x1 resolution, every next level is 4 times bigger.

vec2 get_acceleration_pixel(int texture_index, int side, vec2 coordinates, int level)
  {
    float x = fract(coordinates.x); 
    float y = 1.0 - fract(coordinates.y); 
    float offset_x, offset_y;                  // offset for given side
    
    level = ACCELERATION_LEVELS - level - 1;
    
    switch (side)
      {
        case 0: offset_x = 0.0;       offset_y = 0.0;   break;  // front
        case 1: offset_x = 1/3.0;     offset_y = 0.0;   break;  // back
        case 2: offset_x = 2/3.0;     offset_y = 0.0;   break;  // left
        case 3: offset_x = 0.0;       offset_y = 1/2.0; break;  // right
        case 4: offset_x = 1/3.0;     offset_y = 1/2.0; break;  // top
        case 5: offset_x = 2/3.0;     offset_y = 1/2.0; break;  // bottom
        default: break;
      }
     
    // this could be fetched from a constant array maybe, to save time:
    
    float half_to = 1.0/pow(2,level + 1);
    vec2 relative_level_resolution = vec2(1/3.0 * half_to, 1/2.0 * half_to);
  
    vec2 coordinates_start_max = vec2(offset_x + relative_level_resolution.x,offset_y);
    vec2 coordinates_start_min = vec2(coordinates_start_max.x,coordinates_start_max.y + 0.25);
 
    vec2 relative_coordinates = vec2(x * relative_level_resolution.x,y * relative_level_resolution.y);
    
    vec2 texture_size = textureSize(acceleration_textures[texture_index],0);
    
    ivec2 coordinates_min = ivec2((coordinates_start_min + relative_coordinates) * texture_size - vec2(1,0));  // vec2(1,0) is a pixel offset to correct pixel sampling
    ivec2 coordinates_max = ivec2((coordinates_start_max + relative_coordinates) * texture_size - vec2(1,0));
    
    vec2 result;
    
    // can't index the sampler array with non-constants for the same reason as in get_distance_to_center
    if (texture_index == 0)    
      result = vec2
        (
          texelFetch(acceleration_textures[0],coordinates_min,0).x,
          texelFetch(acceleration_textures[0],coordinates_max,0).x
        );
    else
      result = vec2
        (
          texelFetch(acceleration_textures[1],coordinates_min,0).x,
          texelFetch(acceleration_textures[1],coordinates_max,0).x
        );
      
    return result;
  }
  
vec2 cubemap_side_coordinates(float right, float up, float forward)
  {
    return vec2(0.5 + 0.5 * right / forward, 0.5 + 0.5 * up / forward);
  }
  
// from http://www.nvidia.com/object/cube_map_ogl_tutorial.html, returns vec3 (x,y,side)
  
vec3 cubemap_coordinates_to_2D_coordinates(vec3 cubemap_coordinates)
  {
    vec3 result = vec3(0,0,0);
    vec3 abs_coordinates= vec3(abs(cubemap_coordinates.x),abs(cubemap_coordinates.y),abs(cubemap_coordinates.z));
  
    cubemap_coordinates = normalize(cubemap_coordinates);
  
    float maximum_axis_value = max(max(abs_coordinates.x,abs_coordinates.y),abs_coordinates.z);
  
    if (maximum_axis_value == abs_coordinates.x)
      {
        if (cubemap_coordinates.x > 0)
          {
            result.xy = cubemap_side_coordinates(-1 * cubemap_coordinates.z,cubemap_coordinates.y,cubemap_coordinates.x);
            result.z = 3;
          }
        else
          {
            result.xy = cubemap_side_coordinates(cubemap_coordinates.z,cubemap_coordinates.y,-1 * cubemap_coordinates.x);
            result.z = 2;
          }
      }
    else if (maximum_axis_value == abs_coordinates.y)
      {
        if (cubemap_coordinates.y > 0)
          {
            result.xy = cubemap_side_coordinates(cubemap_coordinates.x,-1 * cubemap_coordinates.z,cubemap_coordinates.y);
            result.z = 4;
          }
        else
          {
            result.xy = cubemap_side_coordinates(-1 * cubemap_coordinates.x,-1 * cubemap_coordinates.z,cubemap_coordinates.y);
            result.z = 5;
          }
      }
    else if (maximum_axis_value == abs_coordinates.z)
      {
        if (cubemap_coordinates.z > 0)
          {
            result.xy = cubemap_side_coordinates(-1 * cubemap_coordinates.x,-1 * cubemap_coordinates.y,-1 * cubemap_coordinates.z);
            result.z = 1;
          }
        else
          {
            result.xy = cubemap_side_coordinates(cubemap_coordinates.x,-1 * cubemap_coordinates.y,cubemap_coordinates.z);
            result.z = 0;
          }
      }
  
    return result;
  }
  
void main()
  {
    switch (texture_to_display)
        {
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
            if (texture(texture_stencil, uv_coords) == vec4(1,1,1,0))
              { 
                // drawing mirror here              
                
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
                  
                int iteration_counter = 0;
                int skip_counter = 0;
              
                if (texture_to_display == 5)   // debugging code, displays some information in the mirror pixel
                  {
                    vec3 helper_coords = cubemap_coordinates_to_2D_coordinates(position1 - cubemaps[0].position);
                    float helper_intensity = get_acceleration_pixel(0,int(helper_coords.z),helper_coords.xy,2).x / 1000.0;
                    float helper_distance = get_distance_to_center(0,position1 - cubemaps[0].position) / 1000.0;
                   
                    // uncomment one of following
                    
                    // --- acceleration texture ---
                    fragment_color = vec4(helper_intensity,helper_intensity,helper_intensity,0);
                    
                    // --- acceleration on ---
                    //fragment_color = vec4(float(acceleration_on),float(acceleration_on),float(acceleration_on),0);
                    
                    // --- cubemap coordinates ---
                    //fragment_color = vec4(helper_coords.x,helper_coords.y,helper_coords.z,0);    
                    
                    // --- distance to center ---
                    //fragment_color = vec4(helper_distance,helper_distance,helper_distance,0);
                    
                    break;
                  }
              
                for (i = 0; i < 1/*NUMBER_OF_CUBEMAPS*/; i++)  // iterate the cubemaps
                  {
                    position1_to_cube_center = cubemaps[i].position - position1;
                    cube_coordinates1 = normalize(position1 - cubemaps[i].position);
                    cube_coordinates2 = normalize(position2 - cubemaps[i].position);   
                    interpolation_step = 0.001;
   
                    for (j = 0; j < USE_ACCELERATION_LEVELS; j++)
                      next_acceleration_bounds[j] = -1;
   
                    t = 0.0;
                    
                    while (t <= 1.0) // trace the ray
                      {
                        t += interpolation_step;
                      
                        tested_point2 = mix(position1,position2,t);
                        cube_coordinates_current = normalize(tested_point2 - cubemaps[i].position);
             
                        // ==== ACCELERATION CODE HERE:
                        
                        skipped = false;

                        iteration_counter += 1;

                        if (iteration_counter > 100000)      // prevent the forever loop in case of bugs
                          {
                            break;
                          }
                          
                        for (j = 0; j < USE_ACCELERATION_LEVELS; j++)
                          {
                            if (acceleration_on < 1)
                              break;
                          
                            if (t > next_acceleration_bounds[j])
                              {
                                vec3 helper_coords = cubemap_coordinates_to_2D_coordinates(cube_coordinates_current);
                                ivec2 int_coordinates;
                                
                                float level_step = 1 / pow(2,j);
                                
                                int_coordinates = ivec2(int(helper_coords.x / level_step),int(helper_coords.y / level_step));
                                
                                vec2 helper_bounds = get_next_prev_acceleration_bound(
                                  cubemaps[i].position,
                                  position1,
                                  position2,
                                  int(helper_coords.z),
                                  int_coordinates.x,
                                  int_coordinates.y,
                                  j
                                  );
                                  
                                // check if intersection can happen:
                                
                                vec2 min_max = get_acceleration_pixel(i,int(helper_coords.z),helper_coords.xy,j);
                                
                                vec3 cubemap_coordinates_next = mix(position1,position2,helper_bounds.x);
                                vec3 cubemap_coordinates_previous = mix(position1,position2,helper_bounds.y);
                                
                                float depth_next = get_distance_to_center(i,cubemap_coordinates_next);
                                float depth_previous = get_distance_to_center(i,cubemap_coordinates_previous);

                                if
                                  (
                                    (min_max.y < depth_next && min_max.y < depth_previous)  ||
                                    (min_max.x > depth_next && min_max.x > depth_previous)    
                                  )
                                  {
                                    skip_counter += 1;

                                    t = helper_bounds.x;  // jump to the next bound
                                              
                                    skipped = true;
                                    break;
                                  }
                                else
                                  next_acceleration_bounds[j] = helper_bounds.x + interpolation_step;
                              }
                          }
                           
                        if (skipped)
                          continue;
                          
                        // ==== END OF ACCELERATION CODE
               
                        distance = abs(get_distance_to_center(i,cube_coordinates_current) - length(cubemaps[i].position - tested_point2));
               
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
                
                fragment_color = best_candidate_distance <= INTERSECTION_LIMIT ? best_candidate_color * 0.75 : vec4(1,0,0,1);
                
                if (texture_to_display == 6)   // debugging code 2, displays some information computed after tracing
                  {
                    float iterations_float = iteration_counter / 10000.0;
                    
                    fragment_color = vec4(iterations_float,iterations_float,iterations_float,0);
                  }
                
              }
            else
              fragment_color = texture(texture_color, uv_coords);
          
            break;  
        }
  }