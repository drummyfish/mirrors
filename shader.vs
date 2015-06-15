#version 330

layout (location = 0) in vec3 Position;

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 projection_matrix;

void main()
{
  gl_Position = vec4(Position,1.0) * (model_matrix * projection_matrix);
}