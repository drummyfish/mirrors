#ifndef GL_WRAPPER_H
#define GL_WRAPPER_H

#define GLM_FORCE_RADIANS

#include <stdio.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <string>
#include <string.h>
#include <string>
#include <fstream>
#include <streambuf>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

//--------------------------------------------------------------

class GLSession
  {
    public:
      static bool initialised;
      
      int argc;
      char **argv;
      int display_mode;
      unsigned int window_size[2];
      unsigned int window_position[2];
      string window_title;
      
      void (*render_callback)(void);
      void (*keyboard_callback)(unsigned char key, int x, int y);
      void (*keyboard_up_callback)(unsigned char key, int x, int y);
      void (*special_callback)(int key, int x, int y);
      void (*special_up_callback)(int key, int x, int y);
      void (*mouse_callback)(int button, int state, int x, int y);
      void (*reshape_callback)(int width, int height);
      void (*mouse_pressed_motion_callback)(int x, int y);
      void (*mouse_not_pressed_motion_callback)(int x, int y);
      
      GLSession()
        {
          this->argc = 0;
          this->argv = 0;
          this->display_mode = GLUT_DOUBLE | GLUT_RGBA;
          this->window_size[0] = 800;
          this->window_size[1] = 600;
          this->window_position[0] = 50;
          this->window_position[1] = 50;
          this->window_title = "window";
          
          this->keyboard_callback = 0;
          this->keyboard_up_callback = 0;
          this->special_callback = 0;
          this->special_up_callback = 0;
          this->mouse_callback = 0;
          this->reshape_callback = 0;
          this->mouse_pressed_motion_callback = 0;
          this->mouse_not_pressed_motion_callback = 0;
        };
        
      void init(void (*render_callback)(void))
        {
          this->render_callback = render_callback;
          glutInit(&this->argc, this->argv);
          glutInitDisplayMode(this->display_mode);
          glutInitWindowSize(this->window_size[0],this->window_size[1]);
          glutInitWindowPosition(this->window_position[0],this->window_position[1]);
          glutCreateWindow(this->window_title.c_str());
          glutDisplayFunc(this->render_callback); 
          glutIdleFunc(this->render_callback);
          
          if (this->keyboard_callback != 0)
            glutKeyboardFunc(this->keyboard_callback);

          if (this->keyboard_up_callback != 0)
            glutKeyboardUpFunc(this->keyboard_callback);
          
          if (this->special_callback != 0)
            glutSpecialFunc(this->special_callback);
         
          if (this->special_up_callback != 0)
            glutSpecialUpFunc(this->special_up_callback);
          
          glewInit();
          GLSession::initialised = true;
        }
        
      void start()
        {
          glutMainLoop();
        };
        
      void end()
        {
        };
  };
  
bool GLSession::initialised = false;
  
//--------------------------------------------------------------

class Transformation
  {
    protected:
      glm::mat4 final_matrix;
    public:
      glm::mat4 get_matrix()
        {
          return this->final_matrix;
        }
  };
  
//--------------------------------------------------------------
  
/**
 * Transformation consisting of translation, rotation and scale.
 */
  
