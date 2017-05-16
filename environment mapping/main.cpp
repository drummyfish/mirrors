/*
  Demonstration of environment mapping.

  Miloslav Číž, 2016
*/

#include "../gl_wrapper.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define CUBEMAP_RESOLUTION 512

TransformationTRSModel transformation_teapot;
TransformationTRSModel transformation_scene;
TransformationTRSModel transformation_sky;
TransformationTRSCamera transformation_camera;

glm::mat4 projection_matrix;
glm::mat4 view_matrix;

UniformVariable *uniform_camera_position;
UniformVariable *uniform_sampler;
UniformVariable *uniform_view_matrix;
UniformVariable *uniform_mirror;              // whether mirror is being drawn
UniformVariable *uniform_sky;                 // whether sky is being drawn
UniformVariable *uniform_model_matrix;
UniformVariable *uniform_projection_matrix;

Geometry3D *geometry_teapot;                  // teapot will be used as a mirror
Geometry3D *geometry_mirror;
Geometry3D *geometry_scene;
Geometry3D *geometry_sky;

Texture2D *texture;

Texture2D *texture_sky;
Texture2D *texture_scene;

FrameBuffer *frame_buffer_cube;               // allows rendering to cubemap
ReflectionTraceCubeMap *cubemap;              // stores and manages the cubemap texture

void draw_scene()       // draws the scene without the mirror
  {
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // draw the scene:
    uniform_model_matrix->update_mat4(transformation_scene.get_matrix());
    texture_scene->bind(0);
    geometry_scene->draw_as_triangles();
    
    // draw the sky:
    uniform_model_matrix->update_mat4(transformation_sky.get_matrix());
    texture_sky->bind(0);
    uniform_sky->update_uint(1);
    geometry_sky->draw_as_triangles();
    uniform_sky->update_uint(0);    
  }

void render()
  {
    uniform_mirror->update_uint(0);
    uniform_view_matrix->update_mat4(CameraHandler::camera_transformation.get_matrix());
    
    draw_scene();
    
    // draw the teapot mirror:
    uniform_mirror->update_uint(1); 

    cubemap->update_uniforms();
    cubemap->bind_textures();

    uniform_camera_position->update_vec3(CameraHandler::camera_transformation.get_translation());
    uniform_model_matrix->update_mat4(transformation_teapot.get_matrix());
    geometry_teapot->draw_as_triangles();
    uniform_mirror->update_uint(0);
    
    glutSwapBuffers();
  }

void recompute_cubemap_side(ReflectionTraceCubeMap *cube_map, GLuint side) 
  {
    cout << "rendering side" << endl;
    
    frame_buffer_cube->set_textures
      (
        cube_map->get_texture_depth(),side,
        0,0,
        cube_map->get_texture_color(),side,
        cube_map->get_texture_distance(),side 
      );
      
    frame_buffer_cube->activate();
    // set the camera:
    uniform_view_matrix->update_mat4(cube_map->get_camera_transformation(side).get_matrix());
    
    draw_scene();
    frame_buffer_cube->deactivate();
  }
  
