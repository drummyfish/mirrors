#include "gl_wrapper.h"

Geometry3D *g2;
TransformationTRS t;
GLint color_location;
GLint light_direction_location;
GLint view_matrix_location;
GLint model_matrix_location;
GLint projection_matrix_location;

void render()
  {
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUniformMatrix4fv(model_matrix_location,1,GL_TRUE, glm::value_ptr(t.get_matrix()));
    g2->draw_as_triangles();
    glutSwapBuffers();
  }
  
void special_callback(int key, int x, int y)
  {
    switch (key)
      {
        case GLUT_KEY_UP:
          t.add_translation(glm::vec3(0,0.1,0));
          break;
        
        case GLUT_KEY_DOWN:
          t.add_translation(glm::vec3(0,-0.1,0));
          break;
        
        case GLUT_KEY_RIGHT:
          t.add_translation(glm::vec3(0.1,0,0));
          break;
        
        case GLUT_KEY_LEFT:
          t.add_translation(glm::vec3(-0.1,0,0));
          break;
          
        default:
          break;
      }
  }
  
void key_callback(unsigned char key, int x, int y)
  {
    switch (key)
      {
        case 'd':
          t.add_rotation(glm::vec3(0,0,0.1));
          break;
          
        case 'a':
          t.add_rotation(glm::vec3(0,0,-0.1));
          break;
        
        case 'w':
          t.add_rotation(glm::vec3(0.1,0,0));
          break;
          
        case 's':
          t.add_rotation(glm::vec3(-0.1,0,0));
          break;
        
        case 'q':
          t.add_rotation(glm::vec3(0,0.1,0));
          break;
          
        case 'e':
          t.add_rotation(glm::vec3(0,-0.1,0));
          break;
          
        case 'r':
          t.add_translation(glm::vec3(0.0,0.0,0.1));
          break;
          
        case 'f':
          t.add_translation(glm::vec3(0.0,0.0,-0.1));
          break;
          
        default:
          break;
      }
  }
  
int main(int argc, char** argv)
  {
    GLSession *s;
    s = GLSession::get_instance();
    s->special_callback = special_callback;
    s->keyboard_callback = key_callback;
    s->init(render);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    Geometry3D g;
    
    cout << "ver: '" << glGetString(GL_VERSION) << "'" << endl;
    
    Shader shader(file_text("shader.vs"),file_text("shader.fs"));
 
    light_direction_location = shader.get_uniform_location("light_direction");
    model_matrix_location = shader.get_uniform_location("model_matrix");
    //view_matrix_location = shader.get_uniform_location("view_matrix");
    
    //view_matrix_location = shader.get_uniform_location("view_matrix");
    projection_matrix_location = shader.get_uniform_location("projection_matrix");
    
    glm::mat4 projection_matrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.01f, 100.0f);
    glm::mat4 view_matrix = glm::mat4(1.0f);
    
    shader.use();
    
    glUniformMatrix4fv(projection_matrix_location,1,GL_TRUE, glm::value_ptr(projection_matrix));
    glUniformMatrix4fv(view_matrix_location,1,GL_TRUE, glm::value_ptr(view_matrix));
    glUniform3f(light_direction_location,0.0,0.0,-1.0);
    
    t.add_translation(glm::vec3(0.0,0.0,-10.0));
    
    g = make_quad(0.8,0.7);
    g.update_gpu();
    
    g2 = &g;
    s->start();
  
    cout << "ending" << endl;
    
    GLSession::clear();
    
    return 0;
  }