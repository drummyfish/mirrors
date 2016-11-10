#version 330

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 projection_matrix;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 texture_coords;
layout (location = 2) in vec3 normal;

out vec3 transformed_normal;
out vec4 transformed_position;
out vec2 uv_coords;

void main()
{
  transformed_position = vec4(position,1.0) * model_matrix * view_matrix;
  gl_Position = transformed_position * projection_matrix;
  transformed_normal = normalize((vec4(normal,0.0) * model_matrix).xyz);
  uv_coords = texture_coords.xy;
}