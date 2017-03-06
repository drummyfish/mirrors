#ifndef GL_WRAPPER_H
#define GL_WRAPPER_H

#define GLM_FORCE_RADIANS

/**
 * @file gl_wrapper_h
 * 
 * Simple OpenGL helper header-only library.
 * 
 * @author Miloslav Číž
 */

#define TEXEL_TYPE_COLOR 0
#define TEXEL_TYPE_DEPTH 1
#define TEXEL_TYPE_STENCIL 2

#include <stdio.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <ctime>
#include <iostream>
#include <string>
#include <string.h>
#include <string>
#include <fstream>
#include <streambuf>
#include <cstdint>
#include <glm/gtc/type_ptr.hpp>
#include <stdint.h>

std::string __vs_quad_text =
  "#version 330\n"
  "layout (location = 0) in vec3 position;\n"
  "layout (location = 1) in vec3 texture_coords;\n"
  "layout (location = 2) in vec3 normal;\n"
  "out vec4 transformed_position;\n"
  "out vec2 uv_coords;\n"
  "void main() {\n"
  "  transformed_position = vec4(position,1.0);\n"
  "  gl_Position = transformed_position;\n"
  "  uv_coords = texture_coords.xy;\n"
  "}\n";

#define VERTEX_SHADER_QUAD_TEXT __vs_quad_text ///< vertex shader code, can be used for drawind quads
  
using namespace std;

// necessary forward declarations:
  
void draw_fullscreen_quad(int,int);
static string file_text(string, bool);

// -------------------------------

/**
 * An object that can be printed.
 */
  
class Printable
  {
    public:
      virtual void print() = 0;
  };
  
/**
 * An object that can be uploaded to GPU.
 */
  
class GPUObject
  {
    public:
      /**
       Updates object data to GPU.
       */
      
      virtual void update_gpu() = 0;
      
      /**
       If supported, loads the data from GPU and
       saves them into the object.
       */
      
      virtual void load_from_gpu()
        {
        }
  };

/**
 * Simple class for error outputs. The class uses static methods,
 * the error outputs can be disabled.
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

      /**
       * Calls glGetError() and if there is an error,
       * outputs the error (if error outputs are enabled).
       * 
       * @param label string that will be written alongside
       *   the error
       * @param print_ok if true, the OK message will be
       *   printed if there was no error
       */
        
      static void checkGlErrors(string label, bool print_ok = false)
        {
          if (!ErrorWriter::enabled)
            return;
          
          GLuint error = glGetError();
          
          const char *helper_string = (const char *) gluErrorString(error);
          
          string error_string = helper_string;
          
          if (error != 0)
            ErrorWriter::write_error("OpenGL error (" + label + ") " + to_string(error) + ": " + error_string);
          else
            {
              if (print_ok)
                cout << "OpenGL OK (" + label + ")" << endl;
            }
        }
  };

bool ErrorWriter::enabled = true;

/**
 * Writes out mat4 data type.
 */

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

void print_memory_bytes(void *address, int length)
  {
    unsigned char *char_address = (unsigned char *) address;
    unsigned char helper;
    
    cout << "address " << address << ":" << endl;
    
    for (int i = 0; i < length; i++)
      {
        helper = char_address[i];
        cout << ((int) helper) << " ";
      }
      
    cout << endl;
  }
  
/**
 * Writes out vec3 data type.
 */
        
void print_vec3(glm::vec3 vector)
  {
    cout << vector.x << " " << vector.y << " " << vector.z << endl;
  }

static string preprocess_text(string text)
  {
    int include_limit = 255;
    bool include_found;
    size_t position, position2,position3;
    string include_string = "#include ";
    string filename2;
    
    while (include_limit >= 0) // each iteration replaces one #include
      {
        position = text.find(include_string);
        include_found = position != string::npos;
            
        if (!include_found)
          break;
            
        position3 = position + include_string.length();
        position2 = text.find("\n",position3);
        filename2 = text.substr(position3,position2 - position3);
        text = text.replace(position,position2 - position,file_text(filename2,false));
        include_limit--;
      }
          
    if (include_limit < 0)
      ErrorWriter::write_error("Include limit reached."); 

    return text;    
  }
  
/**
  * Gets a text of given file.
  * 
  * @param preprocess if true, directives such as #include will be
  *   preprocessed, otherwise not
  */
      
static string file_text(string filename, bool preprocess=false)
  {
    std::ifstream stream(filename);
    
    if (!stream.is_open())
      {
        ErrorWriter::write_error("Could not open file '" + filename + "'.");
      }
    
    std::string result((std::istreambuf_iterator<char>(stream)),std::istreambuf_iterator<char>());
       
    if (preprocess)
      result = preprocess_text(result);
          
    return result;
  }
  
/**
 * Gets frame buffer pixel. This function is slow and is intended for
 * debugging purposes.
 * 
 * @param x x pixel coordinate (starting from left)
 * @param y y pixel coordinate (starting from bottom)
 */
  
glm::vec3 get_framebuffer_pixel(unsigned int x, unsigned int y)
  {
    glm::vec3 result;
    float data[3];
    
    glReadPixels(x,y,1,1,GL_RGB,GL_FLOAT,data);

    result.x = data[0];
    result.y = data[1];
    result.z = data[2];
    
    return result;
  }
  
/**
 * Gets a debug info encoded in one pixel. This function is only
 * to be used for debugging purposes.
 * 
 * @param x pixel coordinate (starting from left)
 * @param y pixel coordinate (starting from bottom)
 * @param n value that was used to encode the information
 * @return value decoded from pixel at [x,y] position using value n
 */
  
glm::vec3 get_pixel_encoded_info(unsigned int x, unsigned int y, double n)
  { 
    glm::vec3 result;
    
    result = get_framebuffer_pixel(x,y);
    result = result - glm::vec3(0.5,0.5,0.5);
    n *= 2;
    result *= glm::vec3(n,n,n);
    return result;
  }
  
/**
 * A singleton class representing an OpenGL session. An object of this class has to be
 * created before most other objects can be created.
 */

