#version 330

in vec3 transformed_normal;
in vec4 transformed_position;
in vec2 uv_coords;

uniform sampler2D tex;

out vec4 FragColor;

void main()
{
  FragColor = texture(tex, uv_coords);
}
