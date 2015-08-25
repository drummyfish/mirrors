#include "../gl_wrapper.h"

#define CAMERA_STEP 0.1
#define ROTATION_STEP 0.1
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

TransformationTRSModel transformation_scene;
TransformationTRSModel transformation_mirror;
TransformationTRSModel transformation_sky;
TransformationTRSModel transformation_cube_map1;
TransformationTRSModel transformation_cube_map2;

Geometry3D *geometry_scene;
Geometry3D *geometry_sky;
Geometry3D *geometry_quad;
Geometry3D *geometry_mirror;
Geometry3D *geometry_box;             // box marking the cube map positions

glm::mat4 view_matrix = glm::mat4(1.0f);
glm::mat4 projection_matrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.01f, 1000.0f);

struct     // uniform locations
  {
    GLint light_direction;
    GLint texture_2d;
    GLint texture_color;
    GLint texture_normal;
    GLint texture_position;
    GLint texture_stencil;
    GLint texture_to_display;
    GLint texture_cube;
    GLint view_matrix;
    GLint mirror;
    GLint sky;
    GLint box;
    GLint model_matrix;
    GLint projection_matrix;
    GLint camera_position;
  } uniforms;

Shader *shader_3d;                   // for first pass: renders a 3D scene
Shader *shader_quad;                 // for second pass: draws textures on quad

FrameBuffer *frame_buffer_cube;      // for rendering to cubemap
FrameBuffer *frame_buffer_camera;    // for "deferred shading" like rendering

Texture2D *texture_sky;
Texture2D *texture_scene;
Texture2D *texture_camera_color;
Texture2D *texture_camera_depth;
Texture2D *texture_camera_position;
Texture2D *texture_camera_normal;
Texture2D *texture_camera_stencil;

bool draw_mirror = true;

int texture_to_display = 1;

EnvironmentCubeMap *cube_map1;
EnvironmentCubeMap *cube_map2;

Texture2D *texture_mirror;
Texture2D *texture_mirror_depth;

void draw_scene()
  {    
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUniform1ui(uniforms.sky,1);
    texture_sky->bind(1);
    glUniformMatrix4fv(uniforms.model_matrix,1,GL_TRUE,glm::value_ptr(transformation_sky.get_matrix()));
    geometry_sky->draw_as_triangles();
    glUniform1ui(uniforms.sky,0);
    
    texture_scene->bind(1);
    glUniformMatrix4fv(uniforms.model_matrix,1,GL_TRUE,glm::value_ptr(transformation_scene.get_matrix()));
    geometry_scene->draw_as_triangles();
    
    // draw the mirror stuff:
    
    if (draw_mirror)
      { // draw the mark boxes:
        glUniform1ui(uniforms.box,1);
        
        glUniformMatrix4fv(uniforms.model_matrix,1,GL_TRUE, glm::value_ptr(transformation_cube_map1.get_matrix()));
        geometry_box->draw_as_triangles();
        
        glUniformMatrix4fv(uniforms.model_matrix,1,GL_TRUE, glm::value_ptr(transformation_cube_map2.get_matrix()));
        geometry_box->draw_as_triangles();
        
        glUniform1ui(uniforms.box,0);
        
        // draw the mirror:
        glUniformMatrix4fv(uniforms.model_matrix,1,GL_TRUE, glm::value_ptr(transformation_mirror.get_matrix()));
        glUniform1ui(uniforms.mirror,1);
        geometry_mirror->draw_as_triangles();    
        glUniform1ui(uniforms.mirror,0);
      }
  }

void draw_quad()  // for the second pass
  {
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    cube_map1->get_texture_color()->bind(0);
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
    glUniform1i(uniforms.texture_2d,1);
    glUniformMatrix4fv(uniforms.view_matrix,1,GL_TRUE, glm::value_ptr(view_matrix));
    glUniform3f(uniforms.light_direction,0.0,0.0,-1.0);
    glUniform1ui(uniforms.mirror,0);
  }
  
