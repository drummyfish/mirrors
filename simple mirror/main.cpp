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
    frame_buffer->activate();
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    texture->bind(0);
    
    glUniformMatrix4fv(view_matrix_location,1,GL_TRUE, glm::value_ptr(CameraHandler::camera_transformation.get_matrix()));
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
    
    glUniformMatrix4fv(view_matrix_location,1,GL_TRUE, glm::value_ptr(CameraHandler::camera_transformation.get_matrix()));
    glUniformMatrix4fv(model_matrix_location,1,GL_TRUE, glm::value_ptr(transformation_cup.get_matrix()));
    geometry_cup->draw_as_triangles();
    
    texture_mirror->bind(0);
    glUniformMatrix4fv(model_matrix_location,1,GL_TRUE, glm::value_ptr(transformation_mirror.get_matrix()));
    geometry_mirror->draw_as_triangles();
    
    glutSwapBuffers();
  }
  
int main(int argc, char** argv)
  {
    GLSession *session;
    session = GLSession::get_instance();
    session->keyboard_callback = CameraHandler::key_callback;
    session->mouse_callback = CameraHandler::mouse_click_callback;
    session->mouse_pressed_motion_callback = CameraHandler::mouse_move_callback;
    session->mouse_not_pressed_motion_callback = CameraHandler::mouse_move_callback;
    session->window_size[0] = WINDOW_WIDTH;
    session->window_size[1] = WINDOW_HEIGHT;
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
    delete texture_mirror_depth;
    GLSession::clear();
    return 0;
  }