class GLSession
  {
    protected:
      static GLSession *instance;           ///< singleton instance
      static bool initialised;              ///< whether the session has been initialised
      
      /**
       * Initialises the GLSession instance with default values. Usage:
       * call GLSession::get_instance() to get the object, optionally
       * modify some of its attributes, then call init() on the object and
       * start() to start the rendering loop. At the end of the program
       * call GLSession::clear();
       */
      
      GLSession()
        {
          // set the default parameters:
          
          this->argc = 0;
          this->argv = 0;
          this->display_mode = GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL;
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
          this->mouse_callback = 0;                      ///< for mouse press and releases, signature: void f(int button, int state, int x, int y)
          this->reshape_callback = 0;
          this->mouse_pressed_motion_callback = 0;       ///< for mouse movement with mouse buttons pressed, signature: void f(int x, int y)
          this->mouse_not_pressed_motion_callback = 0;   ///< for mouse movement without mouse buttons pressed, signature: void f(int x, int y)            
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
          glutMouseFunc(this->mouse_callback); 
          glutPassiveMotionFunc(this->mouse_pressed_motion_callback);
          glutMotionFunc(this->mouse_not_pressed_motion_callback);
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

          glEnable(GL_DEPTH_TEST);
          glEnable(GL_CULL_FACE);
          
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

class Transformation: public Printable
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
  
      virtual void recompute_final_matrix() = 0;
      virtual void recompute_translation_matrix() = 0;
      virtual void recompute_rotation_matrix() = 0;
      virtual void recompute_scale_matrix() = 0; 
        
    public:
      /**
       * Initialises a new instance.
       */
      
      TransformationTRS()
        { 
          this->translation = glm::vec3(0.0,0.0,0.0);
          this->rotation = glm::vec3(0.0,0.0,0.0);
          this->scale = glm::vec3(1.0,1.0,1.0);
        };

      /**
       * Copy constructor.
       */
      
      TransformationTRS(TransformationTRS *transform)
        { 
          this->translation = transform->get_translation();
          this->rotation = transform->get_rotation();
          this->scale = transform->get_scale();
        };
        
      /**
       * Sets the translation for the transformation.
       */
        
      void set_translation(glm::vec3 new_translation)
        {
          this->translation = new_translation;
          this->recompute_translation_matrix();
        };
        
      /**
       * Adds translation vector to current translation value of
       * the transformation.
       */
        
      void add_translation(glm::vec3 translation)
        {
          this->translation = this->translation + translation;
          this->recompute_translation_matrix();
        };
        
      /**
       * Adds translation in "forward" direction.
       */
        
      void add_translation_forward(float distance)
        {
          glm::vec3 helper;
          helper = this->get_direction_forward();
          helper *= distance;
          this->add_translation(helper);
        };
        
      glm::vec3 get_direction(glm::vec3 initial_vector)
        {
          glm::mat4 transform = glm::mat4(1.0f);
          
          transform = glm::rotate(transform,this->rotation.y,glm::vec3(0,1.0f,0));  // around y
          transform = glm::rotate(transform,this->rotation.x,glm::vec3(1.0f,0,0));  // around x
          transform = glm::rotate(transform,this->rotation.z,glm::vec3(0,0,1.0f));  // around z

          return glm::vec3(transform * glm::vec4(initial_vector,0.0f));
        }
        
      /**
       * Gets a unit vector pointing in direction of current
       * rotation (i.e. "forward").
       */
        
      glm::vec3 get_direction_forward()
        {
          return this->get_direction(glm::vec3(0.0f,0.0f,-1.0f));
        };
        
      glm::vec3 get_direction_left()
        {
          return this->get_direction(glm::vec3(-1.0f,0.0f,0.0f));
        };
        
      glm::vec3 get_direction_up()
        {
          return this->get_direction(glm::vec3(0.0f,1.0f,0.0f));
        };
        
      /**
       * Adds rotation vector to current rotation value of
       * the transformation.
       */
        
      void add_rotation(glm::vec3 rotation)
        {
          this->rotation = this->rotation + rotation;
          this->recompute_rotation_matrix();
        }
        
      /**
       * Sets the rotation for the transformation.
       */
        
      void set_rotation(glm::vec3 new_rotation)
        { 
          this->rotation = new_rotation;
          this->recompute_rotation_matrix();
        };
        
      /**
       * Sets the scale for the transformation.
       */
        
      void set_scale(glm::vec3 new_scale)
        {
          this->scale = new_scale;
          this->recompute_scale_matrix();
        };
        
      glm::vec3 get_translation()
        {
          return this->translation;
        };
      
      glm::vec3 get_rotation()
        {
          return this->rotation;
        };
        
      glm::vec3 get_scale()
        {
          return this->scale;
        };
        
      /**
       * Prints the transformation to stdout.
       */
        
      virtual void print()
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
 * Transformation consisting of translation, rotation and scale for models.
 * Rotation is done in following order: around z (roll), around y (yaw),
 * around x (pitch). 
 */
  
class TransformationTRSModel: public TransformationTRS
  {
    protected:
      virtual void recompute_final_matrix()
        {
          this->final_matrix = this->translation_matrix * this->rotation_matrix * this->scale_matrix;
        };
    
      virtual void recompute_translation_matrix()
        {
          this->translation_matrix = glm::translate(glm::mat4(1.0f),this->translation);
          this->recompute_final_matrix();
        };
    
      virtual void recompute_rotation_matrix()
        {
          this->rotation_matrix = glm::mat4(1.0f);
          this->rotation_matrix = glm::rotate(this->rotation_matrix,this->rotation.y,glm::vec3(0,1,0));  // around y
          this->rotation_matrix = glm::rotate(this->rotation_matrix,this->rotation.x,glm::vec3(1,0,0));  // around x
          this->rotation_matrix = glm::rotate(this->rotation_matrix,this->rotation.z,glm::vec3(0,0,1));  // around z
          this->recompute_final_matrix();
        };
        
      virtual void recompute_scale_matrix()
        {
          this->scale_matrix = glm::scale(glm::mat4(1.0f),this->scale);
          this->recompute_final_matrix();
        };
  };
  
/**
 * Transformation consisting of translation, rotation and scale (not used)
 * for cameras. Use this transform to records the camera transform and then
 * apply it to all models in the scene, i.e. you get the view matrix.
 */
  
class TransformationTRSCamera: public TransformationTRS
  {
    protected:
      virtual void recompute_final_matrix()
        {
          this->final_matrix = this->rotation_matrix * this->translation_matrix;
        };
    
      virtual void recompute_translation_matrix()
        {
          this->translation_matrix = glm::translate(glm::mat4(1.0f),glm::vec3(-1 * this->translation.x,-1 * this->translation.y,-1 * this->translation.z));
          this->recompute_final_matrix();
        };
    
      virtual void recompute_rotation_matrix()
        {
          this->rotation_matrix = glm::mat4(1.0f);
          this->rotation_matrix = glm::rotate(this->rotation_matrix,-1 * this->rotation.z,glm::vec3(0,0,1));  // around z
          this->rotation_matrix = glm::rotate(this->rotation_matrix,-1 * this->rotation.x,glm::vec3(1,0,0));  // around x
          this->rotation_matrix = glm::rotate(this->rotation_matrix,-1 * this->rotation.y,glm::vec3(0,1,0));  // around y
          this->recompute_final_matrix();
        };
        
      virtual void recompute_scale_matrix()
        {
          // no scale for camera
        };
  };
  
/**
 * Represents an OpenGL shader.
 */
  
class Shader
  {
    protected:
      GLuint shader_program;
      bool is_ok;
      
      /**
       * Helper function.
       */
      
      bool add_shader(const char* shader_text, GLenum shader_type)
        {
          GLuint shader_object = glCreateShader(shader_type);

          const GLchar* p[1];
          p[0] = shader_text;

          if (shader_object == 0)
            {
              ErrorWriter::write_error("Could not create shader object.");
              ErrorWriter::checkGlErrors("rendering loop");
              return false;
            }
          
          GLint lengths[1];
          lengths[0]= strlen(shader_text);

          glShaderSource(shader_object,1,p,lengths);
          glCompileShader(shader_object);

          GLint success;

          glGetShaderiv(shader_object,GL_COMPILE_STATUS,&success);

          if (success == GL_FALSE)
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
        
      GLuint get_shader_program_number()
        {
          return this->shader_program;
        }
        
      /**
       * Gets the location of uniform variable by its name.
       */
        
      GLint get_uniform_location(string name)
        {
          GLint result;
          result = glGetUniformLocation(this->shader_program,name.c_str());
          
          return result;
        }
      
      /**
       * Initialises a new instance.
       * 
       * @param vertex_shader_text vertex shader source code
       * @param fragment_shader_text fragment shader source code
       * @param compute_shader_text compute shader source code
       * @param transform_feedback_variables list of output vertex shader variable names
       *   to be recorded in transform feedback, leave empty if you don't want to use
       *   transform feedback 
       */
      
      Shader(string vertex_shader_text, string fragment_shader_text, string compute_shader_text, vector<string> *transform_feedback_variables = 0)
        {    
          char log[256];
         
          this->is_ok = true;
          
          this->shader_program = glCreateProgram();
          
          if (this->shader_program == 0)
            ErrorWriter::write_error("Could not create a shader program.");
         
          if (compute_shader_text.length() != 0)
            {
              if (!this->add_shader(compute_shader_text.c_str(),GL_COMPUTE_SHADER))
                {
                  ErrorWriter::write_error("Could not add a compute shader program. (you need OpenGL 4.5 for compute shaders.)");
                  this->is_ok = false;
                }
            }
          else  // can either have compute shader or pipeline shaders
            {
              if (vertex_shader_text.length() != 0)
                if (!this->add_shader(vertex_shader_text.c_str(),GL_VERTEX_SHADER))
                  {
                    ErrorWriter::write_error("Could not add a vertex shader program.");
                    this->is_ok = false;
                  }
                    
              if (fragment_shader_text.length() != 0)
                if (!this->add_shader(fragment_shader_text.c_str(),GL_FRAGMENT_SHADER))
                  {
                    ErrorWriter::write_error("Could not add a fragment shader program.");
                    this->is_ok = false;
                  }
            }
            
          GLint success = 0;
          
          // init transform feedback:
          
          if (transform_feedback_variables != 0 && transform_feedback_variables->size() > 0)
            {
              char *variable_names[512];
          
              unsigned int i;
              
              for (i = 0; i < transform_feedback_variables->size() && i < 512; i++)
                variable_names[i] = (char *) transform_feedback_variables->at(i).c_str();
          
              glTransformFeedbackVaryings(this->shader_program,transform_feedback_variables->size(),(const GLchar **) variable_names,GL_INTERLEAVED_ATTRIBS);
            }
          
          glLinkProgram(this->shader_program);
          glGetProgramiv(this->shader_program,GL_LINK_STATUS,&success);
       
          if (success == 0)
            {
              glGetProgramInfoLog(this->shader_program,sizeof(log),NULL,log);
              cerr << log << endl;
              ErrorWriter::write_error("Could not link the shader program.");
              this->is_ok = false;
            }
            
          glValidateProgram(this->shader_program);
          
          glGetProgramiv(shader_program,GL_VALIDATE_STATUS,&success);
          
          if (!success)
            {
              ErrorWriter::write_error("The shader program is invalid.");
              this->is_ok = false;
            }
        }
        
      bool loaded_succesfully()
        {
          return this->is_ok;
        }
        
      /**
       * Runs the compute shader, use() must have been called before this.
       */
      
      void run_compute(unsigned int groups_x, unsigned int groups_y, unsigned int groups_z)
        {
          glDispatchCompute(groups_x,groups_y,groups_z);
        }
  };
  
/**
 * Represnts a uniform shader variable.
 */

class UniformVariable
  {
    protected:
      GLint location;
      bool location_retrieved;
      string name;
      
      void pre_update_check()
        {
          //if (!this->location_retrieved)
          //  ErrorWriter::write_error("Updating uniform variable without its location retrieved.");
        }
      
    public:
      UniformVariable(string name)
        {
          this->name = name;
          this->location = 0;
          location_retrieved = false;
        }
        
      int get_location()
        {
          return this->location;
        }
        
      /**
       * Retrieves the location from given shader.
       */
        
      bool retrieve_location(Shader *shader)
        {
          this->location = shader->get_uniform_location(this->name.c_str());
        
          if (this->location < 0)
            {
              ErrorWriter::write_error("Uniform variable '" + this->name + "' couldn't retrieve its location.");
              return false;
            }
            
          this->location_retrieved = true;
          return true;
        }
        
      void update_uint(unsigned int value)
        {
          this->pre_update_check();
          glUniform1ui(this->location,value);
        }
        
      void update_int(int value)
        {
          this->pre_update_check();
          glUniform1i(this->location,value);
        }

      void update_mat3(glm::mat3 value)
        {
          this->pre_update_check();
          glUniformMatrix3fv(this->location,1,GL_TRUE,glm::value_ptr(value));
        }
        
      void update_mat4(glm::mat4 value)
        {
          this->pre_update_check();
          glUniformMatrix4fv(this->location,1,GL_TRUE,glm::value_ptr(value));
        }
        
      void update_vec3(glm::vec3 value)
        {
          this->pre_update_check();
          glUniform3fv(this->location,1,glm::value_ptr(value));
        }
        
      void update_float_3(float value1, float value2, float value3)
        {
          this->pre_update_check();
          glUniform3f(this->location,value1,value2,value3);
        }
  };
  
/**
 * Represents a 3D vertex with position, normal and texture
 * coordinates.
 */
  
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
  
/**
 * Represents a texel (texture pixel).
 */
  
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
 * Raster image, intended for use with textures.
 */
  
class Image2D: public Printable
  {
    protected:
      vector<Texel> data;         
      vector<float> data_float;   
      vector<int> data_int;
      unsigned int width;
      unsigned int height;
      int data_type;              // which vector to use
      
      /**
       * Converts 2D coordinates to 1D index.
       */
      
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
      Image2D(unsigned int width, unsigned int height, unsigned int data_type)
        {      
          this->data_type = data_type;
          this->set_size(width,height);
        }

      /**
       * Copy constructor.
       */
        
      Image2D(Image2D *image)
        {
          this->data_type = image->get_data_type();
          this->set_size(image->get_width(),image->get_height());
          
          for (int j = 0; j < image->get_height(); j++)
            for (int i = 0; i < image->get_width(); i++)
              {
                float r,g,b,a;
                
                image->get_pixel(i,j,&r,&g,&b,&a);
                this->set_pixel(i,j,r,g,b,a);
              }
        }
        
      virtual ~Image2D()
        {
        }
        
      int get_width()
        {
          return this->width;
        }
        
      int get_height()
        {
          return this->height;
        }
        
      int get_data_type()
        {
          return this->data_type;
        }
        
      /**
       * Filles the whole image with given color.
       */
        
      void fill(float r, float g, float b, float a)
        {
          int i,j;
          
          for (j = 0; j < this->get_height(); j++)
            for (i = 0; i < this->get_width(); i++)
              this->set_pixel(i,j,r,g,b,a);
        }
        
      /**
       * Filles the image with (0,0,0,0) color.
       */
        
      void clear()
        {
          this->fill(0,0,0,0);
        }
        
      void set_size(unsigned int width, unsigned int height)
        {
          unsigned int i;
          float helper = 0.0;
          
          this->width = width;
          this->height = height;
        
          this->data.clear();
          
          switch (this->data_type)
            {
              case TEXEL_TYPE_COLOR:
                this->data.clear();
                
                for (i = 0; i < width * height; i++)
                  this->data.push_back(Texel());
                break;
              
              case TEXEL_TYPE_DEPTH:
                this->data_float.clear();
                
                for (i = 0; i < width * height; i++)
                  this->data_float.push_back(helper);
                break;
                
              case TEXEL_TYPE_STENCIL:
                this->data_int.clear();
                
                for (i = 0; i < width * height; i++)
                  this->data_int.push_back(0);
                break;
                
              default:
                break;
            }
        }
        
      /**
       * Loads the image from ppm file format.
       */
        
      bool load_ppm(string filename)
        {
          string error_string = "Could not load texture from file '" + filename + "'.";
          char buffer[16];
          FILE *file_handle;
          int character, rgb_component;
          unsigned int x, y, i;
          
          file_handle = fopen(filename.c_str(),"rb");

          if (!file_handle)
            {
              ErrorWriter::write_error(error_string + " (File could not be opened.)");
              return false;
            }
              
          if (!fgets(buffer,sizeof(buffer),file_handle))   // file format
            {
              ErrorWriter::write_error(error_string + " (File format signature not present.)");
              return false;
            }
            
          character = getc(file_handle);                   // skip comments

          while (character == '#')
            {
              while (getc(file_handle) != '\n');        
              character = getc(file_handle);
            }

          ungetc(character,file_handle);

          // image size:

          if (fscanf(file_handle,"%d %d",(int *) &this->width,(int *) &this->height) != 2)
            {
              ErrorWriter::write_error(error_string + " (Width and height not available in the file.)");
              fclose(file_handle);        
              return false;
            }

          // rgb component:

          if (fscanf(file_handle,"%d",&rgb_component) != 1)
            {
              ErrorWriter::write_error(error_string + " (RGB component information not present in the file.)");
              fclose(file_handle);
              return false;
            }

          if (rgb_component != 255)  // check the depth
            {
              ErrorWriter::write_error(error_string + " (Unsupported color depth.)");
              fclose(file_handle);
              return false;
            }

          while (fgetc(file_handle) != '\n');

          this->set_size(this->width,this->height);
        
          //read pixel data:

          unsigned char *data_buffer;
          
          data_buffer = (unsigned char *) malloc(3 * this->width * this->height);
          
          if (fread(data_buffer,3 * this->width,this->height,file_handle) != this->height)
            {
              ErrorWriter::write_error(error_string + " (Error reading the pixel data.)");
              fclose(file_handle);        
              return false;
            }
            
          i = 0;
            
          for (y = 0; y < this->height; y++)
            for (x = 0; x < this->width; x++)
              {
                this->set_pixel(x,y,data_buffer[i] / 255.0,data_buffer[i + 1] / 255.0,data_buffer[i + 2] / 255.0,1.0);
                i += 3;
              }
              
          delete data_buffer;
            
          fclose(file_handle);

          return true;
        }
        
      /**
       * Saves the image to file in ppm format.
       */
        
      bool save_ppm(string filename, bool top_to_bottom=true)
        {
          int i,j;
          float r,g,b,a;

          FILE *file_handle;
          file_handle = fopen(filename.c_str(),"wb");

          if (!file_handle)
            return false;

          fprintf(file_handle,"P6 %d %d 255 ",this->width,this->height);

          int value_from, value_to;
          value_from = top_to_bottom ? 0 : this->height - 1;
          value_to = top_to_bottom ? this->height : -1;
          int increment = top_to_bottom ? 1 : -1;
          
          for (j = value_from; j != value_to; j += increment)
            for (i = 0; ((unsigned int) i) < this->width; i++)
              {
                this->get_pixel(i,j,&r,&g,&b,&a);
                fprintf(file_handle,"%c%c%c",
                  ((unsigned char) glm::clamp<float>((r * 255),0,255)),
                  ((unsigned char) glm::clamp<float>((g * 255),0,255)),
                  ((unsigned char) glm::clamp<float>((b * 255),0,255)));
              }

          fclose(file_handle);
            return true;
        }
        
      /**
       * Raises all pixels to given power (good for viewing depth maps). Good
       * value is 256.
       */
 
      void raise_to_power(unsigned int power)
        {
          float r,g,b,a;
          unsigned int i,j;
          
          for (j = 0; j < this->height; j++)
            for (i = 0; i < this->width; i++)
              {
                this->get_pixel(i,j,&r,&g,&b,&a);
                
                r = pow(r,power);
                g = pow(g,power);
                b = pow(b,power);
                a = pow(a,power);
                
                this->set_pixel(i,j,r,g,b,a);
              }
        }
        
      void multiply(double coefficient)
        {
          float r,g,b,a;
          unsigned int i,j;
          
          for (j = 0; j < this->height; j++)
            for (i = 0; i < this->width; i++)
              {
                this->get_pixel(i,j,&r,&g,&b,&a);
               
                r *= coefficient;
                g *= coefficient;
                b *= coefficient;
                a *= coefficient;
                
                this->set_pixel(i,j,r,g,b,a);
              }
        }
        
      /**
       * Sets pixels at given position to given value. The method behaves depending on the
       * texel type of the image, for TEXEL_TYPE_COLOR all arguments(r, g, b, a) are set
       * as they are, for TEXEL_TYPE_DEPTH only r argument is taken and for TEXEL_TYPE_STENCIL
       * only r argument is taken and rounded to int (so for example 254.8 becomes 255).
       */
        
      void set_pixel(unsigned int x, unsigned int y, float r, float g, float b, float a)
        { 
          Texel texel;
          
          switch (this->data_type)
            {
              case TEXEL_TYPE_COLOR:
                texel.red = r;
                texel.green = g;
                texel.blue = b;
                texel.alpha = a;
                
                this->data[this->coords_2d_to_1d(x,y)] = texel;            
                break;
              
              case TEXEL_TYPE_DEPTH:
                this->data_float[this->coords_2d_to_1d(x,y)] = r;
                break;
                
              case TEXEL_TYPE_STENCIL:
                this->data_int[this->coords_2d_to_1d(x,y)] = round(r);
                break;
                
              default:
                break;
            }
        }
      
      /**
       * Gets pixel at given position, the result is returned in r, g, b and a
       * arguments. The method behaves depending on the texel type set, for
       * TEXEL_TYPE_COLOR red, greed, blue and alpha values are returned as
       * they are, for TEXEL_TYPE_DEPTH the depth value will be returnd in
       * all output parameters and for TEXEL_TYPE_STENCIL the integer value
       * will be returned in all output parameters casted to float.
       */
      
      void get_pixel(int x, int y, float *r, float *g, float *b, float *a)
        {
          int index = this->coords_2d_to_1d(x,y);
          
          switch (this->data_type)
            {
              case TEXEL_TYPE_COLOR:
                *r = this->data[index].red;
                *g = this->data[index].green;
                *b = this->data[index].blue;
                *a = this->data[index].alpha;
                break;
              
              case TEXEL_TYPE_DEPTH:
                *r = *g = *b = *a = this->data_float[index];
                break;
                
              case TEXEL_TYPE_STENCIL:
                *r = *g = *b = *a = (int) this->data_int[index];
                break;
                
              default:
                break;
            }
        }
        
      /**
       * Returns the internal format to use (for glTexImage2D) for the texture as OpenGL enum value,
       * depending on the texel type of the image.
       */
        
      GLuint get_internal_format()
        {
          switch (this->data_type)
            {
              case TEXEL_TYPE_COLOR:
                return GL_RGBA32F;
                break;
              
              case TEXEL_TYPE_DEPTH:
                return GL_DEPTH_COMPONENT24;
                break;
                    
              case TEXEL_TYPE_STENCIL:
                return GL_LUMINANCE;
                break;
                
              default:
                return GL_RGBA;
                break;
            }
            
          return GL_RGBA;
        }

      /**
       * Returns the data format the texture uses (for glTexImage2D) as OpenGL enum value,
       * depending on the texel type of the image.
       */
        
      GLuint get_format()
        {
          switch (this->data_type)
            {
              case TEXEL_TYPE_COLOR:
                return GL_RGBA;
                break;
              
              case TEXEL_TYPE_DEPTH:
                return GL_DEPTH_COMPONENT;
                break;
                    
              case TEXEL_TYPE_STENCIL:
                return GL_LUMINANCE;
                break;
                
              default:
                return GL_RGBA;
                break;
            }
            
          return GL_RGBA;
        }

      /**
       * Returns the data type the texture uses (for glTexImage2D) as OpenGL enum value,
       * depending on the texel type of the image.
       */
        
      GLuint get_type()
        {
          switch (this->data_type)
            {
              case TEXEL_TYPE_COLOR:
                return GL_FLOAT;
                break;
              
              case TEXEL_TYPE_DEPTH:
                return GL_FLOAT;
                break;
                    
              case TEXEL_TYPE_STENCIL:
                return GL_INT;
                break;
                
              default:
                return GL_FLOAT;
                break;
            }
            
          return GL_FLOAT;
        }
        
      /**
       * Gets a pointer to image data.
       */
        
      void *get_data_pointer()
        {
          switch (this->data_type)
            {
              case TEXEL_TYPE_COLOR:
                return &(this->data[0]);
                break;

              case TEXEL_TYPE_DEPTH:
                return &(this->data_float[0]);
                break;
                
              case TEXEL_TYPE_STENCIL:
                return &(this->data_int[0]);
                break;
                
              default:
                return NULL;
                break;
            }
        }
        
      unsigned int get_data_size()
        {
          switch (this->data_type)
            {
              case TEXEL_TYPE_COLOR:
                return sizeof(this->data);
                break;

              case TEXEL_TYPE_DEPTH:
                return sizeof(this->data_float);
                break;
                
              case TEXEL_TYPE_STENCIL:
                return sizeof(this->data_int);
                break;
                
              default:
                return 0;
                break;
            }
        }
      
      virtual void print()
        {
          unsigned int x, y, index;
          
          for (y = 0; y < this->height; y++)
            for (x = 0; x < this->width; x++)
              {
                index = this->coords_2d_to_1d(x,y);
                
                cout << x << "; " << y << ": ";
                
                switch (this->data_type)
                  {
                    case TEXEL_TYPE_COLOR:
                      cout <<
                      this->data[index].red << ", " <<
                      this->data[index].green << ", " <<
                      this->data[index].blue << ", " <<
                      this->data[index].alpha << endl;
                      break;
                    
                    case TEXEL_TYPE_DEPTH:
                      cout << this->data_float[index] << endl; 
                      
                    default: break;
                  }
              }     
        }      
  };
  
/**
 * Abstract texture class.
 */
  
class Texture: public Printable, public GPUObject
  {
    protected:
      GLuint to;                     // texture object id
      unsigned int mipmap_level;     // which level of mipmap is being operated on on CPU

      /**
       * Return texture size based on currently set this->mipmap_level value.
       */
      
      unsigned int get_current_mipmap_size(unsigned int base_size)
        {
          return glm::max((unsigned int) 1,base_size / int(pow(2,this->mipmap_level)));
        }
      
    public:
      virtual void bind(unsigned int unit) = 0;  

      /**
       * Sets mipmap level that will be operated on on CPU (i.e.
       * functions such as load_from_gpu() will be affected). This
       * function has to set the this->mipmap_level value and
       * resize the internal images.
       */
        
      virtual void set_mipmap_level(unsigned int level) = 0;
      
      GLuint get_texture_object()
        {
          return this->to;
        }
  };
  
/**
 * Represents a cubemap texture.
 */
  
class TextureCubeMap: public Texture
  {
    protected:
      unsigned int size;
      unsigned int texel_type;
      Image2D *images[6];
      
    public:
      Image2D *image_front;
      Image2D *image_back;
      Image2D *image_left;
      Image2D *image_right;
      Image2D *image_top;
      Image2D *image_bottom;
      
      /**
       * Initialises a new cube map object.
       * 
       * @size width and height resolution in pixels (cube map must
       *   have square size)
       */
      
      TextureCubeMap(unsigned int size, unsigned int texel_type = TEXEL_TYPE_COLOR)
        {
          this->size = size;
          this->texel_type = texel_type;
          glGenTextures(1,&(this->to));
          this->mipmap_level = 0;
          
          glBindTexture(GL_TEXTURE_CUBE_MAP,this->to);
          glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
          glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
          glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);           
          glBindTexture(GL_TEXTURE_CUBE_MAP,0);

          this->image_front = new Image2D(size,size,texel_type);
          this->image_back = new Image2D(size,size,texel_type);
          this->image_left = new Image2D(size,size,texel_type);
          this->image_right = new Image2D(size,size,texel_type);
          this->image_top = new Image2D(size,size,texel_type);
          this->image_bottom = new Image2D(size,size,texel_type);
          
          this->images[0] = this->image_front;
          this->images[1] = this->image_back;
          this->images[2] = this->image_left;
          this->images[3] = this->image_right;
          this->images[4] = this->image_bottom;
          this->images[5] = this->image_top;
        }
 
      /**
       * Returns an image of specified cubemap side.
       * 
       * @param side OpenGL cubemap side constant: GL_TEXTURE_CUBE_MAP_{NEGATIVE|POSITIOVE}_{X|Y|Z})
       */
 
      Image2D *get_texture_image(GLuint side)
        {
          switch (side)
            {
              case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: return this->images[0];
              case GL_TEXTURE_CUBE_MAP_POSITIVE_Z: return this->images[1];
              case GL_TEXTURE_CUBE_MAP_NEGATIVE_X: return this->images[2];
              case GL_TEXTURE_CUBE_MAP_POSITIVE_X: return this->images[3];
              case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: return this->images[4];
              case GL_TEXTURE_CUBE_MAP_POSITIVE_Y: return this->images[5];
              default: return NULL; break;
            }
        }
 
      virtual void set_mipmap_level(unsigned int level)
        {
          this->mipmap_level = level;
          unsigned int mipmap_size = this->get_current_mipmap_size(this->size);    
          this->image_front->set_size(mipmap_size,mipmap_size);
          this->image_back->set_size(mipmap_size,mipmap_size);
          this->image_left->set_size(mipmap_size,mipmap_size);
          this->image_right->set_size(mipmap_size,mipmap_size);
          this->image_top->set_size(mipmap_size,mipmap_size);
          this->image_bottom->set_size(mipmap_size,mipmap_size);
        }
 
      /**
       Raises all pixels to given power (good for viewing depth maps). Good
       value is 256.
       */
 
      void raise_to_power(unsigned int power)
        {
          this->image_front->raise_to_power(power);
          this->image_back->raise_to_power(power);
          this->image_left->raise_to_power(power);
          this->image_right->raise_to_power(power);
          this->image_top->raise_to_power(power);
          this->image_bottom->raise_to_power(power);
        }
 
      void multiply(double coefficient)
        {
          this->image_front->multiply(coefficient);
          this->image_back->multiply(coefficient);
          this->image_left->multiply(coefficient);
          this->image_right->multiply(coefficient);
          this->image_top->multiply(coefficient);
          this->image_bottom->multiply(coefficient);
        }
 
      virtual ~TextureCubeMap()
        {
          delete this->image_front;
          delete this->image_back;
          delete this->image_left;
          delete this->image_right;
          delete this->image_top;
          delete this->image_bottom;
        }
        
      virtual void update_gpu()
        {
          int i;
          Image2D *images[] =
            {this->image_front,
             this->image_back,
             this->image_left,
             this->image_right,
             this->image_bottom,
             this->image_top};
          
          GLuint targets[] =
            {GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
             GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
             GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
             GL_TEXTURE_CUBE_MAP_POSITIVE_X,
             GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
             GL_TEXTURE_CUBE_MAP_POSITIVE_Y};
          
          glBindTexture(GL_TEXTURE_CUBE_MAP,this->to);

          for (i = 0; i < 6; i++)
            {
              glTexImage2D(
                targets[i],
                this->mipmap_level,
                images[i]->get_internal_format(),
                this->image_front->get_width(),
                this->image_front->get_height(),
                0,
                images[i]->get_format(),
                images[i]->get_type(),
                images[i]->get_data_pointer()
                );    
            }
                  
          glBindTexture(GL_TEXTURE_CUBE_MAP,0);
        }
        
      virtual void load_from_gpu()
        {
          int i;
          
          glBindTexture(GL_TEXTURE_CUBE_MAP,this->to);
          
          GLuint targets[] =
            {GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
             GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
             GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
             GL_TEXTURE_CUBE_MAP_POSITIVE_X,
             GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
             GL_TEXTURE_CUBE_MAP_POSITIVE_Y};
          
          for (i = 0; i < 6; i++)
            {
              switch (this->texel_type)
                {
                  case TEXEL_TYPE_COLOR:
                    glGetTexImage(targets[i],this->mipmap_level,GL_RGBA,GL_FLOAT,this->images[i]->get_data_pointer());
                    break;
                    
                  case TEXEL_TYPE_DEPTH:
                    glGetTexImage(targets[i],this->mipmap_level,GL_DEPTH_COMPONENT,GL_FLOAT,this->images[i]->get_data_pointer());
                    break;
                    
                  default:
                    break;
                }
            }
        }
        
      bool load_ppms(string front, string back, string left, string right, string bottom, string top)
        {
          bool result = true;
          
          result = result && this->image_front->load_ppm(front);
          result = result && this->image_back->load_ppm(back);
          result = result && this->image_left->load_ppm(left);
          result = result && this->image_right->load_ppm(right);
          result = result && this->image_bottom->load_ppm(bottom);
          result = result && this->image_top->load_ppm(top);
          
          auto lambda_size_ok = [](Image2D *image, int size)
            {
              return (image->get_width() == size) && (image->get_height() == size);
            };
          
          if (!lambda_size_ok(this->image_front,this->size) ||
              !lambda_size_ok(this->image_back,this->size) ||
              !lambda_size_ok(this->image_left,this->size) ||
              !lambda_size_ok(this->image_right,this->size) ||
              !lambda_size_ok(this->image_top,this->size) ||
              !lambda_size_ok(this->image_bottom,this->size))
            ErrorWriter::write_error("Loaded cubemap images don't have the same size.");
            
          return result;
        }
        
      void save_ppms(string name)
        {
          this->image_front->save_ppm(name + "_front.ppm");
          this->image_back->save_ppm(name + "_back.ppm");
          this->image_left->save_ppm(name + "_left.ppm");
          this->image_right->save_ppm(name + "_right.ppm");
          this->image_top->save_ppm(name + "_top.ppm");
          this->image_bottom->save_ppm(name + "_bottom.ppm");
        }
        
      virtual void print()
        {
          cout << "front:" << endl;
          this->image_front->print();
          cout << "back:" << endl;
          this->image_back->print();
          cout << "left:" << endl;
          this->image_left->print();
          cout << "right:" << endl;
          this->image_right->print();
          cout << "bottom:" << endl;
          this->image_bottom->print();
          cout << "top:" << endl;
          this->image_top->print();
        }
        
      virtual void bind(unsigned int unit)
        {
          switch (unit)
            {
              case 0: glActiveTexture(GL_TEXTURE0); break;
              case 1: glActiveTexture(GL_TEXTURE1); break;
              case 2: glActiveTexture(GL_TEXTURE2); break;
              case 3: glActiveTexture(GL_TEXTURE3); break;
              case 4: glActiveTexture(GL_TEXTURE4); break;
              case 5: glActiveTexture(GL_TEXTURE5); break;
              case 6: glActiveTexture(GL_TEXTURE6); break;
              case 7: glActiveTexture(GL_TEXTURE7); break;
              case 8: glActiveTexture(GL_TEXTURE8); break;
              case 9: glActiveTexture(GL_TEXTURE9); break;
              
              default:
                break;
            }
          
          glBindTexture(GL_TEXTURE_CUBE_MAP,this->to);
        }
  };
      
/**
 * RGBA 2D texture.
 */
  
class Texture2D: public Texture
  {
    protected:
      Image2D *image_data;
      unsigned int base_width;
      unsigned int base_height;
      
    public:
      /**
       * Initialises a new texture.
       * 
       * @param width width int pixels
       * @param height height int pixels
       * @param texel_type texel type for the texture, possible values are
       *        defined in this file (TEXEL_TYPE_COLOR, TEXEL_TYPE_DEPTH, ...)
       */
      
      Texture2D(unsigned int width, unsigned int height, unsigned int texel_type = TEXEL_TYPE_COLOR)
        {
          this->image_data = new Image2D(width,height,texel_type);
          glGenTextures(1,&(this->to));
          
          this->mipmap_level = 0;
          this->base_width = width;
          this->base_height = height;
        }
      
      virtual void set_mipmap_level(unsigned int level)
        {
          this->mipmap_level = level;
          this->image_data->set_size(this->get_current_mipmap_size(base_width),this->get_current_mipmap_size(base_height));
        }
      
      virtual ~Texture2D()
        {
          delete this->image_data;
        }
        
      Image2D *get_image_data()
        {
          return this->image_data;
        }
        
      float get_max_value()
        {
          int i,j;
          float r, g, b, a;
          float max;
          
          r = 0;
          g = 0;
          b = 0;
          max = 0;
          
          for (j = 0; j < this->get_image_data()->get_height(); j++)
            for (i = 0; i < this->get_image_data()->get_width(); i++)
              {
                this->get_image_data()->get_pixel(i,j,&r,&g,&b,&a);
                
                if (r > max)
                  max = r;
                
                if (g > max)
                  max = g;
                
                if (b > max)
                  max = b;
              }
              
          return max;
        }
        
      /**
       * Loads the texture from ppm file format.
       */
        
      bool load_ppm(string filename)
        {
          return this->image_data->load_ppm(filename);
        }
       
      /**
       * Uploads the texture data to GPU.
       */
        
      virtual void update_gpu()
        {
          glBindTexture(GL_TEXTURE_2D,this->to);
        
          glTexImage2D(
            GL_TEXTURE_2D,
            0,
            this->image_data->get_internal_format(),
            this->image_data->get_width(),
            this->image_data->get_height(),
            0,
            this->image_data->get_format(),
            this->image_data->get_type(),
            this->image_data->get_data_pointer());
          
          glGenerateMipmap(GL_TEXTURE_2D);
          glBindTexture(GL_TEXTURE_2D,0);
        }
        
      virtual void load_from_gpu()
        {
          glBindTexture(GL_TEXTURE_2D,this->to);    
          
          if (this->image_data->get_data_type() == TEXEL_TYPE_STENCIL)
            glGetTexImage(GL_TEXTURE_2D,this->mipmap_level,GL_RED_INTEGER,this->image_data->get_type(),this->image_data->get_data_pointer());
          else
            glGetTexImage(GL_TEXTURE_2D,this->mipmap_level,this->image_data->get_format(),this->image_data->get_type(),this->image_data->get_data_pointer());
          
          glBindTexture(GL_TEXTURE_2D,0);
        }
        
      /**
       * Sets a given OpenGL texture integer parameter.
       */
        
      void set_parameter_int(unsigned int parameter, unsigned int value)
        {
          glBindTexture(GL_TEXTURE_2D,this->to);
          glTexParameteri(GL_TEXTURE_2D,parameter,value);
          glBindTexture(GL_TEXTURE_2D,0);
        }
      
      virtual void bind(unsigned int unit)
        {
          glActiveTexture(GL_TEXTURE0 + unit);
          glBindTexture(GL_TEXTURE_2D,this->to);
        }
      
      /**
       * Prints the texture data to stdout.
       */
      
      virtual void print()
        {
          this->image_data->print();
        }
  };
  
/**
 * Represents a frame buffer. It has a number of attachments,
 * such as color, depth, stencil etc.
 */
  
class FrameBuffer
  {
    protected:
      GLuint fbo;
      
    public:
      FrameBuffer()
        {
          glGenFramebuffers(1,&(this->fbo)); 
        }
        
      /**
       Sets the attachments for the framebuffer, i.e. the textures that will be
       rendered to. If 0 is passed for a texture, it will not be used. Targets
       are OpenGL targets (GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP_POSITIVE_X, ...).
       
       Stencil is not supported yet.
       */
        
      void set_textures(
        Texture *depth, GLuint depth_target,
        Texture *stencil, GLuint stencil_target,
        Texture *color0, GLuint color0_target,
        Texture *color1=0, GLuint color1_target=GL_TEXTURE_2D,
        Texture *color2=0, GLuint color2_target=GL_TEXTURE_2D,
        Texture *color3=0, GLuint color3_target=GL_TEXTURE_2D,
        Texture *color4=0, GLuint color4_target=GL_TEXTURE_2D,
        int mipmap_level=0)
        {
          vector<GLenum> draw_buffers;
          
          this->activate();
            
          if (color0 != 0)
            {
              glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,color0_target,color0->get_texture_object(),mipmap_level);
              draw_buffers.push_back(GL_COLOR_ATTACHMENT0);
            }
            
          if (color1 != 0)
            {
              glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,color1_target,color1->get_texture_object(),mipmap_level);
              draw_buffers.push_back(GL_COLOR_ATTACHMENT1);
            }
          
          if (color2 != 0)
            {
              glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT2,color2_target,color2->get_texture_object(),mipmap_level);
              draw_buffers.push_back(GL_COLOR_ATTACHMENT2);
            }
          
          if (color3 != 0)
            {
              glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT3,color3_target,color3->get_texture_object(),mipmap_level);
              draw_buffers.push_back(GL_COLOR_ATTACHMENT3);
            }
          
          if (color4 != 0)
            {
              glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT4,color4_target,color4->get_texture_object(),mipmap_level);
              draw_buffers.push_back(GL_COLOR_ATTACHMENT4);
            }
          
          if (depth != 0)
            {
              glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,depth_target,depth->get_texture_object(),mipmap_level);
              draw_buffers.push_back(GL_NONE);
            }
       /*        
          if (stencil != 0)
            {
              glFramebufferTexture2D(GL_FRAMEBUFFER,GL_STENCIL_ATTACHMENT,stencil_target,stencil->get_texture_object(),mipmap_level);
              draw_buffers.push_back(GL_COLOR_ATTACHMENT3);
            }
        */
      
          glDrawBuffers(draw_buffers.size(),&(draw_buffers[0]));          
          GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER); 
           
          if (status != GL_FRAMEBUFFER_COMPLETE)
            {
              string helper;
              
              switch (status)
                {
                  case GL_FRAMEBUFFER_UNDEFINED: helper = "GL_FRAMEBUFFER_UNDEFINED"; break;
                  case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: helper = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"; break;
                  case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: helper = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; break;
                  case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: helper = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"; break;
                  case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: helper = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"; break;
                  case GL_FRAMEBUFFER_UNSUPPORTED: helper = "GL_FRAMEBUFFER_UNSUPPORTED"; break;
                  case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: helper = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"; break;
                  case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: helper = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"; break;
                  default: break;
                }
              
              ErrorWriter::write_error("An error occured while binding framebuffer attachments (" + helper + ").");
            }
           
          this->deactivate();          // unbind fbo
        }
        
      void activate()
        {
          glBindFramebuffer(GL_FRAMEBUFFER,this->fbo);
        }
        
      void deactivate()
        {
          glBindFramebuffer(GL_FRAMEBUFFER,0);
        }
        
      ~FrameBuffer()
        {
          glDeleteFramebuffers(1,&(this->fbo));
        } 
  };

