#include "gl_wrapper.h"

Geometry3D *g2, *g3;
TransformationTRS t;
GLint color_location;
GLint view_matrix_location;
GLint model_matrix_location;
GLint projection_matrix_location;

void render()
  {
    glClear(GL_COLOR_BUFFER_BIT);
    glUniformMatrix4fv(model_matrix_location,1,GL_TRUE, glm::value_ptr(t.get_matrix()));
    g2->draw_as_triangles();
    
    g3->draw_as_triangles();
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
      
    t.print();
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
      
    t.print();
  }
  
int main(int argc, char** argv)
  {
    GLSession s;
    s.special_callback = special_callback;
    s.keyboard_callback = key_callback;
    s.init(render);
    
    Geometry3D g,gg;
    
    Shader shader(Shader::file_text("shader.vs"),Shader::file_text("shader.fs"));
 
    color_location = shader.get_uniform_location("color");
    model_matrix_location = shader.get_uniform_location("model_matrix");
    view_matrix_location = shader.get_uniform_location("view_matrix");
    projection_matrix_location = shader.get_uniform_location("projection_matrix");
    
    glm::mat4 projection_matrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.01f, 100.0f);
    glm::mat4 view_matrix = glm::mat4(1.0f);
    
    shader.use();
    
    glUniformMatrix4fv(projection_matrix_location,1,GL_TRUE, glm::value_ptr(projection_matrix));
    glUniformMatrix4fv(view_matrix_location,1,GL_TRUE, glm::value_ptr(view_matrix));
    glUniform3f(color_location,0.0,1.0,1.0);
    
    t.add_translation(glm::vec3(0.0,0.0,-10.0));
    
    g2 = &g;
    g.add_vertex(-0.2,-0.2,0.0);
    g.add_vertex(0.2,-0.2,0.0);
    g.add_vertex(-0.2,0.2,0.0);
    g.add_vertex(0.2,0.2,0.0);
    g.add_triangle(0,1,2);
    g.add_triangle(1,2,3);
    g.update_gpu();
    
    g3 = &gg;
    
    gg.add_vertex(-0.7,-0.2,-5);
    gg.add_vertex(-0.3,-0.2,-5);
    gg.add_vertex(-0.7,0.2,-5);
    gg.add_vertex(-0.3,0.2,-5);
    gg.add_triangle(0,1,2);
    gg.add_triangle(1,2,3);
    gg.update_gpu();
    
    g.print();
    s.start();
  
    return 0;
  }