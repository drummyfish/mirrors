#version 330

in vec3 transformed_normal;
in vec4 transformed_position;
in vec2 uv_coords;
uniform vec3 light_direction;
uniform vec3 camera_position;
uniform sampler2D tex;
uniform samplerCube tex_cube;
uniform bool mirror;

layout(location = 0) out vec4 FragColor;
float diffuse_intensity;
float lighting_intensity;
vec3 cube_coordinates;

void main()
{
  diffuse_intensity = clamp(dot(normalize(transformed_normal),-1 * light_direction),0.0,1.0);
  lighting_intensity = clamp(0.4 + diffuse_intensity,0.0,1.0);
  
  if (mirror)
    {
      cube_coordinates = reflect(vec3(transformed_position) - camera_position,normalize(transformed_normal));
      FragColor = texture(tex_cube,cube_coordinates);
    }
  else
    {
      FragColor = 0.3 * vec4(lighting_intensity, lighting_intensity, lighting_intensity, 1.0);
      FragColor += texture(tex, uv_coords);
    }
}
