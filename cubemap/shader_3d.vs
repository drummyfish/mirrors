#version 330

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 projection_matrix;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 texture_coords;
layout (location = 2) in vec3 normal;

out vec3 transformed_normal;    // normal in world space
out vec4 world_position;        // position in world space
out vec4 transformed_position;  // position in view space
out vec2 uv_coords;

void main()
{
  world_position = vec4(position,1.0) * model_matrix;
  transformed_position = world_position * view_matrix;
  gl_Position = transformed_position * projection_matrix;
  transformed_normal = normalize(normal);
  uv_coords = texture_coords.xy;
}