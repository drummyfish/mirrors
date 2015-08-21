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

Geometry3D *geometry_cup;
Geometry3D *geometry_cow;
Geometry3D *geometry_rock;
Geometry3D *geometry_room;
Geometry3D *geometry_quad;
Geometry3D *geometry_mirror;

glm::mat4 view_matrix = glm::mat4(1.0f);
glm::mat4 projection_matrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.01f, 100.0f);

GLint uniform_location_light_direction;
GLint uniform_location_texture_2d;
GLint uniform_location_texture_color;
GLint uniform_location_texture_normal;
GLint uniform_location_texture_position;
GLint uniform_location_texture_stencil;
GLint uniform_location_texture_to_display;
GLint uniform_location_texture_cube;
GLint uniform_location_view_matrix;
GLint uniform_location_mirror;
GLint uniform_location_model_matrix;
GLint uniform_location_projection_matrix;
GLint uniform_location_camera_position;

Shader *shader_3d;                   // for first pass: renders a 3D scene
Shader *shader_quad;                 // for second pass: draws textures on quad

FrameBuffer *frame_buffer_cube;      // for rendering to cubemap
FrameBuffer *frame_buffer_camera;    // for "deferred shading" like rendering

Texture2D *texture_room;
Texture2D *texture_cow;
Texture2D *texture_rock;
Texture2D *texture_cup;
Texture2D *texture_camera_color;
Texture2D *texture_camera_depth;
Texture2D *texture_camera_position;
Texture2D *texture_camera_normal;
Texture2D *texture_camera_stencil;

bool draw_mirror = true;

int texture_to_display = 1;

EnvironmentCubeMap *cube_map;

Texture2D *texture_mirror;
Texture2D *texture_mirror_depth;

void draw_scene()
  {    
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    texture_cup->bind(1);
    glUniformMatrix4fv(uniform_location_model_matrix,1,GL_TRUE,glm::value_ptr(transformation_cup.get_matrix()));
    geometry_cup->draw_as_triangles();

    texture_rock->bind(1);
    glUniformMatrix4fv(uniform_location_model_matrix,1,GL_TRUE,glm::value_ptr(transformation_rock.get_matrix()));
    geometry_rock->draw_as_triangles();

    texture_cow->bind(1);
    glUniformMatrix4fv(uniform_location_model_matrix,1,GL_TRUE,glm::value_ptr(transformation_cow.get_matrix()));
    geometry_cow->draw_as_triangles();
 
    texture_room->bind(1);
    glUniformMatrix4fv(uniform_location_model_matrix,1,GL_TRUE,glm::value_ptr(glm::mat4(1.0)));
    geometry_room->draw_as_triangles();
    
    glUniformMatrix4fv(uniform_location_model_matrix,1,GL_TRUE, glm::value_ptr(transformation_mirror.get_matrix()));
    
    // draw the mirror:
    
    if (draw_mirror)
      {
        glUniform1ui(uniform_location_mirror,1);
        geometry_mirror->draw_as_triangles();    
        glUniform1ui(uniform_location_mirror,0);
      }
  }

void draw_quad()  // for the second pass
  {
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    cube_map->get_texture_color()->bind(0);
    texture_camera_color->bind(1);
    texture_camera_normal->bind(2);
    texture_camera_position->bind(3);
    texture_camera_stencil->bind(4);
    geometry_quad->draw_as_triangles();
    glEnable(GL_DEPTH_TEST);
  }
 
void set_up_pass1()
  {
    shader_3d->use();
    glUniform1i(uniform_location_texture_2d,1);
    glUniformMatrix4fv(uniform_location_view_matrix,1,GL_TRUE, glm::value_ptr(view_matrix));
    glUniform3f(uniform_location_light_direction,0.0,0.0,-1.0);
    glUniform1ui(uniform_location_mirror,0);
  }
  
void set_up_pass2()
  {
    shader_quad->use(); 
    glUniform1i(uniform_location_texture_cube,0);
    glUniform1i(uniform_location_texture_color,1);
    glUniform1i(uniform_location_texture_normal,2);
    glUniform1i(uniform_location_texture_position,3);
    glUniform1i(uniform_location_texture_stencil,4);
    glUniform1i(uniform_location_texture_to_display,texture_to_display);
    glUniform3fv(uniform_location_camera_position,1,glm::value_ptr(CameraHandler::camera_transformation.get_translation()));
  }
 