void render_to_cubemap()
  {
    cubemap->transformation.set_translation(transformation_teapot.get_translation());
      
    uniform_projection_matrix->update_mat4(ReflectionTraceCubeMap::get_projection_matrix());
    
    uniform_mirror->update_uint(0);
    cubemap->set_viewport();
    recompute_cubemap_side(cubemap,GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    recompute_cubemap_side(cubemap,GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
    recompute_cubemap_side(cubemap,GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
    recompute_cubemap_side(cubemap,GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
    recompute_cubemap_side(cubemap,GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
    recompute_cubemap_side(cubemap,GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
    cubemap->unset_viewport();
    
    uniform_projection_matrix->update_mat4(projection_matrix);
    
    cubemap->get_texture_color()->load_from_gpu();
    cubemap->get_texture_color()->save_ppms("cubemap");
  }
  
void special_callback(int key, int x, int y)
  {
    switch(key)
      {
        case GLUT_KEY_DOWN:
          transformation_teapot.add_translation(glm::vec3(0.0,0.0,0.5));
          break;
          
        case GLUT_KEY_UP:
          transformation_teapot.add_translation(glm::vec3(0.0,0.0,-0.5));
          break;
        
        case GLUT_KEY_LEFT:
          transformation_teapot.add_translation(glm::vec3(-0.5,0.0,0.0));
          break;
          
        case GLUT_KEY_RIGHT:
          transformation_teapot.add_translation(glm::vec3(0.5,0.0,0.0));
          break;
          
        case GLUT_KEY_PAGE_UP:
          transformation_teapot.add_rotation(glm::vec3(0.1,0.0,0.0));
          break;
          
        case GLUT_KEY_PAGE_DOWN:
          transformation_teapot.add_rotation(glm::vec3(-0.1,0.0,0.0));
          break;
          
        case GLUT_KEY_HOME:
          transformation_teapot.add_rotation(glm::vec3(0.0,-0.1,0.0));
          break;
          
        case GLUT_KEY_END:
          transformation_teapot.add_rotation(glm::vec3(0.0,0.1,0.0));
          break;
          
        case GLUT_KEY_INSERT:
          render_to_cubemap();
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

    CameraHandler::camera_transformation.set_translation(glm::vec3(14.8467, 49.5787, -46.4311));
    CameraHandler::camera_transformation.set_rotation(glm::vec3(-0.547712, -3.70633, 0));
    
    Geometry3D g = load_obj("../resources/teapot.obj");
    geometry_teapot = &g;
    geometry_teapot->update_gpu();
    transformation_teapot.set_translation(glm::vec3(0.0,30.0,-30.0));
    transformation_teapot.set_scale(glm::vec3(15.0,15.0,15.0));

    Geometry3D g3 = load_obj("../resources/scene.obj");
    geometry_scene = &g3;
    geometry_scene->update_gpu();
  
    transformation_scene.set_translation(glm::vec3(0.0,0.0,-7.0));
    transformation_scene.set_scale(glm::vec3(6,6,6));
    
    transformation_sky.set_scale(glm::vec3(100.0,100.0,100.0));
    
    Geometry3D g4 = make_box_sharp(160,160,160);
    geometry_sky = &g4;
    geometry_sky->flip_triangles();
    geometry_sky->update_gpu();
    
    texture_sky = new Texture2D(16,16,TEXEL_TYPE_COLOR);
    texture_sky->load_ppm("../resources/sky.ppm");
    texture_sky->update_gpu();
    
    texture_scene = new Texture2D(16,16,TEXEL_TYPE_COLOR);
    texture_scene->load_ppm("../resources/scene.ppm");
    texture_scene->update_gpu();
    
    cout << "GL version: '" << glGetString(GL_VERSION) << "'" << endl;
    
    Shader shader(file_text("shader.vs",false,""),file_text("shader.fs",false,""),"",0,false);   // false - delay the validation, for AMD GPUs
    
    uniform_camera_position = new UniformVariable("camera_position");
    uniform_sampler = new UniformVariable("tex");
    uniform_view_matrix = new UniformVariable("view_matrix");
    uniform_mirror = new UniformVariable("mirror");
    uniform_sky = new UniformVariable("sky");
    uniform_model_matrix = new UniformVariable("model_matrix");
    uniform_projection_matrix = new UniformVariable("projection_matrix");
    
    uniform_camera_position->retrieve_location(&shader);
    uniform_sampler->retrieve_location(&shader);
    uniform_view_matrix->retrieve_location(&shader);
    uniform_mirror->retrieve_location(&shader);
    uniform_sky->retrieve_location(&shader);
    uniform_model_matrix->retrieve_location(&shader);
    uniform_projection_matrix->retrieve_location(&shader);
    
    projection_matrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.01f, 100000.0f);
    view_matrix = glm::mat4(1.0f);
    
    frame_buffer_cube = new FrameBuffer();
    cubemap = new ReflectionTraceCubeMap(CUBEMAP_RESOLUTION,"cubemap_texture","","","",1,2,3);
    cubemap->retrieve_uniform_locations(&shader);

    shader.use();

    cubemap->update_uniforms();
    cubemap->update_gpu();
    
    uniform_projection_matrix->update_mat4(projection_matrix);
    uniform_view_matrix->update_mat4(view_matrix);
    uniform_camera_position->update_float_3(0.0,0.0,-1.0);
    uniform_mirror->update_uint(0);
   
    shader.validate();     // delayed validation, for AMD GPUs
    
    render_to_cubemap();
   
    session->start();

    delete uniform_camera_position;
    delete uniform_sampler;
    delete uniform_view_matrix;
    delete uniform_mirror;         
    delete uniform_sky;             
    delete uniform_model_matrix;
    delete uniform_projection_matrix;
    delete texture;
    delete texture_sky;
    delete texture_scene;
    delete frame_buffer_cube;  
    delete cubemap; 
    
    GLSession::clear();
    
    return 0;
  }