void set_up_pass2()
  {
    shader_quad->use(); 
    glUniform1i(uniforms.texture_cube,0);
    glUniform1i(uniforms.texture_color,1);
    glUniform1i(uniforms.texture_normal,2);
    glUniform1i(uniforms.texture_position,3);
    glUniform1i(uniforms.texture_stencil,4);
    glUniform1i(uniforms.texture_to_display,texture_to_display);
    glUniform3fv(uniforms.camera_position,1,glm::value_ptr(CameraHandler::camera_transformation.get_translation()));
  }
 
void render()
  {    
    set_up_pass1();
    
    // set up the camera:
    glUniformMatrix4fv(uniforms.view_matrix,1,GL_TRUE,glm::value_ptr(CameraHandler::camera_transformation.get_matrix()));
    glUniformMatrix4fv(uniforms.projection_matrix,1,GL_TRUE, glm::value_ptr(projection_matrix));
    
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
    frame_buffer_cube->set_textures(cube_map1->get_texture_depth(),side,0,0,cube_map1->get_texture_color(),side);    
    frame_buffer_cube->activate();
    // set the camera:
    glUniformMatrix4fv(uniforms.view_matrix,1,GL_TRUE,glm::value_ptr(cube_map1->get_camera_transformation(side).get_matrix()));
    draw_mirror = false;
    draw_scene();
    draw_mirror = true;
    frame_buffer_cube->deactivate();
  }
  
