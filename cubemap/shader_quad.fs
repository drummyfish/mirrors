#version 430
#include ../shader_log_include.txt
#define INTERSECTION_LIMIT 1.5          // what distance means intersection, aplies only if ANALYTICAL_INTERSECTION is not defined
#define NUMBER_OF_CUBEMAPS 2
#define ACCELERATION_LEVELS 9
#define ACCELERATION_MIPMAP_LEVELS 9
#define USE_ACCELERATION_LEVELS 3     // how many levels in acceleration texture to use
#define INFINITY_T 999999             // infinite value for t (line parameter) 

#define INTERPOLATION_STEP 0.0005

#define ITERATION_LIMIT 1000000       // to avoid infinite loops etc.

//#define EFFECTIVE_SAMPLING          // sample each pixel at most once

#define DISABLE_ACCELERATION

//#define ANALYTICAL_INTERSECTION     // this switches between analytical and more precise sampling intersection decision

#define SELF_REFLECTIONS
#define SELF_REFLECTIONS_LIMIT 2
#define SELF_REFLECTIONS_BIAS  0.001  // these are unfortunately dependent on cubemap positions very much
#define SELF_REFLECTIONS_BIAS2 0.002

//#define NO_LOG

in vec3 transformed_normal;
in vec4 transformed_position;
in vec2 uv_coords;

