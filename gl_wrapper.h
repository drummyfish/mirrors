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

/**
 * Serves for error output.
 */

class ErrorWriter
  {
    public:
      static bool enabled;
      
      static void write_error(string message)
        {
          if (ErrorWriter::enabled)
            cerr << "Error: " << message << endl;
        }
      
      static void setErrorOutput(bool enabled)
        {
          ErrorWriter::enabled = enabled;
        }
  };

bool ErrorWriter::enabled = true;
  
void print_mat4(glm::mat4 matrix)
  {
    unsigned int i, j;
          
    for (j = 0; j < 4; j++)
      {
        for (i = 0; i < 4; i++)
          cout << matrix[i][j] << " ";
                
          cout << endl;
      }
  }
        
void print_vec3(glm::vec3 vector)
  {
    cout << vector.x << " " << vector.y << " " << vector.z << endl;
  }

/**
  * Returns a text in given file.
  */
      
static string file_text(string filename)
  {
    std::ifstream t(filename);
          
    if (!t.is_open())
      {
        ErrorWriter::write_error("Could not open file '" + filename + "'.");
      }
          
      std::string result((std::istreambuf_iterator<char>(t)),std::istreambuf_iterator<char>());
      return result;
  }
  
/**
 * A singleton class representing an OpenGL session. 
 */

class GLSession
  {
    protected:
      static GLSession *instance;           ///< singleton instance
      static bool initialised;
      
      /**
       * Initialises the GLSession instance with default values. Usage:
       * call GLSession::get_instance() to get the object, optionally
       * modify some of its attributes, then call init() on the object and
       * start() to start the rendering loop. At the end of the program
       * call GLSession::clear();
       */
      
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
          this->loop_exit_behavior = GLUT_ACTION_CONTINUE_EXECUTION;
          
          this->keyboard_callback = 0;
          this->keyboard_up_callback = 0;
          this->special_callback = 0;
          this->special_up_callback = 0;
          this->mouse_callback = 0;
          this->reshape_callback = 0;
          this->mouse_pressed_motion_callback = 0;
          this->mouse_not_pressed_motion_callback = 0;
        };
    
    public:
      int argc;
      char **argv;
      int display_mode;                      ///< GLUT display mode flags
      unsigned int window_size[2];           ///< [width, height] in pixels
      unsigned int window_position[2];       ///< [x, y] in pixels
      string window_title;                   ///< window title
      int loop_exit_behavior;                ///< GLUT enum that says what should happen when the main loop is left
      
      void (*render_callback)(void);
      void (*keyboard_callback)(unsigned char key, int x, int y);
      void (*keyboard_up_callback)(unsigned char key, int x, int y);
      void (*special_callback)(int key, int x, int y);
      void (*special_up_callback)(int key, int x, int y);
      void (*mouse_callback)(int button, int state, int x, int y);
      void (*reshape_callback)(int width, int height);
      void (*mouse_pressed_motion_callback)(int x, int y);
      void (*mouse_not_pressed_motion_callback)(int x, int y);
        
      /**
       * This should be called at the end of the program.
       */
      
      static void clear()
        {
          delete GLSession::instance;
          GLSession::instance = 0;
        }
      
      /**
       * Gets the GLSession singleton instance (on first call a new one
       * is created, later that one is being returned).
       */
      
      static GLSession *get_instance()
        {
          if (!GLSession::is_created())
            GLSession::instance = new GLSession();
            
          return GLSession::instance;
        }
      
      /**
       * Checks if the singleton instance of GLSession has been created.
       */
      
      static bool is_created()
        {
          return GLSession::instance != 0;
        }
     
      /**
       * Checks if the singleton instance of GLSession has been initialised
       * with init().
       */
     
      static bool is_initialised()
        {
          return GLSession::initialised;
        }
      
      /**
       * Initialises the session, i.e. calls the GLUT and GLEW init functions.
       */
      
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
          
          glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,this->loop_exit_behavior);
          
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
        
      /**
       * Starts the rendering loop.
       */
        
      void start()
        {
          glutMainLoop();
        };
        
      /**
       * Ends the rendering loop;
       */
        
      void end()
        {
          glutLeaveMainLoop();
        };
  };
  
GLSession *GLSession::instance;
bool GLSession::initialised;

/**
 * Represents a vertex matrix transformation.
 */

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
  
      /**
       * Recomputes the final matrix, i.e. the composition of translation,
       * rotation and scale.
       */
  
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
      /**
       * Initialises a new instance.
       */
      
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
          print_vec3(this->translation);
          cout << "rotation: ";
          print_vec3(this->rotation);
          cout << "scale: ";
          print_vec3(this->scale);
          cout << "final matrix:" << endl;
          print_mat4(this->final_matrix);
        };
  };