void render()
  {    
    set_up_pass1();
    
    // set up the camera:
    glUniformMatrix4fv(uniform_location_view_matrix,1,GL_TRUE,glm::value_ptr(CameraHandler::camera_transformation.get_matrix()));
    glUniformMatrix4fv(uniform_location_projection_matrix,1,GL_TRUE, glm::value_ptr(projection_matrix));
    
    // 1st pass:
    frame_buffer_camera->activate();
    draw_scene();
    frame_buffer_camera->deactivate();
   
    // 2nd pass:
    set_up_pass2();
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
    glUniformMatrix4fv(uniform_location_view_matrix,1,GL_TRUE,glm::value_ptr(cube_map->get_camera_transformation(side).get_matrix()));
    draw_mirror = false;
    draw_scene();
    draw_mirror = true;
    frame_buffer_cube->deactivate();
  }
  
void recompute_cubemap()
  {
    set_up_pass1();
    cube_map->transformation.set_translation(transformation_mirror.get_translation());
    glUniformMatrix4fv(uniform_location_projection_matrix,1,GL_TRUE,glm::value_ptr(cube_map->get_projection_matrix()));
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
          texture_camera_color->get_image_data()->save_ppm("camera/color.ppm");
          texture_camera_depth->load_from_gpu();
          texture_camera_depth->get_image_data()->raise_to_power(256); 
          texture_camera_depth->get_image_data()->save_ppm("camera/depth.ppm"); 
          texture_camera_position->load_from_gpu();
          
          float r,g,b,a;
          
          texture_camera_position->get_image_data()->get_pixel(300,200,&r,&g,&b,&a);
          texture_camera_position->get_image_data()->save_ppm("camera/position.ppm");
          
          texture_camera_normal->load_from_gpu();
          texture_camera_normal->get_image_data()->save_ppm("camera/normal.ppm");
          
          texture_camera_stencil->load_from_gpu();
          texture_camera_stencil->get_image_data()->save_ppm("camera/stencil.ppm");
          break;
          
        case GLUT_KEY_F1:
          texture_to_display = 1;
          break;
        
        case GLUT_KEY_F2:
          texture_to_display = 2;
          break;
          
        case GLUT_KEY_F3:
          texture_to_display = 3;
          break;
          
        case GLUT_KEY_F4:
          texture_to_display = 4;
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

    texture_camera_color = new Texture2D(WINDOW_WIDTH,WINDOW_HEIGHT,TEXEL_TYPE_COLOR);
    texture_camera_color->update_gpu();
    
    texture_camera_depth = new Texture2D(WINDOW_WIDTH,WINDOW_HEIGHT,TEXEL_TYPE_DEPTH);
    texture_camera_depth->update_gpu();
    
    texture_camera_position = new Texture2D(WINDOW_WIDTH,WINDOW_HEIGHT,TEXEL_TYPE_COLOR);
    texture_camera_position->update_gpu();
    
    texture_camera_normal = new Texture2D(WINDOW_WIDTH,WINDOW_HEIGHT,TEXEL_TYPE_COLOR);
    texture_camera_normal->update_gpu();
    
    texture_camera_stencil = new Texture2D(WINDOW_WIDTH,WINDOW_HEIGHT,TEXEL_TYPE_COLOR);  // couldn't get stencil texture to work => using color instead
    texture_camera_stencil->update_gpu();

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
    
    Shader shad1(file_text("shader_3d.vs"),file_text("shader_3d.fs"));
    Shader shad2(file_text("shader_quad.vs"),file_text("shader_quad.fs"));
    
    shader_3d = &shad1;
    shader_quad = &shad2;
    
    uniform_location_light_direction = shader_3d->get_uniform_location("light_direction");
    uniform_location_mirror = shader_3d->get_uniform_location("mirror");
    uniform_location_texture_2d = shader_3d->get_uniform_location("texture_2d");   
    uniform_location_model_matrix = shader_3d->get_uniform_location("model_matrix");
    uniform_location_view_matrix = shader_3d->get_uniform_location("view_matrix");
    uniform_location_projection_matrix = shader_3d->get_uniform_location("projection_matrix");
    
    uniform_location_texture_color = shader_quad->get_uniform_location("texture_color");
    uniform_location_texture_normal = shader_quad->get_uniform_location("texture_normal");
    uniform_location_texture_position = shader_quad->get_uniform_location("texture_position");
    uniform_location_texture_stencil = shader_quad->get_uniform_location("texture_stencil");
    uniform_location_texture_to_display = shader_quad->get_uniform_location("texture_to_display");
    uniform_location_texture_cube = shader_quad->get_uniform_location("texture_cube"); 
    uniform_location_camera_position = shader_quad->get_uniform_location("camera_position");
    
    ErrorWriter::checkGlErrors("after init",true);
    
    frame_buffer_camera->set_textures(
      texture_camera_depth,GL_TEXTURE_2D,
      0,0,
      texture_camera_color,GL_TEXTURE_2D,
      texture_camera_position,GL_TEXTURE_2D,
      texture_camera_normal,GL_TEXTURE_2D,
      texture_camera_stencil,GL_TEXTURE_2D);
    
    ErrorWriter::checkGlErrors("frame buffer init",true);
    
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