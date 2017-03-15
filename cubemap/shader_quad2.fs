#version 430

uniform sampler2D texture_color;
out vec4 fragment_color;
in vec2 uv_coords;

void main()
  {
    fragment_color = texture(texture_color,uv_coords);
  }