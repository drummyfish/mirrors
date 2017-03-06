/**
 * Example of compute shaders.
 * 
 * WORK IN PROGRESS
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
    glClearColor(1,1,1,1);

    buffer->bind();

    draw_shader->use();
    draw_fullscreen_quad();

    buffer->load_from_gpu();
    buffer->print();
    
    compute_shader->use();
    compute_shader->run_compute(1,1,1);
    
    buffer->load_from_gpu();
    buffer->print();
    
    cout << "-----" << endl;
    
    glutSwapBuffers();
  }
  
int main(int argc, char** argv)
  {
    GLSession *session;
    session = GLSession::get_instance();
    session->window_size[0] = 320;
    session->window_size[1] = 240;
    session->init(render);
    
    draw_shader = new Shader(VERTEX_SHADER_QUAD_TEXT,file_text("shader.fs",true),"");
    compute_shader = new Shader("","",file_text("shader.cs",true));
 
    buffer = new StorageBuffer(16,1);
    
    session->start();

    GLSession::clear();
    
    delete buffer;
    delete compute_shader;
    return 0;
  }