void recompute_cubemap()
  {
    set_up_pass1();
    cube_map1->transformation.set_translation(transformation_cube_map1.get_translation());
    glUniformMatrix4fv(uniforms.projection_matrix,1,GL_TRUE,glm::value_ptr(cube_map1->get_projection_matrix()));
    cube_map1->setViewport();
    cout << "rendering cube map..." << endl;
    recompute_cubemap_side(GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    recompute_cubemap_side(GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
    recompute_cubemap_side(GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
    recompute_cubemap_side(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
    recompute_cubemap_side(GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
    recompute_cubemap_side(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
    cube_map1->unsetViewport();  
    cube_map1->get_texture_color()->load_from_gpu();  
    cube_map1->get_texture_depth()->load_from_gpu();
    cube_map1->get_texture_color()->save_ppms("cubemap_images/cube_map1");
    cube_map1->get_texture_depth()->raise_to_power(256);  
    cube_map1->get_texture_depth()->save_ppms("cubemap_images/cube_map1_depth");
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
          texture_camera_color->get_image_data()->save_ppm("camera/color.ppm",false);
          texture_camera_depth->load_from_gpu();
          texture_camera_depth->get_image_data()->raise_to_power(256); 
          texture_camera_depth->get_image_data()->save_ppm("camera/depth.ppm",false); 
          texture_camera_position->load_from_gpu();
          
          float r,g,b,a;
          
          texture_camera_position->get_image_data()->get_pixel(300,200,&r,&g,&b,&a);
          texture_camera_position->get_image_data()->save_ppm("camera/position.ppm",false);
          
          texture_camera_normal->load_from_gpu();
          texture_camera_normal->get_image_data()->save_ppm("camera/normal.ppm",false);
          
          texture_camera_stencil->load_from_gpu();
          texture_camera_stencil->get_image_data()->save_ppm("camera/stencil.ppm",false);
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
          
        case GLUT_KEY_F5:
          texture_to_display = 0;
          break;
          
        case GLUT_KEY_F11:
          transformation_cube_map1.set_translation(CameraHandler::camera_transformation.get_translation());
          transformation_cube_map1.add_translation(CameraHandler::camera_transformation.get_direction_forward() * 5.0f);
          break;
          
        case GLUT_KEY_F12:
          transformation_cube_map2.set_translation(CameraHandler::camera_transformation.get_translation());
          transformation_cube_map2.add_translation(CameraHandler::camera_transformation.get_direction_forward() * 5.0f);
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
    
    CameraHandler::camera_transformation.set_translation(glm::vec3(17.5,6.0,24.0));
    CameraHandler::camera_transformation.set_rotation(glm::vec3(-0.05,0.1,0.0));
    CameraHandler::translation_step = 2.0;
    
    glDisable(GL_CULL_FACE);    // the mirror will reverse the vertex order :/
    
    frame_buffer_cube = new FrameBuffer();
    frame_buffer_camera = new FrameBuffer();
    
    Geometry3D g2 = make_box(1,1,1);
    geometry_box = &g2;
    geometry_box->update_gpu();
    
    Geometry3D g3 = load_obj("scene.obj");
    geometry_scene = &g3;
    geometry_scene->update_gpu();
    
    Geometry3D g4 = load_obj("sky.obj");//make_box(180,180,180);
    geometry_sky = &g4;
    geometry_sky->update_gpu();
    
    Geometry3D g5 = load_obj("teapot.obj");

    geometry_mirror = &g5;
    geometry_mirror->update_gpu();
    
    Geometry3D g6 = make_quad(2.0,2.0,0.5);
    geometry_quad = &g6;
    geometry_quad->update_gpu();
    
    texture_sky = new Texture2D(16,16,TEXEL_TYPE_COLOR);
    texture_sky->load_ppm("sky.ppm");
    texture_sky->update_gpu();
    
    texture_scene = new Texture2D(16,16,TEXEL_TYPE_COLOR);
    texture_scene->load_ppm("scene.ppm");
    texture_scene->update_gpu();

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

    cube_map1 = new EnvironmentCubeMap(512);
    cube_map1->update_gpu();
    ErrorWriter::checkGlErrors("cube map init",true);
    
    transformation_cube_map1.set_translation(glm::vec3(18,35,-17));
    transformation_cube_map2.set_translation(glm::vec3(-18,35,-22));
    
    transformation_sky.set_scale(glm::vec3(50.0,50.0,50.0));
    transformation_scene.set_translation(glm::vec3(0.0,0.0,-7.0));
    transformation_scene.set_scale(glm::vec3(6,6,6));   
    transformation_mirror.set_translation(glm::vec3(0.0,30.0,-30.0));
    transformation_mirror.set_scale(glm::vec3(15.0,15.0,15.0));
    transformation_mirror.set_rotation(glm::vec3(0.0,0.0,0));
    
    texture_mirror = new Texture2D(512,512,TEXEL_TYPE_COLOR);
    texture_mirror->set_parameter_int(GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    texture_mirror->set_parameter_int(GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    texture_mirror->update_gpu();
    
    texture_mirror_depth = new Texture2D(512,512,TEXEL_TYPE_DEPTH);
    texture_mirror_depth->set_parameter_int(GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    texture_mirror_depth->set_parameter_int(GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    texture_mirror_depth->update_gpu();
    
    Shader shad1(file_text("shader_3d.vs",true),file_text("shader_3d.fs",true));
    Shader shad2(file_text("shader_quad.vs",true),file_text("shader_quad.fs",true));
    
    shader_3d = &shad1;
    shader_quad = &shad2;
    
    uniforms.light_direction = shader_3d->get_uniform_location("light_direction");
    uniforms.mirror = shader_3d->get_uniform_location("mirror");
    uniforms.sky = shader_3d->get_uniform_location("sky");
    uniforms.texture_2d = shader_3d->get_uniform_location("texture_2d");   
    uniforms.model_matrix = shader_3d->get_uniform_location("model_matrix");
    uniforms.view_matrix = shader_3d->get_uniform_location("view_matrix");
    uniforms.projection_matrix = shader_3d->get_uniform_location("projection_matrix");
    uniforms.box = shader_3d->get_uniform_location("box");
    
    uniforms.texture_color = shader_quad->get_uniform_location("texture_color");
    uniforms.texture_normal = shader_quad->get_uniform_location("texture_normal");
    uniforms.texture_position = shader_quad->get_uniform_location("texture_position");
    uniforms.texture_stencil = shader_quad->get_uniform_location("texture_stencil");
    uniforms.texture_to_display = shader_quad->get_uniform_location("texture_to_display");
    uniforms.texture_cube = shader_quad->get_uniform_location("texture_cube"); 
    uniforms.camera_position = shader_quad->get_uniform_location("camera_position");
    
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
    delete texture_mirror;
    delete cube_map1;
    delete texture_scene;
    delete texture_sky;
    delete texture_mirror_depth;
    GLSession::clear();
    return 0;
  }