/**
 * Represents an OpenGL shader.
 */
  
class Shader
  {
    protected:
      GLuint shader_program;
      
      /**
       * Helper function.
       */
      
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
      /**
       * Sets the shader for usage.
       */
      
      void use()
        {
          glUseProgram(this->shader_program);
        }
        
      /**
       * Gets the location of uniform variable by its name.
       */
        
      GLint get_uniform_location(string name)
        {
          GLint result;
          
          result = glGetUniformLocation(this->shader_program,name.c_str());
          
          if (result < 0)
            ErrorWriter::write_error("Could not get uniform location of '" + name + "'.");
            
          return result;
        }
      
      /**
       * Initialises a new instance.
       * 
       * @param vertex_shader_text vertex shader source code
       * @param fragment_shader_text fragment shader source code
       * @param transform_feedback_variables list of output vertex shader variable names
       *   to be recorded in transform feedback, leave empty if you don't want to use
       *   transform feedback 
       */
      
      Shader(string vertex_shader_text, string fragment_shader_text, vector<string> transform_feedback_variables)
        {
          char log[256];
          
          this->shader_program = glCreateProgram();
          
          if (this->shader_program == 0)
            {
              ErrorWriter::write_error("Could not create a shader program.");
            }
         
          if (vertex_shader_text.length() != 0)
            if (!this->add_shader(vertex_shader_text.c_str(),GL_VERTEX_SHADER))
              {
                ErrorWriter::write_error("Could not add a vertex shader program.");
              }
              
          if (fragment_shader_text.length() != 0)
            if (!this->add_shader(fragment_shader_text.c_str(),GL_FRAGMENT_SHADER))
              {
                ErrorWriter::write_error("Could not add a fragment shader program.");
              }
            
          GLint success = 0;
          
          // init transform feedback:
          
          if (transform_feedback_variables.size() > 0)
            {
              char *variable_names[512];
          
              unsigned int i;
              
              for (i = 0; i < transform_feedback_variables.size() && i < 512; i++)
                variable_names[i] = (char *) transform_feedback_variables[i].c_str();
          
              glTransformFeedbackVaryings(this->shader_program,transform_feedback_variables.size(),(const GLchar **) variable_names,GL_INTERLEAVED_ATTRIBS);
            }
          
          glLinkProgram(this->shader_program);
          glGetProgramiv(this->shader_program,GL_LINK_STATUS,&success);
        
          if (success == 0)
            {
              glGetProgramInfoLog(this->shader_program,sizeof(log),NULL,log);
              cerr << log << endl;
              ErrorWriter::write_error("Could not link the shader program.");
            }
            
          glValidateProgram(this->shader_program);
          
          glGetProgramiv(shader_program,GL_VALIDATE_STATUS,&success);
          
          if (!success)
            {
              ErrorWriter::write_error("The shader program is invalid.");
            }
        }
  };

class Vertex3D
  {
    public:
      glm::vec3 position;
      glm::vec3 texture_coord;
      glm::vec3 normal;
      
      Vertex3D()
        {
          this->position = glm::vec3(0.0f,0.0f,0.0f);
          this->texture_coord = glm::vec3(0.0f,0.0f,0.0f);
          this->normal = glm::vec3(0.0f,0.0f,0.0f);
        }
        
      void print()
        {
          print_vec3(this->position);
          cout << " (uvw: ";
          print_vec3(this->texture_coord);
          cout << ", normal: ";
          print_vec3(this->normal);
          cout << ")" << endl;
        }
  };

/**
 * Represents a transform feedback buffer to get values computed
 * in vertex shader.
 */
  
