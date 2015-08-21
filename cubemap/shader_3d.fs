#version 330

in vec3 transformed_normal;
in vec4 transformed_position;
in vec2 uv_coords;
uniform vec3 light_direction;
uniform sampler2D texture_2d;
uniform bool mirror;                   // whether mirror is being rendered

layout(location = 0) out vec4 fragment_color;
layout(location = 1) out vec3 output_position;   // x y z position in space (divided by 64)
layout(location = 2) out vec3 output_normal;     // x y z normal
layout(location = 3) out vec3 output_stencil;    // (1,1,1) or (0,0,0) masking the mirror out
float diffuse_intensity;
float lighting_intensity;
vec3 cube_coordinates;

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
  diffuse_intensity = clamp(dot(normalize(transformed_normal),-1 * light_direction),0.0,1.0);
  lighting_intensity = clamp(0.4 + diffuse_intensity,0.0,1.0);
  
  if (!mirror)
    {
      fragment_color = 0.3 * vec4(lighting_intensity, lighting_intensity, lighting_intensity, 1.0);
      fragment_color += texture(texture_2d,uv_coords);
    }
    
  output_position = map_minus_n_n_0_1(transformed_position.xyz,16);  
  output_normal = map_minus_n_n_0_1(transformed_normal,1.0);
  output_stencil = mirror ? vec3(1.0,1.0,1.0) : vec3(0.0,0.0,0.0);
}