/**
 * Represents a cube map that is used for capturing environment.
 */
  
class ReflectionTraceCubeMap: public GPUObject
  {
    protected:
      unsigned int size;
      TextureCubeMap *texture_color;
      TextureCubeMap *texture_depth;
      TextureCubeMap *texture_distance;
      TextureCubeMap *texture_normal;
      static glm::mat4 projection_matrix;       // matrix used for cubemap texture rendering
      GLint initial_viewport[4];
      
      // uniforms associated with the cubemap
      UniformVariable *uniform_texture_color;
      UniformVariable *uniform_texture_distance;
      UniformVariable *uniform_texture_normal;
      UniformVariable *uniform_position;
      
      unsigned int texture_color_sampler;
      unsigned int texture_distance_sampler;
      unsigned int texture_normal_sampler;
      
    public:    
      TransformationTRSModel transformation;    // contains the cubemap transformation, to be able to place it in the world, only translation is considered 
      
      /**
       * Creates a new object.
       * 
       * @param size size of cubemap side in pixels
       * @param uniform_texture_color_name name of the uniform variable (sampler cube) for color
       * @param uniform_texture_distance_name name of the uniform variable (sampler cube) for distance
       * @param uniform_texture_normal_name name of the uniform variable (sampler cube) for normal
       * @param uniform_position_name name of the uniform variable (vec3) for cubemap position       
       * @param texture_color_sampler number of texture sampler to use for color texture
       * @param texture_normal_sampler number of texture sampler to use for normal texture
       * @param texture_distance_sampler number of texture sampler to use for position texture
       */
      
      ReflectionTraceCubeMap(int size, string uniform_texture_color_name, string uniform_texture_distance_name, string uniform_texture_normal_name, string uniform_position_name, unsigned int texture_color_sampler, unsigned int texture_normal_sampler, unsigned int texture_distance_sampler)
        {
          this->size = size;
          ReflectionTraceCubeMap::projection_matrix = glm::perspective((float) (M_PI / 2.0), 1.0f, 0.01f, 10000.0f);          
          
          this->texture_color = new TextureCubeMap(size,TEXEL_TYPE_COLOR);
          this->texture_distance = new TextureCubeMap(size,TEXEL_TYPE_COLOR);
          this->texture_depth = new TextureCubeMap(size,TEXEL_TYPE_DEPTH);
          this->texture_normal = new TextureCubeMap(size,TEXEL_TYPE_COLOR);
        
          glTextureParameteri(this->texture_distance->get_texture_object(),GL_TEXTURE_MAG_FILTER,GL_NEAREST);
          glTextureParameteri(this->texture_distance->get_texture_object(),GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST); 
          
          this->uniform_texture_color = new UniformVariable(uniform_texture_color_name);
          this->uniform_texture_distance = new UniformVariable(uniform_texture_distance_name);
          this->uniform_texture_normal = new UniformVariable(uniform_texture_normal_name);
          this->uniform_position = new UniformVariable(uniform_position_name);
          
          this->texture_color_sampler = texture_color_sampler;
          this->texture_distance_sampler = texture_distance_sampler;
          this->texture_normal_sampler = texture_normal_sampler;
        }
        
      /**
       * Retrieves uniform locations from given shader.
       */
        
      bool retrieve_uniform_locations(Shader *shader)
        {
          bool result = true;
          
          result = result && this->uniform_texture_color->retrieve_location(shader);
          result = result && this->uniform_texture_distance->retrieve_location(shader);
          result = result && this->uniform_texture_normal->retrieve_location(shader);
          result = result && this->uniform_position->retrieve_location(shader);
          
          return result;
        }
        
      /**
       * Updates the uniform variables (which must have been initialised with retrieve_uniform_locations()).
       */
        
      void update_uniforms()
        {
          this->uniform_texture_color->update_int((int) this->texture_color_sampler);
          this->uniform_texture_distance->update_int((int) this->texture_distance_sampler);
          this->uniform_texture_normal->update_int((int) this->texture_normal_sampler);
          this->uniform_position->update_vec3(this->transformation.get_translation());
        }
        
      /**
       * Binds the textures to samplers that were set with the constructor.
       */
        
      void bind_textures()
        {
          this->texture_color->bind(this->texture_color_sampler);
          this->texture_distance->bind(this->texture_distance_sampler);
          this->texture_normal->bind(this->texture_normal_sampler);
        }
        
      virtual ~ReflectionTraceCubeMap()
        {
          delete this->texture_color;
          delete this->texture_distance;
          delete this->texture_depth;
          delete this->texture_normal;
          
          delete this->uniform_texture_color;
          delete this->uniform_texture_distance;
          delete this->uniform_texture_normal;
          delete this->uniform_position; 
        }
      
      virtual void update_gpu()
        {
          this->get_texture_color()->update_gpu();
          this->get_texture_distance()->update_gpu();
          this->get_texture_depth()->update_gpu();
          this->get_texture_normal()->update_gpu();
        }
      
      static glm::mat4 get_projection_matrix()
        {
          return ReflectionTraceCubeMap::projection_matrix;
        }
      
      TextureCubeMap *get_texture_color()
        {
          return this->texture_color;
        }
      
      TextureCubeMap *get_texture_depth()
        {
          return this->texture_depth;
        }

      TextureCubeMap *get_texture_normal()
        {
          return this->texture_normal;
        }
        
      TextureCubeMap *get_texture_distance()
        {
          return this->texture_distance;
        }
  
      /**
       Saves the current viewport settings and sets the new one
       for cubemap rendering.
       */
  
      void set_viewport()
        {
          glGetIntegerv(GL_VIEWPORT,this->initial_viewport);   // save the old viewport
          glViewport(0,0,this->size,this->size);
        }

      /**
       Computes the acceleration texture on GPU and stores it in MIPmap
       levels of the distance texture. This will also cause distance texture
       update on GPU.
       
       NOT WORKING YET
       */
        
      void compute_acceleration_texture()
        {
          string helper_shader_fs_text =
            "#version 430\n"
            "layout(location = 0) out vec4 fragment_color;\n" 
            "uniform samplerCube cubemap;\n"
            "uniform uint sample_mip;\n"
            
            "uniform mat3 coord_coeffs;"   // coefficients for each cubemap side, rows in format: a + b * gl_FragCoord.x + c * gl_FragCoord.y
            
            "void main() {\n" 
            "  float x = coord_coeffs[0][0] + coord_coeffs[0][1] * gl_FragCoord.x + coord_coeffs[0][2] * gl_FragCoord.y;\n"
            "  float y = coord_coeffs[1][0] + coord_coeffs[1][1] * gl_FragCoord.x + coord_coeffs[1][2] * gl_FragCoord.y;\n"
            "  float z = coord_coeffs[2][0] + coord_coeffs[2][1] * gl_FragCoord.x + coord_coeffs[2][2] * gl_FragCoord.y;\n"
            
            "  vec4 texel1 = textureLod(cubemap,normalize(vec3(x,y + 0.1,z + 0.1)),sample_mip);\n" // too bad textureGather can't be used with cubemaps
            "  vec4 texel2 = textureLod(cubemap,normalize(vec3(x,y + 0.1,z - 0.1)),sample_mip);\n"
            "  vec4 texel3 = textureLod(cubemap,normalize(vec3(x,y - 0.1,z + 0.1)),sample_mip);\n"
            "  vec4 texel4 = textureLod(cubemap,normalize(vec3(x,y - 0.1,z - 0.1)),sample_mip);\n"
            "  vec4 new_texel = vec4(min(min(texel1.x,texel2.x),min(texel3.x,texel4.x)),max(max(texel1.y,texel2.y),max(texel3.y,texel4.y)),texel1.z,0);\n"
            "fragment_color = new_texel;\n"     
            "}\n";
    
          UniformVariable uniform_cubemap("cubemap");
          UniformVariable uniform_coeffs("coord_coeffs");
          UniformVariable uniform_sample_mip("sample_mip");
    
          Shader helper_shader(VERTEX_SHADER_QUAD_TEXT,preprocess_text(helper_shader_fs_text),"");
          FrameBuffer helper_frame_buffer;
          
          uniform_cubemap.retrieve_location(&helper_shader);
          uniform_coeffs.retrieve_location(&helper_shader);
          uniform_sample_mip.retrieve_location(&helper_shader);

          glGenerateTextureMipmap(this->get_texture_distance()->get_texture_object());
          
          GLuint sides[] =
            {
              GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
              GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
              GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
              GL_TEXTURE_CUBE_MAP_POSITIVE_X,
              GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
              GL_TEXTURE_CUBE_MAP_POSITIVE_Y
            };
  
          int mip_resolution = this->size;
          int mip_level = 0;  
          
          while (true) // for each mipmap level
            {
              if (mip_resolution <= 1)
                break;
                
              mip_resolution /= 2;
              mip_level++;
                
              for (int i = 0; i < 6; i++) // for each side
                {
                  glm::mat3 coeffs;

                  switch (i)
                    {   
                      case 0:
                        coeffs[0][0] = mip_resolution / 2;   coeffs[0][1] = -1;    coeffs[0][2] = 0;
                        coeffs[1][0] = mip_resolution / 2;   coeffs[1][1] = 0;     coeffs[1][2] = -1;
                        coeffs[2][0] = -mip_resolution / 2;  coeffs[2][1] = 0;     coeffs[2][2] = 0;
                        break;
                    
                      case 1:
                        coeffs[0][0] = -mip_resolution / 2;  coeffs[0][1] = 1;     coeffs[0][2] = 0;
                        coeffs[1][0] = mip_resolution / 2;   coeffs[1][1] = 0;     coeffs[1][2] = -1;
                        coeffs[2][0] = mip_resolution / 2;   coeffs[2][1] = 0;     coeffs[2][2] = 0;
                        break;
                    
                      case 2:
                        coeffs[0][0] = -mip_resolution / 2;  coeffs[0][1] = 0;     coeffs[0][2] = 0;
                        coeffs[1][0] = mip_resolution / 2;   coeffs[1][1] = 0;     coeffs[1][2] = -1;
                        coeffs[2][0] = -mip_resolution / 2;  coeffs[2][1] = 1;     coeffs[2][2] = 0;
                        break;
                  
                      case 3:
                        coeffs[0][0] = mip_resolution / 2;   coeffs[0][1] = 0;     coeffs[0][2] = 0;
                        coeffs[1][0] = mip_resolution / 2;   coeffs[1][1] = 0;     coeffs[1][2] = -1;
                        coeffs[2][0] = mip_resolution / 2;   coeffs[2][1] = -1;    coeffs[2][2] = 0;
                        break;
                  
                      case 4:
                        coeffs[0][0] = -mip_resolution / 2;  coeffs[0][1] = 1;     coeffs[0][2] = 0;
                        coeffs[1][0] = -mip_resolution / 2;  coeffs[1][1] = 0;     coeffs[1][2] = 0;
                        coeffs[2][0] = mip_resolution / 2;   coeffs[2][1] = 0;     coeffs[2][2] = -1;
                        break;
                  
                      case 5:
                        coeffs[0][0] = -mip_resolution / 2;  coeffs[0][1] = 1;     coeffs[0][2] = 0;
                        coeffs[1][0] = mip_resolution / 2;   coeffs[1][1] = 0;     coeffs[1][2] = 0;
                        coeffs[2][0] = -mip_resolution / 2;  coeffs[2][1] = 0;     coeffs[2][2] = 1;
                        break;
                    
                      default:
                        break;
                    }
              
                coeffs = transpose(coeffs);
          
                helper_frame_buffer.set_textures(
                  0,0,
                  0,0,
                  this->get_texture_distance(),sides[i],
                  0,0,
                  0,0,
                  0,0,
                  0,0,
                  mip_level
                  );
          
                helper_shader.use();
          
                this->get_texture_distance()->bind(1);
                uniform_cubemap.update_int(1);
                uniform_sample_mip.update_uint(mip_level - 1);
                uniform_coeffs.update_mat3(coeffs);
                
                helper_frame_buffer.activate();
                draw_fullscreen_quad(this->size,this->size);
                helper_frame_buffer.deactivate();
              }
            }
            
          ErrorWriter::checkGlErrors("acceleration texture GPU",true);
        }
        
      /**
       Computes the acceleration texture on CPU and stores it in MIPmap
       levels of the distance texture. This will also cause distance texture
       update on GPU.
       */
        
      void compute_acceleration_texture_sw()
        {
          unsigned int level = 1;
    
          while (true)  // for all mipmap levels
            {
              Image2D *previous_level_images[6];
        
              GLuint sides[] =
                {
                  GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
                  GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
                  GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                  GL_TEXTURE_CUBE_MAP_POSITIVE_X,
                  GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                  GL_TEXTURE_CUBE_MAP_POSITIVE_Y
                };
        
              for (int i = 0; i < 6; i++)
                previous_level_images[i] = new Image2D(this->texture_distance->get_texture_image(sides[i]));
        
              this->texture_distance->set_mipmap_level(level);
     
              for (int k = 0; k < 6; k++)
                for (int j = 0; j < this->texture_distance->image_front->get_width(); j++)
                  for (int i = 0; i < this->texture_distance->image_front->get_height(); i++)
                    {
                      int x = 2 * i;
                      int y = 2 * j;
             
                      float values_min[4];
                      float values_max[4];
                      float r,g,b,a;
              
                      previous_level_images[k]->get_pixel(x,y,         values_min,     &g,&b,&a);
                      previous_level_images[k]->get_pixel(x + 1,y,     values_min + 1, &g,&b,&a);
                      previous_level_images[k]->get_pixel(x,y + 1,     values_min + 2, &g,&b,&a);
                      previous_level_images[k]->get_pixel(x + 1,y + 1, values_min + 3, &g,&b,&a);
              
                      previous_level_images[k]->get_pixel(x,y,         &r, values_max,     &b,&a);
                      previous_level_images[k]->get_pixel(x + 1,y,     &r, values_max + 1, &b,&a);
                      previous_level_images[k]->get_pixel(x,y + 1,     &r, values_max + 2, &b,&a);
                      previous_level_images[k]->get_pixel(x + 1,y + 1, &r, values_max + 3, &b,&a);
                
                      float new_min = glm::min(glm::min(glm::min(values_min[0],values_min[1]),values_min[2]),values_min[3]);
                      float new_max = glm::max(glm::max(glm::max(values_max[0],values_max[1]),values_max[2]),values_max[3]);
              
                      this->texture_distance->get_texture_image(sides[k])->set_pixel(i,j,new_min,new_max,b,a);
                    }
        
              this->texture_distance->update_gpu();
        
              level++;
        
              for (int i = 0; i < 6; i++)
                delete previous_level_images[i];
        
              if (this->texture_distance->image_front->get_width() <= 1)
                break;
            }
      
          this->texture_distance->set_mipmap_level(0);
          this->texture_distance->load_from_gpu();
        }
        
      /**
       Restores the original viewport settings (saved when setViewport
       was called).
       */
      
      void unset_viewport()
        {
          glViewport(this->initial_viewport[0],this->initial_viewport[1],this->initial_viewport[2],this->initial_viewport[3]);
        }
  
      /**
       Gets the transformation for the camera by given cube map side
       (such as GL_TEXTURE_CUBE_MAP_POSITIVE_X, ...)
       */
  
      TransformationTRSCamera get_camera_transformation(GLuint cube_side_target)
        {
          TransformationTRSCamera result;
          
          result.set_translation(this->transformation.get_translation());
          result.set_rotation(this->transformation.get_rotation());
          
          switch (cube_side_target)
            {
              case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                result.add_rotation(glm::vec3(0.0,0.0,M_PI));
                // forward => do nothing
                break;
                
              case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
                // backward
                result.add_rotation(glm::vec3(M_PI,0.0,0.0));
                break;
                
              case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
                // left
                result.add_rotation(glm::vec3(0.0,M_PI / 2.0,M_PI));
                break;
                
              case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
                // right
                result.add_rotation(glm::vec3(0.0,-1 * M_PI / 2.0,M_PI));
                break;
                
              case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
                result.add_rotation(glm::vec3(-1 * M_PI / 2.0,0.0,0.0));
                break;
                
              case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
                result.add_rotation(glm::vec3(M_PI / 2.0,0.0,0.0));
                break;
                
              default:
                break;
            }
          
          return result;
        }
  };