class TransformationTRS: public Transformation
  {
    protected:
      glm::mat4 translation_matrix;
      glm::mat4 rotation_matrix;
      glm::mat4 scale_matrix;
      
      glm::vec3 translation;    ///< translation as dx, dy, dz
      glm::vec3 rotation;       ///< rotation as around x, around y, around z
      glm::vec3 scale;          ///< scale as scale x, scale y, scale z
    
      void recompute_final_matrix()
        {
          this->final_matrix = this->translation_matrix * this->rotation_matrix * this->scale_matrix;
        };
    
      void recompute_translation_matrix()
        {
          this->translation_matrix = glm::translate(glm::mat4(1.0f),this->translation);
          this->recompute_final_matrix();
        };
    
      void recompute_rotation_matrix()
        {
          this->rotation_matrix = glm::mat4(1.0f);
          this->rotation_matrix = glm::rotate(this->rotation_matrix,this->rotation.y,glm::vec3(0,1,0));  // around y
          this->rotation_matrix = glm::rotate(this->rotation_matrix,this->rotation.x,glm::vec3(1,0,0));  // around x
          this->rotation_matrix = glm::rotate(this->rotation_matrix,this->rotation.z,glm::vec3(0,0,1));  // around z
          this->recompute_final_matrix();
        };
        
      void recompute_scale_matrix()
        {
          this->scale_matrix = glm::scale(glm::mat4(1.0f),this->scale);
          this->recompute_final_matrix();
        };
        
    public:
      static void print_mat4(glm::mat4 matrix)
        {
          unsigned int i, j;
          
          for (j = 0; j < 4; j++)
            {
              for (i = 0; i < 4; i++)
                cout << matrix[i][j] << " ";
                
              cout << endl;
            }
        }
        
      static void print_vec3(glm::vec3 vector)
        {
          cout << vector.x << " " << vector.y << " " << vector.z << endl;
        }
      
      TransformationTRS()
        {
          this->set_translation(glm::vec3(0,0,0));
          this->set_rotation(glm::vec3(0,0,0));
          this->set_scale(glm::vec3(1,1,1));
        };
      
      void set_translation(glm::vec3 new_translation)
        {
          this->translation = new_translation;
          this->recompute_translation_matrix();
        };
        
      void add_translation(glm::vec3 translation)
        {
          this->translation = this->translation + translation;
          this->recompute_translation_matrix();
        };
        
      void add_rotation(glm::vec3 rotation)
        {
          this->rotation = this->rotation + rotation;
          this->recompute_rotation_matrix();
        }
        
      void set_rotation(glm::vec3 new_rotation)
        {
          this->rotation = new_rotation;
          this->recompute_rotation_matrix();
        };
        
      void set_scale(glm::vec3 new_scale)
        {
          this->scale = new_scale;
          this->recompute_scale_matrix();
        };
        
      void print()
        { 
          cout << "translation: ";
          TransformationTRS::print_vec3(this->translation);
          cout << "rotation: ";
          TransformationTRS::print_vec3(this->rotation);
          cout << "scale: ";
          TransformationTRS::print_vec3(this->scale);
          cout << "final matrix:" << endl;
          TransformationTRS::print_mat4(this->final_matrix);
        };
  };

//--------------------------------------------------------------
  
class Shader
  {
    protected:
      GLuint shader_program;
      
      bool add_shader(const char* shader_text, GLenum shader_type)
        {
          GLuint shader_object = glCreateShader(shader_type);

          const GLchar* p[1];
          p[0] = shader_text;

          GLint lengths[1];
          lengths[0]= strlen(shader_text);

          glShaderSource(shader_object,1,p,lengths);
          glCompileShader(shader_object);

          GLint success;

          glGetShaderiv(shader_object,GL_COMPILE_STATUS,&success);

          if (!success)
            {
              GLchar log[1024];
              glGetShaderInfoLog(shader_object,sizeof(log),NULL,log);
              cerr << "Shader compile log: " << log << endl;
              return false;
            }

          glAttachShader(this->shader_program,shader_object);

          return true;
        }
      
    public:
      static string file_text(string filename)
        {
          std::ifstream t(filename);
          
          if (!t.is_open())
            {
              cerr << "ERROR: could not open " << filename << "." << endl;
            }
          
          std::string result((std::istreambuf_iterator<char>(t)),std::istreambuf_iterator<char>());
          return result;
        }
      
      void use()
        {
          glUseProgram(this->shader_program);
        }
        
      GLint get_uniform_location(string name)
        {
          GLint result;
          
          result = glGetUniformLocation(this->shader_program,name.c_str());
          
          if (result < 0)
            cerr << "ERROR: could not get uniform location of " << name << "." << endl;
            
          return result;
        } 
      
      Shader(string vertex_shader_text, string fragment_shader_text)
        {
          char log[256];
          
          this->shader_program = glCreateProgram();
          
          if (this->shader_program == 0)
            {
              cerr << "ERROR: could not create a shader program." << endl;
            }
         
          if (vertex_shader_text.length() != 0)
            if (!this->add_shader(vertex_shader_text.c_str(),GL_VERTEX_SHADER))
              {
                cerr << "ERROR: could not add a vertex shader program." << endl;
              }
              
          if (fragment_shader_text.length() != 0)
            if (!this->add_shader(fragment_shader_text.c_str(),GL_FRAGMENT_SHADER))
              {
                cerr << "ERROR: could not add a fragment shader program." << endl;
              }
            
          GLint success = 0;
          glLinkProgram(this->shader_program);
          glGetProgramiv(this->shader_program,GL_LINK_STATUS,&success);
        
          if (success == 0)
            {
              glGetProgramInfoLog(this->shader_program,sizeof(log),NULL,log);
              cerr << log << endl;
              cerr << "ERROR: could not link the shader program." << endl;
            }
            
          glValidateProgram(this->shader_program);
          
          glGetProgramiv(shader_program,GL_VALIDATE_STATUS,&success);
          
          if (!success)
            {
              cerr << "ERROR: the shader program is invalid." << endl;
            }
        }
  };

