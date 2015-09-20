#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 texture_coords;
layout (location = 2) in vec3 normal;

out vec4 transformed_position;
out vec2 uv_coords;

void main()
{
  transformed_position = vec4(position,1.0);
  gl_Position = transformed_position;
  uv_coords = texture_coords.xy;
}