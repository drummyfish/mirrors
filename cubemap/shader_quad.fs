#version 330

in vec3 transformed_normal;
in vec4 transformed_position;
in vec2 uv_coords;

uniform int texture_to_display;      // which texture to display (1 = color, 2 = normal etc.)
uniform sampler2D texture_color;
uniform sampler2D texture_normal;
uniform sampler2D texture_position;
uniform sampler2D texture_stencil;

out vec4 FragColor;

void main()
{
  if (texture(texture_stencil, uv_coords) == vec4(1,1,1,0))
    FragColor = texture(texture_normal, uv_coords);
  else
    switch (texture_to_display)
      {
        case 1:
          FragColor = texture(texture_color, uv_coords);
          break;  
        
        case 2:
          FragColor = texture(texture_normal, uv_coords);
          break;
          
        case 3:
          FragColor = texture(texture_position, uv_coords);
          break;
          
        case 4:
          FragColor = texture(texture_stencil, uv_coords);
          break;
        
        default:
          FragColor = texture(texture_color, uv_coords);
          break;
      }
}
