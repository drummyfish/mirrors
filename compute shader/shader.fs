#version 430

out vec4 FragColor;

layout (std430, binding=1) buffer output_buffer_data
  {
    uint data[];
  } output_buffer;
  
void main()
  {
    output_buffer.data[0] = 100;
    FragColor = vec4(1,1,0,1);
  }
