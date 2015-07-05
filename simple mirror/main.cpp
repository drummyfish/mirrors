#include "../gl_wrapper.h"

#define CAMERA_STEP 2.0
#define ROTATION_STEP 0.1

TransformationTRSModel transformation_cup;
TransformationTRSModel transformation_mirror;
TransformationTRSCamera transformation_camera;

GLint color_location;
GLint light_direction_location;
GLint sampler_location;
GLint view_matrix_location;
GLint model_matrix_location;
GLint projection_matrix_location;
TransformFeedbackBuffer *tfb;
Texture2D *tex;

Geometry3D *geometry_cup;
Geometry3D *geometry_mirror;
Texture2D *texture;
Texture2D *texture_mirror;
FrameBuffer *frame_buffer;

void render()
  {
    
    frame_buffer->activate();
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    texture->bind(0);
    
    glUniformMatrix4fv(view_matrix_location,1,GL_TRUE, glm::value_ptr(transformation_camera.get_matrix()));
    glUniformMatrix4fv(model_matrix_location,1,GL_TRUE, glm::value_ptr(transformation_cup.get_matrix()));
    geometry_cup->draw_as_triangles();
    
    texture_mirror->bind(0);
    glUniformMatrix4fv(model_matrix_location,1,GL_TRUE, glm::value_ptr(transformation_mirror.get_matrix()));
    geometry_mirror->draw_as_triangles();
    frame_buffer->deactivate();
    
    //-------
    
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    texture->bind(0);
    
    glUniformMatrix4fv(view_matrix_location,1,GL_TRUE, glm::value_ptr(transformation_camera.get_matrix()));
    glUniformMatrix4fv(model_matrix_location,1,GL_TRUE, glm::value_ptr(transformation_cup.get_matrix()));
    geometry_cup->draw_as_triangles();
    
    texture_mirror->bind(0);
    glUniformMatrix4fv(model_matrix_location,1,GL_TRUE, glm::value_ptr(transformation_mirror.get_matrix()));
    geometry_mirror->draw_as_triangles();
    
    glutSwapBuffers();
  }
  
void special_callback(int key, int x, int y)
  {
    switch (key)
      {
        case GLUT_KEY_UP:
          transformation_camera.add_translation(glm::vec3(0,CAMERA_STEP,0));
          break;
        
        case GLUT_KEY_DOWN:
          transformation_camera.add_translation(glm::vec3(0,-CAMERA_STEP,0));
          break;
        
        case GLUT_KEY_RIGHT:
          transformation_camera.add_translation(glm::vec3(CAMERA_STEP,0,0));
          break;
        
        case GLUT_KEY_LEFT:
          transformation_camera.add_translation(glm::vec3(-CAMERA_STEP,0,0));
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
          transformation_camera.add_rotation(glm::vec3(0,0,ROTATION_STEP));
          break;
          
        case 'a':
          transformation_camera.add_rotation(glm::vec3(0,0,-ROTATION_STEP));
          break;
        
        case 'w':
          transformation_camera.add_rotation(glm::vec3(ROTATION_STEP,0,0));
          break;
          
        case 's':
          transformation_camera.add_rotation(glm::vec3(-ROTATION_STEP,0,0));
          break;
        
        case 'q':
          transformation_camera.add_rotation(glm::vec3(0,ROTATION_STEP,0));
          break;
          
        case 'e':
          transformation_camera.add_rotation(glm::vec3(0,-ROTATION_STEP,0));
          break;
          
        case 'r':
          transformation_camera.add_translation(glm::vec3(0.0,0.0,CAMERA_STEP));
          break;
          
        case 'f':
          transformation_camera.add_translation(glm::vec3(0.0,0.0,-CAMERA_STEP));
          break;
          
        default:
          break;
      }
  }
  
int main(int argc, char** argv)
  {
    GLSession *session;
    session = GLSession::get_instance();
    session->special_callback = special_callback;
    session->keyboard_callback = key_callback;
    session->init(render);
    
    Geometry3D g = load_obj("cup.obj");
    geometry_cup = &g;
    geometry_cup->update_gpu();
    transformation_cup.set_translation(glm::vec3(0.0,0.0,-10.0));
    transformation_cup.set_rotation(glm::vec3(3.1415,0.0,0));
    
    Geometry3D g2 = make_quad(10,10);
    geometry_mirror = &g2;
    geometry_mirror->update_gpu();
    transformation_mirror.set_translation(glm::vec3(0.0,0.0,-20.0));
    transformation_mirror.set_rotation(glm::vec3(3.1415 / 2.0,0.0,0));
    
    texture = new Texture2D(8,8);
    texture->load_ppm("texture.ppm");
    texture->update_gpu();
    
    texture_mirror = new Texture2D(512,512);
    texture_mirror->set_parameter_int(GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    texture_mirror->set_parameter_int(GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    texture_mirror->update_gpu();
    
    frame_buffer = new FrameBuffer();
    frame_buffer->set_textures(0,0,texture_mirror,0,0);
    
    cout << "GL version: '" << glGetString(GL_VERSION) << "'" << endl;
    
    Shader shader(file_text("shader.vs"),file_text("shader.fs"));
    
    light_direction_location = shader.get_uniform_location("light_direction");
    sampler_location = shader.get_uniform_location("tex");
    glUniform1i(sampler_location,0);
    model_matrix_location = shader.get_uniform_location("model_matrix");
    view_matrix_location = shader.get_uniform_location("view_matrix");
    projection_matrix_location = shader.get_uniform_location("projection_matrix");
    glm::mat4 projection_matrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.01f, 100.0f);
    glm::mat4 view_matrix = glm::mat4(1.0f);
    
    shader.use();
    
    glUniformMatrix4fv(projection_matrix_location,1,GL_TRUE, glm::value_ptr(projection_matrix));
    glUniformMatrix4fv(view_matrix_location,1,GL_TRUE, glm::value_ptr(view_matrix));
    glUniform3f(light_direction_location,0.0,0.0,-1.0);
    
    session->start();
    
    delete texture;
    delete texture_mirror;
    delete frame_buffer;
    GLSession::clear();
    return 0;
  }