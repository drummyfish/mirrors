#version 430

layout (local_size_x = 16, local_size_y = 16) in;

layout (std430, binding=1) buffer output_buffer_data
  {
    uint data[];
  } output_buffer;

void main()
  {
    output_buffer.data[0] = 22;
  }