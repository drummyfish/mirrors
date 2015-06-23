#version 330

in vec3 transformed_normal;
uniform vec3 light_direction;

out vec4 FragColor;
float diffuse_intensity;
float lighting_intensity;

void main()
{
  diffuse_intensity = clamp(dot(normalize(transformed_normal),-1 * light_direction),0.0,1.0);
  lighting_intensity = clamp(0.4 + diffuse_intensity,0.0,1.0);
  FragColor = vec4(lighting_intensity, lighting_intensity, lighting_intensity, 1.0);
}
