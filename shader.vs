#version 330

uniform mat4 model_matrix;
//uniform mat4 view_matrix;
uniform mat4 projection_matrix;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 texture_coords;
layout (location = 2) in vec3 normal;

out vec3 transformed_normal;

void main()
{
  gl_Position = vec4(position,1.0) * (model_matrix * projection_matrix);
  transformed_normal = normalize((model_matrix * vec4(normal,0.0)).xyz);
}