class TransformFeedbackBuffer
  {
    protected:
      GLuint vbo;
      unsigned int size;
      unsigned char *byte_array;        ///< to store the feedback data from GPU
  
    public:
      /**
       * Initialises a new object.
       * 
       * @param size size of the buffer in bytes
       */
      
      TransformFeedbackBuffer(unsigned int size)
        {
          if (!GLSession::is_initialised())
            ErrorWriter::write_error("TransformFeedbackBuffer object created before GLSession was initialised.");
        
          this->size = size;
          byte_array = (unsigned char *) malloc(this->size);
          glGenBuffers(1,&this->vbo);
          glBindBuffer(GL_ARRAY_BUFFER,this->vbo);
          glBufferData(GL_ARRAY_BUFFER,size,nullptr,GL_STATIC_READ);
        }
        
      ~TransformFeedbackBuffer()
        {
          delete this->byte_array;
        }
      
      void transform_feedback_begin(unsigned int primitive)
        {
          glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,this->vbo);
          glBeginTransformFeedback(primitive);
        }
      
      void print_byte()
        {
          unsigned int i;
          
          glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,this->vbo);
          glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER,0,this->size,this->byte_array);
          
          for (i = 0; i < this->size; i++)
            cout << ((unsigned int) this->byte_array[i]) << " ";
            
          cout << endl;
        }
      
      void print_float()
        {
          glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,this->vbo);
          glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER,0,this->size,this->byte_array);
          
          unsigned int float_count = this->size / sizeof(GLfloat);
          unsigned int i;
          
          for (i = 0; i < float_count; i++)
            cout << ((GLfloat *) this->byte_array)[i] << " ";
            
          cout << endl;
        }
        
      void transform_feedback_end()
        {
          glEndTransformFeedback();
        }
          
  };
  
class Texel
  {
    public:
      float red;
      float green;
      float blue;
      float alpha;
      
      Texel()
        {
          this->set_value(1.0,1.0,1.0,1.0);
        }
      
      void set_value(float r, float g, float b, float a)
        {
          this->red = r;
          this->green = g;
          this->blue = b;
          this->alpha = a;
        }
  };
  
/**
 * RGBA 2D image texture.
 */
  
class Texture2D
  {
    protected:
      GLuint to;             // texture object id
      
      unsigned int width;
      unsigned int height;
      vector<Texel> data;
      
      unsigned int coords_2d_to_1d(unsigned int x, unsigned int y)
        {
          if (x < 0)
            x = 0;
          else if (x >= this->width)
            x = this->width - 1;
          
          if (y < 0)
            y = 0;
          else if (y >= this->height)
            y = this->height - 1;
          
          return y * this->width + x;
        }
      
    public:
      Texture2D(unsigned int width, unsigned int height)
        {
          unsigned int i;
          
          this->width = width;
          this->height = height;
        
          glGenTextures(1,&(this->to));
          
          for (i = 0; i < width * height; i++)
            this->data.push_back(Texel());
        }
        
      void set_pixel(unsigned int x, unsigned int y, float r, float g, float b, float a)
        {
          Texel texel;
          texel.set_value(r,g,b,a);
          this->data[this->coords_2d_to_1d(x,y)] = texel;
        }
        
      void update_gpu()
        {
          glBindTexture(GL_TEXTURE_2D,this->to);
          glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,this->width,this->height,0,GL_RGBA,GL_FLOAT,&(this->data[0]));
          glGenerateMipmap(GL_TEXTURE_2D);
          glBindTexture(GL_TEXTURE_2D,0);
        }
        
      void set_parameter_int(unsigned int parameter, unsigned int value)
        {
          glBindTexture(GL_TEXTURE_2D,this->to);
          glTexParameteri(GL_TEXTURE_2D,parameter,value);
          glBindTexture(GL_TEXTURE_2D,0);
        }
        
      void print()
        {
          unsigned int x, y, index;
          
          for (y = 0; y < this->height; y++)
            for (x = 0; x < this->width; x++)
              {
                index = this->coords_2d_to_1d(x,y);
                
                cout << x << "; " << y << ": " <<
                  this->data[index].red << ", " <<
                  this->data[index].green << ", " <<
                  this->data[index].blue << ", " <<
                  this->data[index].alpha << endl;
              }     
        }
  };
  
/**
 * Represents a 3D geometry.
 */
  
