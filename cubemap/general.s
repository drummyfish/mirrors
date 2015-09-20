/*
 * General stuff to be included in shaders.
 */

float position_scale = 16;

vec3 map_0_1_minus_n_n(vec3 input_vec, float n)
  {
    return (input_vec - vec3(0.5,0.5,0.5)) * 2 * n;
  }

vec3 map_minus_n_n_0_1(vec3 input_vec, float n)   // for mapping <-n,n> to <0,1> due to OpenGL clamping texture values
  {
    return (input_vec / n + vec3(1.0,1.0,1.0)) / 2.0;
  }