glm::mat4 ReflectionTraceCubeMap::projection_matrix; 
  
/**
 * Represents a 3D geometry consisting of vertices and triangles.
 * Each vertex has a position a normal and texture coordinates
 * (each one being vec3 of float, in that order).
 */
  
class Geometry3D: public Printable, public GPUObject
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
        
      void draw_as_lines()
        {
          glBindVertexArray(this->vao);
          glDrawElements(GL_LINE_STRIP,this->triangles.size() * 3,GL_UNSIGNED_INT,0);
          glBindVertexArray(0);
        };
        
      /**
       * Flips the vertex order in all triangles.
       */
        
      void flip_triangles()
        {
          for (unsigned int i = 0; i < this->triangles.size(); i += 3)
            {
              int helper = this->triangles[i];
              this->triangles[i] = this->triangles[i + 1];
              this->triangles[i + 1] = helper;
            }
        }
        
      /**
       * Sends the geometry data to GPU.
       */
        
      virtual void update_gpu()
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
        
      void add_vertex(double x, double y, double z, double u=0, double v=0, double w=0, double normal_x=0, double normal_y=0, double normal_z=-1)
        {
          Vertex3D new_vertex;
          new_vertex.position = glm::vec3(x,y,z);
          new_vertex.texture_coord = glm::vec3(u,v,w);
          new_vertex.normal = glm::vec3(normal_x,normal_y,normal_z);
          this->vertices.push_back(new_vertex);
        }
      void add_triangle(unsigned int i1, unsigned int i2, unsigned int i3)
        {
          this->triangles.push_back(i1);
          this->triangles.push_back(i2);
          this->triangles.push_back(i3);
        }
        
      virtual void print()
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
  
