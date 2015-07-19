#include "../gl_wrapper.h"

#define CAMERA_STEP 0.1
#define ROTATION_STEP 0.1
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

TransformationTRSModel transformation_cup;
TransformationTRSModel transformation_mirror;
TransformationTRSCamera transformation_camera;

GLint color_location;
GLint light_direction_location;
GLint sampler_location;
GLint view_matrix_location;
GLint mirror_location;
GLint model_matrix_location;
GLint projection_matrix_location;
TransformFeedbackBuffer *tfb;
Texture2D *tex;

Geometry3D *geometry_cup;
Geometry3D *geometry_mirror;
Texture2D *texture;
Texture2D *texture_mirror;
Texture2D *texture_mirror_depth;
FrameBuffer *frame_buffer;

bool clicked = false;     // whether mouse was clicked
int initial_mouse_coords[2];
glm::vec3 initial_camera_rotation;

void render()
  {
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    
    // first pass:
    
    texture->bind(0);
    
    glUniformMatrix4fv(view_matrix_location,1,GL_TRUE,glm::value_ptr(CameraHandler::camera_transformation.get_matrix()));
    
    // draw the cup:
    
    glStencilFunc(GL_ALWAYS,1,0xFF);           // always pass the stencil test
    glUniformMatrix4fv(model_matrix_location,1,GL_TRUE,glm::value_ptr(transformation_cup.get_matrix()));
    geometry_cup->draw_as_triangles();

    glUniformMatrix4fv(model_matrix_location,1,GL_TRUE, glm::value_ptr(transformation_mirror.get_matrix()));
   
    glDepthMask(GL_FALSE);                     // disable writing to dept buffer so we can later draw over the mirror
    glStencilFunc(GL_ALWAYS,1,0xFF);           // for each mirror fragment write 1 to stencil buffer
    glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);   // write 1 only for pixels that get rasterized
    glStencilMask(0xFF);                       // needen for glClear(GL_STENCIL_BUFFER_BIT)
    glClear(GL_STENCIL_BUFFER_BIT);            // clear the stencil buffer to zeros
    
    // draw the mirror:
    glUniform1ui(mirror_location,1);
    geometry_mirror->draw_as_triangles();
    glUniform1ui(mirror_location,0);
    
    // second pass:
    
    glm::vec4 a,b,c;                           // 3 mirror vertices for the plane of reflection
    a = transformation_mirror.get_matrix() * glm::vec4(geometry_mirror->vertices[0].position,1.0);
    b = transformation_mirror.get_matrix() * glm::vec4(geometry_mirror->vertices[1].position,1.0);
    c = transformation_mirror.get_matrix() * glm::vec4(geometry_mirror->vertices[2].position,1.0);

    glUniformMatrix4fv(model_matrix_location,1,GL_TRUE,    // cup transformation + reflection transformation
      glm::value_ptr(
        make_reflection_matrix(glm::vec3(a),glm::vec3(b),glm::vec3(c)) *
        transformation_cup.get_matrix()
        ));    
    
    glDepthMask(GL_TRUE);                      // re-enable dept buffer writing

    glStencilMask(0x00);                       // disable writing to stencil buffer
    glStencilFunc(GL_EQUAL,1,0xFF);            // only draw on pixels where stencil = 1
    
    geometry_cup->draw_as_triangles();         // draw the mirrored cup over the mirror

    glDisable(GL_STENCIL_TEST);
    
    glutSwapBuffers();
  }
  
void special_callback(int key, int x, int y)
  {
    switch(key)
      {
        case GLUT_KEY_DOWN:
          transformation_mirror.add_translation(glm::vec3(0.0,0.0,0.5));
          break;
          
        case GLUT_KEY_UP:
          transformation_mirror.add_translation(glm::vec3(0.0,0.0,-0.5));
          break;
        
        case GLUT_KEY_LEFT:
          transformation_mirror.add_translation(glm::vec3(-0.5,0.0,0.0));
          break;
          
        case GLUT_KEY_RIGHT:
          transformation_mirror.add_translation(glm::vec3(0.5,0.0,0.0));
          break;
          
        case GLUT_KEY_PAGE_UP:
          transformation_mirror.add_rotation(glm::vec3(0.1,0.0,0.0));
          break;
          
        case GLUT_KEY_PAGE_DOWN:
          transformation_mirror.add_rotation(glm::vec3(-0.1,0.0,0.0));
          break;
          
        case GLUT_KEY_HOME:
          transformation_mirror.add_rotation(glm::vec3(0.0,-0.1,0.0));
          break;
          
        case GLUT_KEY_END:
          transformation_mirror.add_rotation(glm::vec3(0.0,0.1,0.0));
          break;
          
        default:
          break;
      }
  }
  
int main(int argc, char** argv)
  {
    GLSession *session;
    session = GLSession::get_instance();
    session->keyboard_callback = CameraHandler::key_callback;
    session->mouse_callback = CameraHandler::mouse_click_callback;
    session->mouse_pressed_motion_callback = CameraHandler::mouse_move_callback;
    session->mouse_not_pressed_motion_callback = CameraHandler::mouse_move_callback;
    session->special_callback = special_callback;
    session->window_size[0] = WINDOW_WIDTH;
    session->window_size[1] = WINDOW_HEIGHT;
    session->init(render);
    
    CameraHandler::camera_transformation.set_translation(glm::vec3(5.5,2.0,8.0));
    CameraHandler::camera_transformation.set_rotation(glm::vec3(-0.05,0.1,0.0));
    
    glDisable(GL_CULL_FACE);    // the mirror will reverse the vertex order :/
    
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
    
    texture = new Texture2D(8,8,TEXEL_TYPE_COLOR);
    texture->load_ppm("texture.ppm");
    texture->update_gpu();
    
    texture_mirror = new Texture2D(512,512,TEXEL_TYPE_COLOR);
    texture_mirror->set_parameter_int(GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    texture_mirror->set_parameter_int(GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    texture_mirror->update_gpu();
    
    texture_mirror_depth = new Texture2D(512,512,TEXEL_TYPE_DEPTH);
    texture_mirror_depth->set_parameter_int(GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    texture_mirror_depth->set_parameter_int(GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    texture_mirror_depth->update_gpu();
    
    frame_buffer = new FrameBuffer();
    frame_buffer->set_textures(texture_mirror_depth,0,texture_mirror,0,0);
    
    cout << "GL version: '" << glGetString(GL_VERSION) << "'" << endl;
    
    Shader shader(file_text("shader.vs"),file_text("shader.fs"));
    
    light_direction_location = shader.get_uniform_location("light_direction");
    mirror_location = shader.get_uniform_location("mirror");
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
    glUniform1ui(mirror_location,0);
    
    session->start();
    
    delete texture;
    delete texture_mirror;
    delete frame_buffer;
    delete texture_mirror_depth;
    GLSession::clear();
    return 0;
  }