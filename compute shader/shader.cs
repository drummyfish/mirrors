#version 430

layout (local_size_x = 8, local_size_y = 1) in;

layout (std430, binding=1) buffer output_buffer_data
  {
    uint data[];
  } output_buffer;

bool is_prime(uint number)
  {
    uint i;
    uint up_to = uint(ceil(sqrt(number)));
  
    for (i = 2; i <= up_to; i++)
      if (number % i == 0)
        return false;
  
    return true;
  }
  
void main()
  {
    output_buffer.data[gl_GlobalInvocationID[0]] = is_prime(gl_GlobalInvocationID[0]) ? gl_GlobalInvocationID[0] : 0;
  }