struct environment_cubemap
  {
    samplerCube texture_color;        // contains color
    samplerCube texture_distance;     // contains distance to cubemap center, this is NOT a depth texture
    samplerCube texture_normal;
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

out vec4 fragment_color;

vec3 cube_coordinates1, cube_coordinates2, cube_coordinates_current;
vec3 position1_to_cube_center, position1_to_position2;
vec3 normal;
vec3 position1, position2;
vec3 reflection_vector;

float t;
float distance, distance_prev;
float best_candidate_distance;
int i, j;

bool intersection_found;
bool intersection_on_mirror;
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

ivec2 normalized_coords_to_int_coords(vec2 normalized_coords, int level)   // converts normalized cubemap side coords (uv) to 2D integer coordinates within given acceleration level
  {
    float level_step = 1 / pow(2,level);
    return ivec2(int(normalized_coords.x / level_step),int(normalized_coords.y / level_step)); 
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
  
vec4 sample_color(int cubemap_index, vec3 cubemap_coordinates)
  {
    if (cubemap_index == 0)  // for some reason we can only index with constants
      return textureLod(cubemaps[0].texture_color,cubemap_coordinates,0);
    else
      return textureLod(cubemaps[1].texture_color,cubemap_coordinates,0);
  }
 
vec3 sample_normal(int cubemap_index, vec3 cubemap_coordinates)
  {
    if (cubemap_index == 0)  // for some reason we can only index with constants
      return textureLod(cubemaps[0].texture_normal,cubemap_coordinates,0).xyz;
    else
      return textureLod(cubemaps[1].texture_normal,cubemap_coordinates,0).xyz;
  }
 
bool sample_mirror_mask(int cubemap_index, vec3 cubemap_coordinates)
  {
    if (cubemap_index == 0)  // for some reason we can only index with constants
      return textureLod(cubemaps[0].texture_distance,cubemap_coordinates,0).z > 50;
    else
      return textureLod(cubemaps[1].texture_distance,cubemap_coordinates,0).z > 50;
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
  
vec2 get_acceleration_pixel(int texture_index, vec3 cube_coordinates, int level)
  {
    vec2 result;
    
    level = ACCELERATION_MIPMAP_LEVELS - level;

    if (texture_index == 0)
      result = textureLod(cubemaps[0].texture_distance,cube_coordinates,level).xy;
    else
      result = textureLod(cubemaps[1].texture_distance,cube_coordinates,level).xy;    

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

#ifndef NO_LOG
ivec2 log_coord = ivec2(170,160);
bool do_log =
  gl_FragCoord.x < (log_coord.x + 1) &&
  gl_FragCoord.x > log_coord.x &&
  gl_FragCoord.y > log_coord.y &&
  gl_FragCoord.y < (log_coord.y + 1);
#endif
  
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
                int mirror_bounce_counter = 0;
                int skip_counter = 0;
                bool assertion = true;
                vec3 debug_vector = vec3(0,0,0);

                if (texture_to_display == 5)   // debugging code, displays some information in the mirror pixel
                  {
                    vec3 helper_coords = cubemap_coordinates_to_2D_coordinates(position1 - cubemaps[0].position);
                    vec2 helper_min_max = get_acceleration_pixel(0,position1 - cubemaps[0].position,4) / 1000.0;
                    float helper_distance = get_distance_to_center(0,position1 - cubemaps[0].position) / 1000.0;
                    bool helper_mask = sample_mirror_mask(0,position1 - vec3(0,30,-30));
                    vec4 helper_color = sample_color(0,position1 - vec3(0,30,-30));
                    vec3 helper_normal = sample_normal(0,position1 - vec3(0,30,-30));
                   
                    // uncomment one of following
                    
                    // --- color ---
                    //fragment_color = helper_color;
                    
                    // -- normal ----
                    //fragment_color = vec4(helper_normal,0);
                    
                    // --- acceleration texture max ---
                    //fragment_color = vec4(helper_min_max.y,helper_min_max.y,helper_min_max.y,0);
                    
                    // --- acceleration texture max - min ---
                    //fragment_color = vec4(helper_min_max.y - helper_min_max.x,helper_min_max.y - helper_min_max.x,helper_min_max.y - helper_min_max.x,0);
                   
                    // --- acceleration on ---
                    //fragment_color = vec4(float(acceleration_on),float(acceleration_on),float(acceleration_on),0);
                    
                    // --- cubemap coordinates ---
                    //fragment_color = vec4(helper_coords.x,helper_coords.y,helper_coords.z,0);    
                    
                    // --- distance to center ---
                    //fragment_color = vec4(helper_distance,helper_distance,helper_distance,0);
                    
                    // --- mirror mask ---
                    //fragment_color = vec4(helper_mask ? 1 : 0,helper_mask ? 1 : 0,helper_mask ? 1 : 0,0);
                    
                    break;
                  }
              
                for (int self_reflection_count = 0; self_reflection_count < SELF_REFLECTIONS_LIMIT; self_reflection_count++)
                  {
                    
#ifndef NO_LOG
if (do_log)
  {
    shader_log_write_char(65);
  }
#endif   
                  
                    for (i = 0; i < NUMBER_OF_CUBEMAPS; i++)  // iterate the cubemaps
                      {   
                                     
                        position1_to_position2 = position2 - position1;
                        position1_to_cube_center = cubemaps[i].position - position1;
                        cube_coordinates1 = normalize(position1 - cubemaps[i].position);
                        cube_coordinates2 = normalize(position2 - cubemaps[i].position);
              
                        for (j = 0; j < USE_ACCELERATION_LEVELS; j++)
                          next_acceleration_bounds[j] = -1;
   
                        #ifdef SELF_REFLECTIONS
                          t = mix(SELF_REFLECTIONS_BIAS2,SELF_REFLECTIONS_BIAS,dot(normalize(position1_to_position2),normalize(camera_to_position1))  )    ;
                        #else
                          //t = SELF_REFLECTIONS_BIAS;
                          t = 0;
                        #endif
                 
                        bool first_iteration = true;

                        while (t <= 1.0) // trace the ray
                          {
                            // decide the interpolation step:
#ifndef NO_LOG
if (do_log)
  {
    shader_log_write_float(t);
  }
#endif                              
                            #ifndef EFFECTIVE_SAMPLING
                              t += INTERPOLATION_STEP;
                            #else
                              // TODO
                              vec3 helper_coords = cubemap_coordinates_to_2D_coordinates(cube_coordinates_current);
                              ivec2 int_coordinates = normalized_coords_to_int_coords(helper_coords.xy,ACCELERATION_MIPMAP_LEVELS);
                              vec2 helper_bounds = get_next_prev_acceleration_bound(
                                cubemaps[i].position,
                                position1,
                                position2,
                                int(helper_coords.z),
                                int_coordinates.x,
                                int_coordinates.y,
                                ACCELERATION_MIPMAP_LEVELS
                                );  
                          
                              vec2 min_max = get_acceleration_pixel(i,cube_coordinates_current,ACCELERATION_MIPMAP_LEVELS);                          
                          
                              vec3 position_next = mix(position1,position2,helper_bounds.x);
                              vec3 position_previous = mix(position1,position2,helper_bounds.y);
                                
                              float distance_next = length(position_next - cubemaps[i].position);
                              float distance_previous = length(position_previous - cubemaps[i].position);
                          
                              float distance2 = abs(distance - distance_next);
                          
                              t = helper_bounds.x > t ? helper_bounds.x + 0.0001 : t + INTERPOLATION_STEP;
                            #endif
                      
                            iteration_counter += 1;
                      
                            tested_point2 = mix(position1,position2,t);
                            cube_coordinates_current = normalize(tested_point2 - cubemaps[i].position);
             
                            // ==== ACCELERATION CODE HERE:
                            #ifndef DISABLE_ACCELERATION
                            skipped = false;

                            if (iteration_counter > ITERATION_LIMIT)      // prevent the forever loop in case of bugs
                              {
                                break;
                              }
                         
                            for (j = 1; j < USE_ACCELERATION_LEVELS; j++)
                              {
                                if (acceleration_on < 1)
                                  break;
                          
                                if (t > next_acceleration_bounds[j])
                                  {
                                    vec3 helper_coords = cubemap_coordinates_to_2D_coordinates(cube_coordinates_current);
                                
                                    ivec2 int_coordinates = normalized_coords_to_int_coords(helper_coords.xy,j);
                                
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
                                
                                    vec2 min_max = get_acceleration_pixel(i,cube_coordinates_current,j) + vec2(-1 * INTERSECTION_LIMIT,INTERSECTION_LIMIT);                          
                                    vec3 position_next = mix(position1,position2,helper_bounds.x);
                                    vec3 position_previous = mix(position1,position2,helper_bounds.y);
                                
                                    float distance_next = length(position_next - cubemaps[i].position);
                                    float distance_previous = length(position_previous - cubemaps[i].position);

                                    if
                                      (
                                        (min_max.y < distance_next && min_max.y < distance_previous)  ||
                                        (min_max.x > distance_next && min_max.x > distance_previous)    
                                      )
                                      {
                                        skip_counter += 1;
                                        t = helper_bounds.x;  // jump to the next bound
                                        skipped = true;
                                        break;
                                      }
                                    else
                                      next_acceleration_bounds[j] = helper_bounds.x + INTERPOLATION_STEP;
                                  }
                              }
                           
                            if (skipped)
                              continue;
                            #endif
                            // ==== END OF ACCELERATION CODE
               
                            // intersection decision:
                  
                            #ifndef ANALYTICAL_INTERSECTION
                              distance = abs(get_distance_to_center(i,cube_coordinates_current) - length(cubemaps[i].position - tested_point2));
               
                              if (distance < best_candidate_distance)
                                {
                                  best_candidate_distance = distance;
                            
                                  best_candidate_color = sample_color(i,cube_coordinates_current);
                              
                                  if (distance <= INTERSECTION_LIMIT)  // first hit -> stop
                                    {
                                      intersection_found = true;
           
                                      intersection_on_mirror = sample_mirror_mask(i,cube_coordinates_current);           
                                      break;
                                    }
                                }
                            #else
                              distance = get_distance_to_center(i,cube_coordinates_current) - length(cubemaps[i].position - tested_point2);
               
                              if (first_iteration)
                                {
                                  first_iteration = false;
                                }
                              else
                                {
                                  if (distance_prev * distance <= 0)
                                    {
                                      best_candidate_distance = distance;
                                      best_candidate_color = sample_color(i,cube_coordinates_current);
                                      intersection_found = true;
                                    
                                      intersection_on_mirror = sample_mirror_mask(i,cube_coordinates_current);
                                      
                                      break;
                                    }
                                }
                          
                              distance_prev = distance;
                            #endif
              
                          }
                  
                        #ifndef SELF_REFLECTIONS
                        intersection_on_mirror = false;
                        #endif
                  
                        if (intersection_found)
                          {
                            break;
                          }
                      } // for self reflection count
                      
                    if (!intersection_on_mirror)
                      {
                        break;
                      }
                    else
                      {
                        mirror_bounce_counter += 1;
                        position1 = tested_point2;
                        normal = sample_normal(i,cube_coordinates_current);
                        reflection_vector = reflect(normalize(position1_to_position2),normal);
                        position2 = position1 + reflection_vector * 1000;
                        position1_to_position2 = position2 - position1;
                        intersection_found = false;
                      }
                  } // for each cubemap
                
                #ifdef SELF_REFLECTIONS
                if (intersection_on_mirror)
                  fragment_color = vec4(1,0,0,1);
                else
                #endif
                  fragment_color = best_candidate_distance <= INTERSECTION_LIMIT ? best_candidate_color * (0.75 - mirror_bounce_counter * 0.2) : vec4(1,0,0,1);

#ifndef NO_LOG
if (do_log)
  {
    fragment_color = vec4(0,0,0,0);
  }
#endif
                
                if (texture_to_display == 6)   // debugging code 2, displays some information computed after tracing
                  {
                    float iterations_float = iteration_counter / 10000.0;
                    float skips_float = skip_counter / 4.0;
                    
                    // uncomennt one of the following
                    
                    // --- iteartions ---
                    fragment_color = vec4(iterations_float,iterations_float,iterations_float,0);
                    
                    // --- skips ---
                    //fragment_color = vec4(skips_float,skips_float,skips_float,0);
                    
                    // --- skips and iterations ---
                    //fragment_color = vec4(iterations_float,skips_float,0,0);
                    
                    // --- assert ---
                    //if (assertion) fragment_color = vec4(1,0,0,0); else fragment_color = vec4(0,1,0,0);

                    // ---  encoded debug vector ---
                    //fragment_color = vec4(map_minus_n_n_0_1(debug_vector,1000.0),0);
                  }
              }
            else
              fragment_color = texture(texture_color, uv_coords);
          
            break;  
        }
  }