/**
 * Example of compute shaders.
 * 
 * WORK IN PROGRESS
 */

#include "../gl_wrapper.h"
#include <string>
#include <iostream>

using namespace std;

void render()
  {
    glClearColor(1,1,1,1);
    glutSwapBuffers();
  }
  
int main(int argc, char** argv)
  {
    GLSession *session;
    session = GLSession::get_instance();
    session->window_size[0] = 320;
    session->window_size[1] = 240;
    session->init(render);
    
    Shader *compute_shader = new Shader("","",file_text("shader.cs"));
    
    if (compute_shader->loaded_succesfully())
      cout << "Compute shader loaded." << endl;
    else
      cout << "Error: Compute shader not loaded." << endl;
    
    compute_shader->use();
    compute_shader->run_compute(1,1,1);
    
    //session->start();
    GLSession::clear();
    delete compute_shader;
    return 0;
  }
