#include "../gl_wrapper.h"

#define CAMERA_STEP 0.1
#define ROTATION_STEP 0.1
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

TransformationTRSModel transformation_cup;
TransformationTRSModel transformation_cow;
TransformationTRSModel transformation_rock;
TransformationTRSModel transformation_mirror;
TransformationTRSCamera transformation_camera;

glm::mat4 view_matrix = glm::mat4(1.0f);
glm::mat4 projection_matrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.01f, 100.0f);

GLint color_location;
GLint light_direction_location;
GLint sampler_location;
GLint sampler_location2;
GLint sampler_cube_location;
GLint view_matrix_location;
GLint mirror_location;
GLint model_matrix_location;
GLint projection_matrix_location;
GLint camera_position_location;

Shader *shader1;
Shader *shader2;

FrameBuffer *frame_buffer_cube;      // for rendering to cubemap
FrameBuffer *frame_buffer_camera;    // for "deferred shading" like rendering

Geometry3D *geometry_cup;
Geometry3D *geometry_cow;
Geometry3D *geometry_rock;
Geometry3D *geometry_room;
Geometry3D *geometry_quad;
Geometry3D *geometry_mirror;

Texture2D *texture_room;
Texture2D *texture_cow;
Texture2D *texture_rock;
Texture2D *texture_cup;

Texture2D *texture_camera_color;
Texture2D *texture_camera_depth;

bool draw_mirror = true;

EnvironmentCubeMap *cube_map;

Texture2D *texture_mirror;
Texture2D *texture_mirror_depth;

bool clicked = false;     // whether mouse was clicked
int initial_mouse_coords[2];
glm::vec3 initial_camera_rotation;

void draw_scene()
  {    
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    texture_cup->bind(1);
    glUniformMatrix4fv(model_matrix_location,1,GL_TRUE,glm::value_ptr(transformation_cup.get_matrix()));
    geometry_cup->draw_as_triangles();

    texture_rock->bind(1);
    glUniformMatrix4fv(model_matrix_location,1,GL_TRUE,glm::value_ptr(transformation_rock.get_matrix()));
    geometry_rock->draw_as_triangles();

    texture_cow->bind(1);
    glUniformMatrix4fv(model_matrix_location,1,GL_TRUE,glm::value_ptr(transformation_cow.get_matrix()));
    geometry_cow->draw_as_triangles();
 
    texture_room->bind(1);
    glUniformMatrix4fv(model_matrix_location,1,GL_TRUE,glm::value_ptr(glm::mat4(1.0)));
    geometry_room->draw_as_triangles();
    
    glUniformMatrix4fv(model_matrix_location,1,GL_TRUE, glm::value_ptr(transformation_mirror.get_matrix()));
    
    // draw the mirror:
    
    if (draw_mirror)
      {
        cube_map->get_texture_color()->bind(0);
        glUniform1ui(mirror_location,1);
        geometry_mirror->draw_as_triangles();    
        glUniform1ui(mirror_location,0);
      }
  }

void draw_quad()  // for the second pass
  {
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    texture_camera_color->bind(1);
    geometry_quad->draw_as_triangles();
    glEnable(GL_DEPTH_TEST);
  }
  
void render()
  {
    shader1->use();
    glUniform1i(sampler_location,1);
    glUniform1i(sampler_cube_location,0);
    glUniformMatrix4fv(view_matrix_location,1,GL_TRUE, glm::value_ptr(view_matrix));
    glUniform3f(light_direction_location,0.0,0.0,-1.0);
    glUniform1ui(mirror_location,0);
    
    // set up the camera:
    glUniformMatrix4fv(view_matrix_location,1,GL_TRUE,glm::value_ptr(CameraHandler::camera_transformation.get_matrix()));
    glUniformMatrix4fv(projection_matrix_location,1,GL_TRUE, glm::value_ptr(projection_matrix));
    glUniform3fv(camera_position_location,1,glm::value_ptr(CameraHandler::camera_transformation.get_translation()));
    
    // 1st pass:
    frame_buffer_camera->activate();
    draw_scene();
    frame_buffer_camera->deactivate();
   
    // 2nd pass:
    shader2->use(); 
    glUniform1i(sampler_location2,1);
    draw_quad();
    
    ErrorWriter::checkGlErrors("rendering loop");
    glutSwapBuffers();
  }
  
void recompute_cubemap_side(GLuint side) 
  {
    cout << "rendering side" << endl;
    frame_buffer_cube->set_textures(cube_map->get_texture_depth(),side,0,0,cube_map->get_texture_color(),side);    
    frame_buffer_cube->activate();
    // set the camera:
    glUniformMatrix4fv(view_matrix_location,1,GL_TRUE,glm::value_ptr(cube_map->get_camera_transformation(side).get_matrix()));
    draw_mirror = false;
    draw_scene();
    draw_mirror = true;
    frame_buffer_cube->deactivate();
  }
  