Geometry3D make_quad(float width, float height, float shift_z)
  {
    Geometry3D result;
    
    float half_width = width / 2.0;
    float half_height = height / 2.0;
    
    result.add_vertex(-half_width,-half_height,shift_z,0.0,0.0,0.0,0.0,1.0,0.0); 
    result.add_vertex(half_width,-half_height,shift_z,1.0,0.0,0.0,0.0,1.0,0.0);   
    result.add_vertex(-half_width,half_height,shift_z,0.0,1.0,0.0,0.0,1.0,0.0);  
    result.add_vertex(half_width,half_height,shift_z,1.0,1.0,0.0,0.0,1.0,0.0);    
  
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
    
    result.add_vertex(-half_width,-half_height,-half_depth,0.25,0.25,0.0,-1.0,-1.0,-1.0); // 0
    result.add_vertex(half_width,-half_height,-half_depth,0.75,0.25,0.0,1.0,-1.0,-1.0);   // 1
    result.add_vertex(-half_width,half_height,-half_depth,0.25,0.75,0.0,-1.0,1.0,-1.0);   // 2
    result.add_vertex(half_width,half_height,-half_depth,0.75,0.75,0.0,1.0,1.0,-1.0);     // 3

    result.add_vertex(-half_width,-half_height,half_depth,0.0,0.0,0.0,-1.0,-1.0,1.0);     // 4
    result.add_vertex(half_width,-half_height,half_depth,1.0,0.0,0.0,1.0,-1.0,1.0);       // 5
    result.add_vertex(-half_width,half_height,half_depth,0.0,1.0,0.0,-1.0,1.0,1.0);       // 6
    result.add_vertex(half_width,half_height,half_depth,1.0,1.0,0.0,1.0,1.0,1.0);         // 7
    
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
 * Makes a box with separate sides and sharp edges.
 */
  
Geometry3D make_box_sharp(float width, float height, float depth)
  {
    Geometry3D result;
    
    float half_width = width / 2.0;
    float half_height = height / 2.0;
    float half_depth = depth / 2.0;
    
    // front:
    result.add_vertex(-half_width,-half_height,half_depth,0.501,0.665,0.0,0.0,0.0,1.0);  // 0
    result.add_vertex(half_width,-half_height,half_depth,0.999,0.665,0.0,0.0,0.0,1.0);   // 1
    result.add_vertex(-half_width,half_height,half_depth,0.501,0.334,0.0,0.0,0.0,1.0);   // 2
    result.add_vertex(half_width,half_height,half_depth,0.999,0.334,0.0,0.0,0.0,1.0);    // 3

    result.add_triangle(1,2,0);
    result.add_triangle(1,3,2); 
    
    // left vertices:
    result.add_vertex(-half_width,-half_height,-half_depth,0.001,0.665,0.0,-1.0,0.0,0.0); // 4 
    result.add_vertex(-half_width,-half_height,half_depth,0.499,0.665,0.0,-1.0,0.0,0.0);  // 5
    result.add_vertex(-half_width,half_height,-half_depth,0.001,0.334,0.0,-1.0,0.0,0.0);  // 6
    result.add_vertex(-half_width,half_height,half_depth,0.499,0.334,0.0,-1.0,0.0,0.0);   // 7
    
    result.add_triangle(5,6,4);
    result.add_triangle(5,7,6);
    
    // right vertices:
    result.add_vertex(half_width,-half_height,-half_depth,0.499,0.999,0.0,1.0,0.0,0.0);   // 8
    result.add_vertex(half_width,-half_height,half_depth,0.001,0.999,0.0,1.0,0.0,0.0);    // 9
    result.add_vertex(half_width,half_height,-half_depth,0.499,0.667,0.0,1.0,0.0,0.0);    // 10
    result.add_vertex(half_width,half_height,half_depth,0.001,0.667,0.0,1.0,0.0,0.0);     // 11
    
    result.add_triangle(9,8,10);
    result.add_triangle(9,10,11);
    
    // back vertices:
    result.add_vertex(-half_width,-half_height,-half_depth,0.499,0.332,0.0,0.0,0.0,-1.0); // 12
    result.add_vertex(half_width,-half_height,-half_depth,0.001,0.332,0.0,0.0,0.0,-1.0);  // 13
    result.add_vertex(-half_width,half_height,-half_depth,0.499,0.001,0.0,0.0,0.0,-1.0);  // 14
    result.add_vertex(half_width,half_height,-half_depth,0.001,0.001,0.0,0.0,0.0,-1.0);   // 15
    
    result.add_triangle(13,12,14);
    result.add_triangle(13,14,15);
    
    // top vertices:
    result.add_vertex(-half_width,half_height,-half_depth,0.501,0.001,0.0,0.0,1.0,0.0);   // 16
    result.add_vertex(-half_width,half_height,half_depth,0.501,0.332,0.0,0.0,1.0,0.0);    // 17
    result.add_vertex(half_width,half_height,-half_depth,0.999,0.001,0.0,0.0,1.0,0.0);    // 18
    result.add_vertex(half_width,half_height,half_depth,0.999,0.332,0.0,0.0,1.0,0.0);     // 19
    
    result.add_triangle(17,18,16);
    result.add_triangle(17,19,18);
    
    // bottom vertices:
    result.add_vertex(-half_width,-half_height,-half_depth,0.501,0.999,0.0,0.0,-1.0,0.0); // 20
    result.add_vertex(-half_width,-half_height,half_depth,0.999,0.999,0.0,0.0,-1.0,0.0);  // 21
    result.add_vertex(half_width,-half_height,-half_depth,0.501,0.667,0.0,0.0,-1.0,0.0);  // 22
    result.add_vertex(half_width,-half_height,half_depth,0.999,0.667,0.0,0.0,-1.0,0.0);   // 23
    
    result.add_triangle(21,20,22);
    result.add_triangle(21,22,23);
    
    return result;
  }
  
/**
 * Makes a matrix that represents reflection across plane given by
 * three points.
 */
  
glm::mat4 make_reflection_matrix(glm::vec3 point1, glm::vec3 point2, glm::vec3 point3)
  {
    glm::mat4 result;
    
    glm::vec3 vector1 = point2 - point1;
    glm::vec3 vector2 = point3 - point1;
    glm::vec3 normal = glm::normalize(glm::cross(vector1,vector2));          // plane normal
    
    glm::vec3 translation_vector;    // how to translate the plane to [0,0,0]
    
    float d;                         // d in plane equation (ax + by + cz + d = 0)
    
    d = -1 * normal.x * point1.x -1 * normal.y * point1.y -1 * normal.z * point1.z; 
    
    float e, f, g;                   // coefficients of parametric line equations
    
    e = -1.0 * normal.x;
    f = -1.0 * normal.y;
    g = -1.0 * normal.z;
    
    float t;                         // parameter t in parametric line equation
    
    t = -1 * d / ((float) (e * normal.x + f * normal.y + g * normal.z));
    
    translation_vector.x = t * e;
    translation_vector.y = t * f;
    translation_vector.z = t * g;
    
    glm::mat4 reflection_matrix(1.0f);
    reflection_matrix[0][0] = 1 - 2 * normal.x * normal.x;
    reflection_matrix[0][1] = -2 * normal.x * normal.y;
    reflection_matrix[0][2] = -2 * normal.x * normal.z;
    
    reflection_matrix[1][0] = -2 * normal.x * normal.y;
    reflection_matrix[1][1] = 1 - 2* normal.y * normal.y;
    reflection_matrix[1][2] = -2 * normal.y * normal.z;
    
    reflection_matrix[2][0] = -2 * normal.x * normal.z;
    reflection_matrix[2][1] = -2 * normal.y * normal.z;
    reflection_matrix[2][2] = 1 - 2 * normal.z * normal.z;   
    
    result = glm::translate(result,translation_vector);    // translate to the origin
    result = result * reflection_matrix;                           // reflect
    result = glm::translate(result,-1.0f * translation_vector);            // translate back
    
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
                if (line[0] == '/')
                  line = line.substr(1);
                
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
  
/**
 * Loads a 3D geometry from obj file format. This is a simple method and doesn't
 * support OBJ in its full specification.
 * 
 * @param filename file to be loaded
 * @param flip whether to flip object vertically (due to obj coords)
 */
  
Geometry3D load_obj(string filename, bool flip=false)
  {
    Geometry3D result;
    
    ifstream obj_file(filename.c_str());
    string line;
    float obj_line_data[4][3];
    glm::vec3 helper_point;
    
    int flip_factor = flip ? -1 : 1;

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
                  result.add_vertex(obj_line_data[0][0],obj_line_data[1][0] * flip_factor,obj_line_data[2][0],0.0,0.0,0.0,1.0,0.0,0.0);
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
                      result.vertices[indices[i]].texture_coord.y = 1.0 - texture_vertices[vt_index].y;
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
  
/**
 * Class that uses static methods and provides methods for default
 * camera control.
 */

class CameraHandler
  {
    protected:
      static bool clicked;          // whether mouse left was clicked
      static bool clicked_right;    // whether mouse right was clicked
      static int initial_mouse_coords[2];
      static glm::vec3 initial_camera_rotation;
      
    public:
      static float translation_step;
      static float rotation_step;
      static TransformationTRSCamera camera_transformation;
      
      /**
       * Set this method as mouse click callback or call it inside mouse
       * click callback in order for camera transformation of this
       * class to be handled.
       */
      
      static void mouse_click_callback(int button, int state, int x, int y)
        {
          if (button == GLUT_LEFT_BUTTON)
            CameraHandler::clicked = !CameraHandler::clicked;
          else if (button == GLUT_RIGHT_BUTTON)
            CameraHandler::clicked_right = !CameraHandler::clicked_right;
          
          if (CameraHandler::clicked)
            {
              CameraHandler::initial_mouse_coords[0] = x;
              CameraHandler::initial_mouse_coords[1] = y;
              CameraHandler::initial_camera_rotation = CameraHandler::camera_transformation.get_rotation();
            }
        }
      
      /**
       * Set this method as mouse move callback or call it inside mouse
       * move callback in order for camera transformation of this
       * class to be handled.
       */
      
      static void mouse_move_callback(int x, int y)
        {
          int dx, dy;
   
          dx = x - initial_mouse_coords[0];
          dy = y - initial_mouse_coords[1];
          
          if (CameraHandler::clicked)
            camera_transformation.set_rotation(
              glm::vec3(CameraHandler::initial_camera_rotation.x - dy / (800.0) * CameraHandler::rotation_step,
                        CameraHandler::initial_camera_rotation.y - dx / (600.0) * CameraHandler::rotation_step,
                        0.0));
        }
        
      /**
       * Set this method as mouse key callback or call it inside mouse
       * key callback in order for camera transformation of this
       * class to be handled.
       */
        
      static void key_callback(unsigned char key, int x, int y)
        {
          glm::vec3 helper;
          float speed_factor = CameraHandler::clicked_right ? 3.0 : 1.0;
    
          switch (key)
            {
              case 'd':
                helper = CameraHandler::camera_transformation.get_direction_left();
                helper *= -1 * speed_factor * CameraHandler::translation_step;
                CameraHandler::camera_transformation.add_translation(helper);
                break;
          
              case 'a':
                helper = CameraHandler::camera_transformation.get_direction_left();
                helper *= speed_factor * CameraHandler::translation_step;
                CameraHandler::camera_transformation.add_translation(helper);
                break;
              
              case 'w':
                helper = CameraHandler::camera_transformation.get_direction_forward();
                helper *= speed_factor * CameraHandler::translation_step;
                CameraHandler::camera_transformation.add_translation(helper);
                break;
                
              case 's':
                helper = CameraHandler::camera_transformation.get_direction_forward();
                helper *= -1 * speed_factor * CameraHandler::translation_step;
                CameraHandler::camera_transformation.add_translation(helper);
                break;
              
              case 'q':
                helper = CameraHandler::camera_transformation.get_direction_up();
                helper *= speed_factor * CameraHandler::translation_step;
                CameraHandler::camera_transformation.add_translation(helper);
                break;
                
              case 'e':
                helper = CameraHandler::camera_transformation.get_direction_up();
                helper *= -1 * speed_factor * CameraHandler::translation_step;
                CameraHandler::camera_transformation.add_translation(helper);
                break;
                
              default:
                break;
            }
        }
  };
  
/**
 * Serves for gathering statistics about rendering and performance,
 * can serve optimizations. The object holds a set of named double values
 * that are being recorded each frame (with possible skip) and their
 * average values can be retrieved or printed. This class also provides
 * methods for measuring metrics such as time or rasterised fragments
 * (using OpenGL queries).
 * 
 * TODO: fps mesurement is inaccurate, only whole seconds are measured => fix this
 */
  
class Profiler: public Printable
  {
    protected:
      int skip_frames;    ///< how many frames should be skipped before recording values
      int frames_to_be_skipped;
      
      int frames_recorded_total;
      
      vector<double> cumulative_values;
      vector<string> value_names;         ///< corresponding names to cumulative_values

      GLuint time_query_id;
      GLuint fragment_count_query_id;
      
      int frame_count;
      double time_start;     // for fps measurement
      double fps;
      
    public:
      Profiler()
        {
          if (!GLSession::is_initialised())
            ErrorWriter::write_error("Initialising profiler object before GLSession was initialised.");
            
          glGenQueries(1,&this->time_query_id);
          glGenQueries(1,&this->fragment_count_query_id);
          this->skip_frames = 32;
          this->frames_to_be_skipped = 0;
          this->frames_recorded_total = 0;
          
          this->frame_count = -1;
          this->time_start = -1.0;
          this->fps = 0;
          
          this->reset();
        }
        
      virtual ~Profiler()
        {
        }
        
      /**
       * Starts measuting time of OpenGL commands.
       */
        
      void time_measure_begin()
        {
          glBeginQuery(GL_TIME_ELAPSED,this->time_query_id);
        }
        
      void fragment_count_measure_begin()
        {
          glBeginQuery(GL_SAMPLES_PASSED,this->fragment_count_query_id);
        }
        
      unsigned int fragment_count_measure_end()
        {
          GLuint fragment_count;
          glEndQuery(GL_SAMPLES_PASSED);
          glGetQueryObjectuiv(this->fragment_count_query_id,GL_QUERY_RESULT,&fragment_count);
          return fragment_count;
        }
        
      /**
       * Stops measuring time and returns the time measured from
       * time_measure_start(...) call.
       * 
       * @return number of miliseconds
       */
        
      double time_measure_end()
        {
          GLuint time_passed;
          glEndQuery(GL_TIME_ELAPSED);
          glGetQueryObjectuiv(this->time_query_id,GL_QUERY_RESULT,&time_passed);
          return (time_passed / 1000000.0);
        }
      
      /**
       * Creates a new value to be recorded. Values must be created before
       * any recording happens.
       */
        
      void new_value(string value_name)
        {
          this->cumulative_values.push_back(0.0);
          this->value_names.push_back(value_name);
        }
        
      /**
       * Records given value, this should be called every rendering frame for
       * each recorded value (for frame skipping call set_fram_skip(...) method).
       * 
       * @param index value index
       * @param value value to be recorded
       */
        
      void record_value(unsigned int index, double value)
        {
          if (this->frames_to_be_skipped == 0)
            this->cumulative_values[index] += value;
        }
        
      /**
       * This must be called at the end of each rendering frame (for frame skipping
       * call set_fram_skip(...) method).
       */ 
       
      void next_frame()
        {
          time_t time_sec;
          time(&time_sec);
            
          if (this->frame_count >= 0 )
            {
              if (this->time_start < 0)
                this->time_start = time_sec;
            }
              
          this->frame_count++;

          if (this->frames_to_be_skipped <= 0)
            {
              this->frames_recorded_total++;
              this->frames_to_be_skipped = this->skip_frames;

              double elapsed_time = clock() - this->time_start;
              elapsed_time = time_sec - this->time_start;
              
              this->fps = this->frame_count / elapsed_time; 
              this->frame_count = 0;
              this->time_start = -1.0;
            }
          else
            {
              this->frames_to_be_skipped--;
            }
        }
      
      /**
       * Resets all values so the recording will start from scratch.
       */
        
      void reset()
        {
          unsigned int i;
          
          for (i = 0; i < this->cumulative_values.size(); i++)
            this->cumulative_values[i] = 0.0;
          
           this->frames_recorded_total = 0;
        }
        
      /**
       * Sets frame skip value. Value recording will only happen after the
       * set value of frames.
       */ 
       
      void set_frame_skip(unsigned int number_of_frames)
        {
          this->skip_frames = number_of_frames;
        }
        
      double get_average_value(unsigned int index)
        {
          return this->cumulative_values[index] / this->frames_recorded_total;
        }
        
      virtual void print()
        {
          unsigned int i;
          
          cout << "profiling info:" << endl;
          cout << "  fps: " << this->fps << endl;
          cout << "  total frames recorded (" << this->skip_frames << " frame skip): " << this->frames_recorded_total << endl;
          cout << "  average values recorded:" << endl;
          
          for (i = 0; i < this->cumulative_values.size(); i++)
            cout << "    " << this->value_names[i] << ": " << this->get_average_value(i) << endl;
        };
  };
  
class StorageBuffer: public GPUObject, public Printable
  {
    // WORK IN PROGRESS
      
    protected:
      GLuint ssbo;
      GLuint binding_point;
      unsigned int size_bytes;
      char *data;
      
    public:
      StorageBuffer(unsigned int size_bytes, unsigned int binding_point)
        {
          glGenBuffers(1,&(this->ssbo));
          glBindBuffer(GL_SHADER_STORAGE_BUFFER,this->ssbo);
          
          this->size_bytes = size_bytes;
          
          this->binding_point = binding_point;
          
          this->data = (char *) malloc(size_bytes);
          memset(this->data,0,size_bytes);
          
          glBindBufferBase(GL_SHADER_STORAGE_BUFFER,this->binding_point,this->ssbo);
          glBufferData(GL_SHADER_STORAGE_BUFFER,size_bytes,this->data,GL_STATIC_DRAW);
        }

      void bind()
        {
          glBindBufferBase(GL_SHADER_STORAGE_BUFFER,this->binding_point,this->ssbo);
          glFinish();
        }
        
      virtual ~StorageBuffer()
        {
          free(this->data);
        }
        
      virtual void update_gpu()
        {
          glBindBufferBase(GL_SHADER_STORAGE_BUFFER,this->binding_point,this->ssbo);
          glBufferData(GL_SHADER_STORAGE_BUFFER,this->size_bytes,this->data,GL_STATIC_DRAW);
        }
      
      virtual void load_from_gpu()
        {
          glBindBufferBase(GL_SHADER_STORAGE_BUFFER,this->binding_point,this->ssbo);
          glGetBufferSubData(GL_SHADER_STORAGE_BUFFER,0,this->size_bytes,this->data);
        }
        
      virtual void print()
        {
          for (unsigned int i = 0; i < this->size_bytes; i++)
            cout << ((int) this->data[i]) << " ";
            
          cout << endl;
        }
  };
  
/**
 * Allows the shaders to write debugging info into a log, using SSBOs. Only one log per program is now supported.
 */
  
#define SHADER_LOG_LINE_LENGTH 512
#define SHADER_LOG_MAX_LINES 512
#define SHADER_LOG_DATA_OFFSET sizeof(uint32_t) * 3
#define SHADER_LOG_DATA_SIZE SHADER_LOG_LINE_LENGTH * SHADER_LOG_MAX_LINES * sizeof(uint32_t)
#define SHADER_LOG_TOTAL_SIZE SHADER_LOG_DATA_OFFSET + SHADER_LOG_DATA_SIZE

class ShaderLog: public GPUObject, public Printable
  {
    protected:
      GLuint ssbo;
      GLuint binding_point;
      
      uint32_t number_of_lines;              // order of these members has to be kept!
      uint32_t max_lines;
      uint32_t line_length;
      uint32_t data[SHADER_LOG_DATA_SIZE];
      
      unsigned int print_limit;
      
    public:
      void set_number_of_lines(unsigned int lines)
        {
          this->number_of_lines = lines;
        }
        
      unsigned int get_number_of_lines(unsigned int lines)
        {
          return this->number_of_lines;
        }
        
      ShaderLog()
        {
          if (!GLSession::is_initialised())
            ErrorWriter::write_error("ShaderLog object created before GLSession was initialised.");
          
          this->number_of_lines = 0;
          this->max_lines = SHADER_LOG_MAX_LINES;
          this->line_length = SHADER_LOG_LINE_LENGTH;
          this->binding_point = 0;

          this->print_limit = -1;
          
          glGenBuffers(1,&(this->ssbo));
          glBindBuffer(GL_SHADER_STORAGE_BUFFER,this->ssbo);
          
          // note: &(this->number_of_lines) on the next line is correct, it's the start of bigger memory block
          glBufferData(GL_SHADER_STORAGE_BUFFER,SHADER_LOG_TOTAL_SIZE,&(this->number_of_lines),GL_STATIC_DRAW);
        };
        
      void clear()
        {
          this->number_of_lines = 0;
        }

      int get_number_of_lines()
        {
          return this->number_of_lines;
        }
        
      void bind()
        {
          glBindBufferBase(GL_SHADER_STORAGE_BUFFER,this->binding_point,this->ssbo);
          glFinish();
        }
        
      void set_print_limit(unsigned int print_limit)
        {
          this->print_limit = print_limit;
        }
        
      virtual void update_gpu()
        {
          glBindBufferBase(GL_SHADER_STORAGE_BUFFER,this->binding_point,this->ssbo);
          glBufferData(GL_SHADER_STORAGE_BUFFER,SHADER_LOG_TOTAL_SIZE,&(this->number_of_lines),GL_STATIC_DRAW);
        }
      
      virtual void load_from_gpu()
        {
          glBindBufferBase(GL_SHADER_STORAGE_BUFFER,this->binding_point,this->ssbo);
          glGetBufferSubData(GL_SHADER_STORAGE_BUFFER,0,SHADER_LOG_TOTAL_SIZE,&(this->number_of_lines));
        }
        
      virtual void print()
        {
          bool raw_values = false;   // for debuggin set this to true
          
          cout << "------------ shader log ------------" << endl;
          
          
          for (uint i = 0; i < this->number_of_lines; i++)
            {
              if (i >= this->max_lines || (this->print_limit >= 0 and i >= this->print_limit))
                break;

              uint line_position = 0;
              uint32_t *line_data = this->data + i * SHADER_LOG_LINE_LENGTH; 
              
              cout << "line " << i << ": ";
              
              while (line_position < this->line_length)
                {
                  if (raw_values)
                    {
                      cout << ((uint) *(line_data + line_position));
                      line_position++;
                    }
                  else
                    { 
                      // interpret data types here
                    
                      uint data_type_number = (uint) *(line_data + line_position);
                      line_position++;  
                  
                      switch (data_type_number)
                        {
                          case 1:    // uint
                            if (line_position < this->line_length)
                              {    
                                uint data = (uint) *(line_data + line_position);
                                cout << data;
                              }
                        
                            line_position += 1;
                            break;
                        
                          case 2:    // int
                            if (line_position < this->line_length)
                              {    
                                int data = (int) *(line_data + line_position);
                                cout << data;
                              }
                        
                            line_position += 1;
                            break;
                        
                          case 3:    // float
                            if (line_position < this->line_length)
                              {    
                                float data = glm::uintBitsToFloat( *(line_data + line_position));
                                cout << data;
                              }
                        
                            line_position += 1;
                            break;
                            
                          case 4:    // char
                            if (line_position < this->line_length)
                              {    
                                char data = (char) *(line_data + line_position);
                                
                                if (data == 10)
                                  cout << endl;
                                else
                                  cout << data;
                              }
                        
                            line_position += 1;
                            break;
                            
                          case 5:    // newline
                            cout << endl;
                            line_position += 1;
                            break;
                            
                          default:
                            line_position = this->line_length + 1; // end
                            break;
                        }
                    }
                    
                  cout << " ";
                }

              cout << endl;
            }
            
          cout << "------------ end of log ------------" << endl;
        };
        
      virtual ~ShaderLog()
        {
        }
        
      GLuint get_ssbo()
        {
          return this->ssbo;
        }
  };
  
bool CameraHandler::clicked = false;
bool CameraHandler::clicked_right = false;
float CameraHandler::translation_step = 0.5;
float CameraHandler::rotation_step = 1.0;
int CameraHandler::initial_mouse_coords[2] = {0,0};
glm::vec3 CameraHandler::initial_camera_rotation;
TransformationTRSCamera CameraHandler::camera_transformation;

Geometry3D *geometry_fullscreen_quad = 0;

void draw_fullscreen_quad(int viewport_width=-1, int viewport_height=-1)
  {
    static Geometry3D g;
    GLint old_viewport[4];
      
    if (geometry_fullscreen_quad == 0)
      {
        g = make_quad(2,2,0.5);
        g.flip_triangles();
        geometry_fullscreen_quad = &g;
        geometry_fullscreen_quad->update_gpu();
      }

    if (viewport_width >= 0 && viewport_height >= 0)
      {
        glGetIntegerv(GL_VIEWPORT,old_viewport);   // save the old viewport
        glViewport(0,0,viewport_width,viewport_height);
      }
    
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    
    geometry_fullscreen_quad->draw_as_triangles();
    
    glEnable(GL_DEPTH_TEST);
    
    if (viewport_width >= 0 && viewport_height >= 0)
      {
        glViewport(old_viewport[0],old_viewport[1],old_viewport[2],old_viewport[3]);
      }
  }

#endif
