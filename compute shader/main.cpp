/**
 * Example of compute shaders.
 * 
 * Computes prime numbers up to 23.
 */

#include "../gl_wrapper.h"
#include <string>
#include <iostream>

using namespace std;

Shader *draw_shader;
Shader *compute_shader;
StorageBuffer *buffer;

void render()
  {
  }

int main(int argc, char** argv)
  {
    GLSession *session;
    session = GLSession::get_instance();
    session->init(render);
    
    compute_shader = new Shader("","",file_text("shader.cs",true));
    buffer = new StorageBuffer(24,1);      // 3 * 8 workgroups
    compute_shader->use();
    compute_shader->run_compute(3,1,1);    // launch 3 workgroups
    buffer->load_from_gpu();
    buffer->print();

    GLSession::clear();
    
    delete buffer;
    delete compute_shader;
    return 0;
  }