void recompute_cubemap()
  {
    cube_map->transformation.set_translation(transformation_mirror.get_translation());
    glUniformMatrix4fv(projection_matrix_location,1,GL_TRUE,glm::value_ptr(cube_map->get_projection_matrix()));
    cube_map->setViewport();
    cout << "rendering cube map..." << endl;
    recompute_cubemap_side(GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    recompute_cubemap_side(GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
    recompute_cubemap_side(GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
    recompute_cubemap_side(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
    recompute_cubemap_side(GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
    recompute_cubemap_side(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
    cube_map->unsetViewport();
    cube_map->get_texture_color()->load_from_gpu();
    cube_map->get_texture_depth()->load_from_gpu();
    cube_map->get_texture_color()->save_ppms("cubemap_images/cube_map");

    cube_map->get_texture_depth()->raise_to_power(256);
    
    cube_map->get_texture_depth()->save_ppms("cubemap_images/cube_map_depth");
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
        
        case GLUT_KEY_INSERT:
          recompute_cubemap();
          texture_camera_color->load_from_gpu();
          texture_camera_color->get_image_data()->save_ppm("camera.ppm");
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
    
    frame_buffer_cube = new FrameBuffer();
    frame_buffer_camera = new FrameBuffer();
    
    Geometry3D g = load_obj("cup.obj",true);
    geometry_cup = &g;
    geometry_cup->update_gpu();
    
    Geometry3D g2 = load_obj("cow.obj");
    geometry_cow = &g2;
    geometry_cow->update_gpu();
    
    Geometry3D g3 = load_obj("rock.obj");
    geometry_rock = &g3;
    geometry_rock->update_gpu();
    
    Geometry3D g4 = make_box(60,60,60);
    geometry_room = &g4;
    geometry_room->update_gpu();
    
    Geometry3D g5 = load_obj("teapot.obj");
    geometry_mirror = &g5;
    geometry_mirror->update_gpu();
    
    Geometry3D g6 = make_quad(2.0,2.0,0.5);
    geometry_quad = &g6;
    geometry_quad->update_gpu();
    
    texture_room = new Texture2D(16,16,TEXEL_TYPE_COLOR);
    texture_room->load_ppm("sky_test.ppm");
    texture_room->update_gpu();
    
    texture_cow = new Texture2D(16,16,TEXEL_TYPE_COLOR);
    texture_cow->load_ppm("cow.ppm");
    texture_cow->update_gpu();
    
    texture_rock = new Texture2D(16,16,TEXEL_TYPE_COLOR);
    texture_rock->load_ppm("rock.ppm");
    texture_rock->update_gpu();
    
    texture_cup = new Texture2D(16,16,TEXEL_TYPE_COLOR);
    texture_cup->load_ppm("cup.ppm");
    texture_cup->update_gpu();

    texture_camera_color = new Texture2D(512,512,TEXEL_TYPE_COLOR);
    texture_camera_color->update_gpu();
    
    texture_camera_depth = new Texture2D(512,512,TEXEL_TYPE_DEPTH);
    texture_camera_depth->update_gpu();
    
    cube_map = new EnvironmentCubeMap(512);
    cube_map->update_gpu();
    ErrorWriter::checkGlErrors("cube map init",true);
    
    transformation_cup.set_translation(glm::vec3(0.0,0.0,7.0));
    transformation_cow.set_translation(glm::vec3(15.0,5.0,-2.0));
    transformation_rock.set_translation(glm::vec3(-15.0,-1.0,-1.0));
    transformation_rock.set_scale(glm::vec3(0.5,0.5,0.5));   
    transformation_mirror.set_translation(glm::vec3(0.0,0.0,-1.0));
    transformation_mirror.set_scale(glm::vec3(4.0,4.0,4.0));
    transformation_mirror.set_rotation(glm::vec3(0.0,0.0,0));
    
    texture_mirror = new Texture2D(512,512,TEXEL_TYPE_COLOR);
    texture_mirror->set_parameter_int(GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    texture_mirror->set_parameter_int(GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    texture_mirror->update_gpu();
    
    texture_mirror_depth = new Texture2D(512,512,TEXEL_TYPE_DEPTH);
    texture_mirror_depth->set_parameter_int(GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    texture_mirror_depth->set_parameter_int(GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    texture_mirror_depth->update_gpu();
    
    Shader shad1(file_text("shader.vs"),file_text("shader.fs"));
    Shader shad2(file_text("shader2.vs"),file_text("shader2.fs"));
    
    shader1 = &shad1;
    shader2 = &shad2;
    
    light_direction_location = shader1->get_uniform_location("light_direction");
    camera_position_location = shader1->get_uniform_location("camera_position");
    mirror_location = shader1->get_uniform_location("mirror");
    sampler_location = shader1->get_uniform_location("tex");
    sampler_cube_location = shader1->get_uniform_location("tex_cube");    
    model_matrix_location = shader1->get_uniform_location("model_matrix");
    view_matrix_location = shader1->get_uniform_location("view_matrix");
    projection_matrix_location = shader1->get_uniform_location("projection_matrix");
    
    sampler_location2 = shader2->get_uniform_location("tex");
    
    ErrorWriter::checkGlErrors("after init",true);
    
    frame_buffer_camera->set_textures(texture_camera_depth,GL_TEXTURE_2D,0,0,texture_camera_color,GL_TEXTURE_2D);
        
    session->start();
    
    delete frame_buffer_cube;
    delete texture_camera_color;
    delete frame_buffer_camera;
    delete texture_cup;
    delete texture_mirror;
    delete cube_map;
    delete texture_rock;
    delete texture_room;
    delete texture_cow;
    delete texture_mirror_depth;
    GLSession::clear();
    return 0;
  }