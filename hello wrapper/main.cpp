/*
  Simple application showing the basics of gl_wrapper.

  A simple model is created and displayed with texture, camera can
  be controlled with WSAD and mouse.

  Miloslav Číž, 2016
*/

#include "../gl_wrapper.h"

TransformationTRSModel transformation_model;
TransformationTRSCamera transformation_camera;

// uniform variable pointers:

UniformVariable *uniform_light_direction;         
UniformVariable *uniform_sampler;
UniformVariable *uniform_view_matrix;
UniformVariable *uniform_model_matrix;
UniformVariable *uniform_projection_matrix;

Geometry3D *geometry_model;
Texture2D *texture;

bool clicked = false;               // whether mouse was clicked
int initial_mouse_coords[2];

void render()    // render callback
  {
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
  
    texture->bind(0);
    uniform_view_matrix->update_mat4(CameraHandler::camera_transformation.get_matrix());

    uniform_model_matrix->update_mat4(transformation_model.get_matrix());    
    geometry_model->draw_as_triangles();     // draw the model
    
    glutSwapBuffers();
  }
  
int main(int argc, char** argv)
  {
    GLSession *session;
    session = GLSession::get_instance();     // creates the session

    // add default callbacks for camera movement:
    session->keyboard_callback = CameraHandler::key_callback;                         // this is a premade callback that moves the camera with WSAD keys
    session->mouse_callback = CameraHandler::mouse_click_callback;                    // this is a premade callback that rotates the camera with left mouse button press
    session->mouse_pressed_motion_callback = CameraHandler::mouse_move_callback;      // another premade callback
    session->mouse_not_pressed_motion_callback = CameraHandler::mouse_move_callback;  // another premade callback

    session->window_size[0] = 800;
    session->window_size[1] = 600;
    session->init(render);
 
    CameraHandler::camera_transformation.set_translation(glm::vec3(5.5,2.0,8.0));
    CameraHandler::camera_transformation.set_rotation(glm::vec3(-0.05,0.1,0.0));

    Geometry3D g = make_box_sharp(2,2,2);            // creates a box with sharp edges
    geometry_model = &g;
    geometry_model->update_gpu();                    // uploads the geometry data to GPU
    transformation_model.set_translation(glm::vec3(0.0,0.0,-10.0));
    transformation_model.set_rotation(glm::vec3(0.5,1.4,0.0));
    transformation_model.set_scale(glm::vec3(3.0,1.0,2.0));
    
    texture = new Texture2D(8,8,TEXEL_TYPE_COLOR);
    texture->load_ppm("../resources/texture.ppm");
    texture->update_gpu();                          // uploads the texture to GPU
 
    cout << "GL version: '" << glGetString(GL_VERSION) << "'" << endl;
  
    Shader shader(file_text("shader.vs"),file_text("shader.fs"),"");

    // create the uniform variables:
    uniform_light_direction = new UniformVariable("light_direction");
    uniform_sampler = new UniformVariable("tex");
    uniform_view_matrix = new UniformVariable("view_matrix");
    uniform_model_matrix = new UniformVariable("model_matrix");
    uniform_projection_matrix = new UniformVariable("projection_matrix");

    // let each variable retrieve its location
    uniform_light_direction->retrieve_location(&shader);    
    uniform_sampler->retrieve_location(&shader);
    uniform_view_matrix->retrieve_location(&shader);
    uniform_model_matrix->retrieve_location(&shader);
    uniform_projection_matrix->retrieve_location(&shader);

    glm::mat4 projection_matrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.01f, 100.0f);
    glm::mat4 view_matrix = glm::mat4(1.0f);
 
    shader.use();   // sets the shader to be used

    uniform_projection_matrix->update_mat4(projection_matrix);
    uniform_view_matrix->update_mat4(view_matrix);

    uniform_sampler->update_int(0);
    uniform_light_direction->update_float_3(0.0,0.0,-1.0);
 
    ErrorWriter::checkGlErrors("after init",true);       // checks for any OpenGL errors

    session->start();        // start the render loop
    
    delete texture;
    delete uniform_light_direction;
    delete uniform_sampler;
    delete uniform_view_matrix;
    delete uniform_model_matrix;
    delete uniform_projection_matrix;

    GLSession::clear();                  // should always be called
    return 0;
  }
