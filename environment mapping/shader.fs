#version 330

in vec3 transformed_normal;
in vec4 transformed_position;
in vec2 uv_coords;
uniform vec3 camera_position;
uniform bool mirror;
uniform bool sky;

uniform sampler2D tex;
uniform samplerCube cubemap_texture;

out vec4 FragColor;
float diffuse_intensity;
float lighting_intensity;

void main()
{
  if (sky)
    {
      FragColor = vec4(0,0,0,0);   // no shading for sky
    }
  else
    {
      // compute shading:
      
      diffuse_intensity = clamp(dot(normalize(transformed_normal),-1 * normalize(vec3(1,1,1))),0.0,1.0);
      lighting_intensity = clamp(0.4 + diffuse_intensity,0.0,1.0);
      FragColor = 0.3 * vec4(lighting_intensity, lighting_intensity, lighting_intensity, 1.0);
    }
  
  if (mirror)
    {
      vec3 coordinates = reflect(normalize(transformed_position.xyz - camera_position),normalize(transformed_normal));
      FragColor = FragColor * 0.3 + textureLod(cubemap_texture,coordinates,0);
    }
  else
    {
      FragColor += texture(tex,uv_coords);
    }
}
