#version 330

in vec3 transformed_normal;         // normal in world space
in vec4 transformed_position;       // position in world space
in vec4 world_position;
in vec2 uv_coords;
uniform vec3 light_direction;
uniform sampler2D texture_2d;
uniform bool mirror;                // whether mirror is being rendered
uniform bool sky;                   // whether sky is being rendered
uniform bool marker;                // whether marking geometry is being rendered
uniform bool rendering_cubemap;     // whether cubemap is being rendered
uniform vec3 cubemap_position;      // if rendering_cubemap = true, contains cubemap world position

layout(location = 0) out vec4 fragment_color;
layout(location = 1) out vec3 output_position_distance;   /* x y z position in space if rendering_cubemap = true,
                                                             otherwise distance to cubemap in red channel */
layout(location = 2) out vec3 output_normal;              // x y z normal
layout(location = 3) out vec3 output_stencil;             // (1,1,1) or (0,0,0) masking the mirror out

float diffuse_intensity;
float lighting_intensity;
vec3 cube_coordinates;
float distance_from_cubemap;
  
void main()
  {
    diffuse_intensity = clamp(dot(normalize(transformed_normal),-1 * light_direction),0.0,1.0);
    lighting_intensity = clamp(0.4 + diffuse_intensity,0.0,1.0);
  
    if (sky)
      {
        fragment_color = texture(texture_2d,uv_coords);
      }
    else if (marker)
      {
        fragment_color = vec4(1,0,0,1);
      }
    else if (!mirror)
      {
        fragment_color = 0.8 + 0.2 * vec4(lighting_intensity, lighting_intensity, lighting_intensity, 1.0);
        fragment_color *= texture(texture_2d,uv_coords);
      }

    if (rendering_cubemap)
      {
        distance_from_cubemap = distance(world_position.xyz,cubemap_position);
        output_position_distance = vec3(distance_from_cubemap,distance_from_cubemap,0.0);
      }
    else
      {
        output_position_distance = world_position.xyz;  
      }
  
    output_normal = transformed_normal.xyz;
    output_stencil = mirror ? vec3(1.0,1.0,1.0) : vec3(0.0,0.0,0.0);
  }