//--------------------------------------------------------------
  
class Geometry3D
  {
    protected:
      vector<glm::vec3> vertices;
      vector<unsigned int> triangles;
      
      GLuint vbo;
      GLuint vao;
      GLuint ibo;
       
    public:
      Geometry3D()
        {
          if (!GLSession::initialised)
            cerr << "ERROR: Geometry3D object created before GLSession was initialised." << endl;

          glGenVertexArrays(1,&(this->vao));
          glBindVertexArray(this->vao);
          glGenBuffers(1,&(this->vbo));
          glBindBuffer(GL_ARRAY_BUFFER,this->vbo);
          glGenBuffers(1,&(this->ibo));
          glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,this->ibo);
          glBindBuffer(GL_ARRAY_BUFFER,this->vbo);
          glEnableVertexAttribArray(0);
          glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
          glBindVertexArray(0);
        };
      
      void draw_as_triangles()
        {
          glBindVertexArray(this->vao);
          glDrawElements(GL_TRIANGLES,this->triangles.size() * 3,GL_UNSIGNED_INT,0);
          glBindVertexArray(0);
        };
        
      void update_gpu()
        {
          glBindVertexArray(this->vao);
          glBindBuffer(GL_ARRAY_BUFFER,this->vbo);
          glBufferData(GL_ARRAY_BUFFER,this->vertices.size() * sizeof(glm::vec3),&(this->vertices[0]),GL_STATIC_DRAW);
          glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,this->ibo);
          glBufferData(GL_ELEMENT_ARRAY_BUFFER,this->triangles.size() * sizeof(unsigned int),&this->triangles[0],GL_STATIC_DRAW);
          glBindVertexArray(0);
        };
        
      vector<glm::vec3> *get_vertices()
        {
          return &(this->vertices);
        }
        
      vector<unsigned int> *get_triengles()
        {
          return &(this->triangles);
        }
        
      void add_vertex(double x, double y, double z)
        {
          this->vertices.push_back(glm::vec3(x,y,z));
        }
        
      void add_triangle(unsigned int i1, unsigned int i2, unsigned int i3)
        {
          this->triangles.push_back(i1);
          this->triangles.push_back(i2);
          this->triangles.push_back(i3);
        }
        
      void print()
        {
          int i;
          
          cout << "vertices:" << endl;
          
          for (i = 0; i < (int) this->vertices.size(); i++)
            {
              cout << "  " << i << ": " << vertices[i][0] << " " << vertices[i][1] << " " << vertices[i][2] << endl;
            }
            
          cout << "triangles:" << endl;
          
          for (i = 0; i < (int) this->triangles.size(); i++)
            {
              if (i % 3 == 0)
                cout << "  ";
              
              cout << triangles[i] << " ";
              
              if (i % 3 == 2)
                cout << endl;
            }
        }
  };

//--------------------------------------------------------------
  
#endif