class Geometry3D
  {
    protected:
      GLuint vbo;
      GLuint vao;
      GLuint ibo;
       
    public:
      vector<Vertex3D> vertices;
      vector<unsigned int> triangles;
      
      Geometry3D()
        {
          if (!GLSession::is_initialised())
            ErrorWriter::write_error("Geometry3D object created before GLSession was initialised.");

          glGenVertexArrays(1,&(this->vao));
          glBindVertexArray(this->vao);
          glGenBuffers(1,&(this->vbo));
          glBindBuffer(GL_ARRAY_BUFFER,this->vbo);
          glGenBuffers(1,&(this->ibo));
          glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,this->ibo);
          glBindBuffer(GL_ARRAY_BUFFER,this->vbo);
          
          glEnableVertexAttribArray(0);  // position
          glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex3D),0);
        
          glEnableVertexAttribArray(1);  // texture coord
          glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex3D),(const GLvoid*) sizeof(glm::vec3));
          
          glEnableVertexAttribArray(2);  // normal
          glVertexAttribPointer(2,3,GL_FLOAT,GL_TRUE,sizeof(Vertex3D),(const GLvoid*) (sizeof(glm::vec3) * 2));
          
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
          glBufferData(GL_ARRAY_BUFFER,this->vertices.size() * sizeof(Vertex3D),&(this->vertices[0]),GL_STATIC_DRAW);
          glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,this->ibo);
          glBufferData(GL_ELEMENT_ARRAY_BUFFER,this->triangles.size() * sizeof(unsigned int),&this->triangles[0],GL_STATIC_DRAW);
          glBindVertexArray(0);
        };
        
      vector<Vertex3D> *get_vertices()
        {
          return &(this->vertices);
        }
        
      vector<unsigned int> *get_triangles()
        {
          return &(this->triangles);
        }
        
      void add_vertex(double x, double y, double z, double u, double v, double w, double normal_x, double normal_y, double normal_z)
        {
          Vertex3D new_vertex;
          new_vertex.position = glm::vec3(x,y,z);
          new_vertex.texture_coord = glm::vec3(u,v,w);
          new_vertex.normal = glm::vec3(normal_x,normal_y,normal_z);
          this->vertices.push_back(new_vertex);
        }
        
      void add_vertex(double x, double y, double z)
        {
          this->add_vertex(x,y,z,0.0,0.0,0.0,1.0,0.0,0.0);
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
            this->vertices[i].print();
            
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

Geometry3D make_triangle(float side_length)
  {
    Geometry3D result;
    float helper1, helper2, helper3;
    
    helper1 = (side_length / 2.0) / sin(M_PI / 3.0);
    helper2 = side_length * sin(M_PI / 6.0);
    helper3 = helper1 - side_length * cos(M_PI / 6.0);
    
    result.add_vertex(0.0,helper1,0.0,      0.0,0.0,0.0,  0.0,0.0,1.0); 
    result.add_vertex(-helper2,helper3,0.0,  0.0,0.0,0.0,  0.0,0.0,1.0); 
    result.add_vertex(helper2,helper3,0.0,  0.0,0.0,0.0,  0.0,0.0,1.0); 
  
    result.add_triangle(0,1,2);
    
    return result;
  }
  
Geometry3D make_quad(float width, float height)
  {
    Geometry3D result;
    
    float half_width = width / 2.0;
    float half_height = height / 2.0;
    
    result.add_vertex(-half_width,0,-half_height,0.0,0.0,0.0,0.0,1.0,0.0); 
    result.add_vertex(half_width,0,-half_height,0.0,0.0,0.0,0.0,1.0,0.0);   
    result.add_vertex(-half_width,0,half_height,0.0,0.0,0.0,0.0,1.0,0.0);  
    result.add_vertex(half_width,0,half_height,0.0,0.0,0.0,0.0,1.0,0.0);    
  
    result.add_triangle(1,0,2);
    result.add_triangle(1,2,3); 
    
    return result;
  }
  
Geometry3D make_box(float width, float height, float depth)
  {
    Geometry3D result;
    
    float half_width = width / 2.0;
    float half_height = height / 2.0;
    float half_depth = depth / 2.0;
    
    result.add_vertex(-half_width,-half_height,-half_depth,0.0,0.0,0.0,-1.0,-1.0,-1.0); // 0
    result.add_vertex(half_width,-half_height,-half_depth,0.0,0.0,0.0,1.0,-1.0,-1.0);   // 1
    result.add_vertex(-half_width,half_height,-half_depth,0.0,0.0,0.0,-1.0,1.0,-1.0);   // 2
    result.add_vertex(half_width,half_height,-half_depth,0.0,0.0,0.0,1.0,1.0,-1.0);     // 3

    result.add_vertex(-half_width,-half_height,half_depth,0.0,0.0,0.0,-1.0,-1.0,1.0);   // 4
    result.add_vertex(half_width,-half_height,half_depth,0.0,0.0,0.0,1.0,-1.0,1.0);     // 5
    result.add_vertex(-half_width,half_height,half_depth,0.0,0.0,0.0,-1.0,1.0,1.0);     // 6
    result.add_vertex(half_width,half_height,half_depth,0.0,0.0,0.0,1.0,1.0,1.0);       // 7
    
    result.add_triangle(1,0,2); // front
    result.add_triangle(1,2,3); 
    
    result.add_triangle(4,5,6); // back
    result.add_triangle(6,5,7);
    
    result.add_triangle(2,0,4); // left
    result.add_triangle(2,4,6);
    
    result.add_triangle(1,3,5); // right
    result.add_triangle(5,3,7);
    
    result.add_triangle(3,2,6); // top
    result.add_triangle(7,3,6);
    
    result.add_triangle(0,1,4); // bottom
    result.add_triangle(4,1,5);
    
    return result;
  }
  
  /**
    Parses the data contained in one line of obj file format
    (e.g. "v 1.5 3 4.2" or "f 1/2 3/5 4/6 1/20").
    @param data in this variable the parsed data will be returned, the
       first index represents the element number (i.e. x, y, z for
       a vertex or one of the triangle indices) and the second index
       represents one of up to 3 shashed values (if the values are
       in format a/b/3), if any of the values is not present, -1.0
       is inserted.
   */
  
void parse_obj_line(string line,float data[4][3])
  {
    line = line.substr(line.find_first_of(' '));  // get rid of the first characters

    unsigned int i,j;
    size_t position;
    bool do_break;

    for (i = 0; i < 4; i++)
      for (j = 0; j < 3; j++)
        data[i][j] = -1.0;

    for (i = 0; i < 4; i++)
      {
        for (j = 0; j < 3; j++)
          {
            do_break = false;

            try
              {
                if (line.length() >= 1)
                  data[i][j] = stof(line,&position);

                if (line[position] != '/')
                  do_break = true;

                if (position + 1 <= line.length())
                  line = line.substr(position + 1);
                else
                  return;

                if (do_break)
                  break;
              }
            catch (exception& e)
              {
              }
          }
      }
  }
  
Geometry3D load_obj(string filename)
  {
    Geometry3D result;
    
    ifstream obj_file(filename.c_str());
    string line;
    float obj_line_data[4][3];
    glm::vec3 helper_point;

    vector<glm::vec3> normals;
    vector<glm::vec3> texture_vertices;

    if (!obj_file.is_open())
      {
        ErrorWriter::write_error("couldn't open file '" + filename + "'.");
        return result;
      }

    while (getline(obj_file,line))
      {
        switch (line[0])
          {
            case 'v':
              if (line[1] == 'n')        // normal vertex
                {
                  parse_obj_line(line,obj_line_data);

                  helper_point.x = obj_line_data[0][0];
                  helper_point.y = obj_line_data[1][0];
                  helper_point.z = obj_line_data[2][0];

                  normals.push_back(helper_point);
                  break;
                }
              else if (line[1] == 't')   // texture vertex
                {
                  parse_obj_line(line,obj_line_data);

                  helper_point.x = obj_line_data[0][0];
                  helper_point.y = obj_line_data[1][0];
                  helper_point.z = 0;

                  texture_vertices.push_back(helper_point);
                  break;
                }
              else                       // position vertex
                {
                  parse_obj_line(line,obj_line_data);
                  result.add_vertex(obj_line_data[0][0],obj_line_data[1][0],obj_line_data[2][0],0.0,0.0,0.0,1.0,0.0,0.0);
                  break;
                }

            case 'f':
              unsigned int indices[4],i,faces;

              parse_obj_line(line,obj_line_data);

              for (i = 0; i < 4; i++)     // triangle indices
                indices[i] = floor(obj_line_data[i][0]) - 1;

              if (obj_line_data[3][0] < 0.0)
                {
                  result.add_triangle(indices[0],indices[1],indices[2]);
                  faces = 3;     // 3 vertex face
                }
              else
                {
                  result.add_triangle(indices[0],indices[1],indices[2]);
                  result.add_triangle(indices[0],indices[2],indices[3]);
                  faces = 4;     // 4 vertex face
                }

              unsigned int vt_index, vn_index;

              for (i = 0; i < faces; i++)    // texture coordinates and normals
                {
                  vt_index = floor(obj_line_data[i][1]) - 1;
                  vn_index = floor(obj_line_data[i][2]) - 1;

                  if (indices[i] >= result.vertices.size())
                    continue;
                  
                  if (vt_index < texture_vertices.size() && vt_index >= 0)
                    {
                      result.vertices[indices[i]].texture_coord.x = texture_vertices[vt_index].x;
                      result.vertices[indices[i]].texture_coord.y = texture_vertices[vt_index].y;
                      result.vertices[indices[i]].texture_coord.z = texture_vertices[vt_index].z;
                    }
                  
                  if (vn_index < normals.size() && vn_index >= 0)
                    {
                      result.vertices[indices[i]].normal.x = normals[vn_index].x;
                      result.vertices[indices[i]].normal.y = normals[vn_index].y;
                      result.vertices[indices[i]].normal.z = normals[vn_index].z;
                    }
                }

              break;

            default:
              break;
          }
      }

    obj_file.close();
    return result;
  }
  
#endif