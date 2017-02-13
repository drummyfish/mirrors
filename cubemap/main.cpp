#include "../gl_wrapper.h"

#define CAMERA_STEP 0.1
#define ROTATION_STEP 0.1
#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 240
#define CUBEMAP_RESOLUTION 512
#define NEAR 0.01f
#define FAR 1000.0f

//#define SELF_REFLECTIONS

TransformationTRSModel transformation_scene;
TransformationTRSModel transformation_mirror;
TransformationTRSModel transformation_sky;

Geometry3D *geometry_scene;
Geometry3D *geometry_sky;             // skybox
Geometry3D *geometry_line;            // helper marking line
Geometry3D *geometry_mirror;
Geometry3D *geometry_box;             // box marking the cube map positions

ShaderLog *shader_log;

glm::mat4 view_matrix = glm::mat4(1.0f);
glm::mat4 projection_matrix = glm::perspective(45.0f, 4.0f / 3.0f, NEAR, FAR);

UniformVariable uniform_mirror("mirror");
UniformVariable uniform_light_direction("light_direction");
UniformVariable uniform_texture_2d("texture_2d");
UniformVariable uniform_texture_color("texture_color");
UniformVariable uniform_texture_normal("texture_normal");
UniformVariable uniform_texture_position("texture_position");
UniformVariable uniform_texture_stencil("texture_stencil");
UniformVariable uniform_texture_to_display("texture_to_display");
UniformVariable uniform_acceleration_on("acceleration_on");
UniformVariable uniform_view_matrix("view_matrix");
UniformVariable uniform_sky("sky");
UniformVariable uniform_rendering_cubemap("rendering_cubemap");
UniformVariable uniform_marker("marker");
UniformVariable uniform_model_matrix("model_matrix");
UniformVariable uniform_projection_matrix("projection_matrix");
UniformVariable uniform_camera_position("camera_position");
UniformVariable uniform_cubemap_position("cubemap_position");

Shader *shader_3d;                   // for first pass: renders a 3D scene
Shader *shader_quad;                 // for second pass: draws textures on quad
Shader *shader_compute;              // for computing the acceleration texture

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
int acceleration_on = 1;
bool wait_for_key_release = false;

ReflectionTraceCubeMap *cubemaps[2];

Texture2D *texture_mirror;
Texture2D *texture_mirror_depth;

Profiler *profiler;

int info_countdown = 0;

void print_info()
  {
    profiler->print();
    cout << "approximate time/mirror pixel (us): " <<  (1000.0 * profiler->get_average_value(1) / profiler->get_average_value(2)) << endl;
    profiler->reset();
    
    cout << "-----" << endl;
    
    cout << "camera position: ";
    print_vec3(CameraHandler::camera_transformation.get_translation());
   
    cout << "camera rotation: ";
    print_vec3(CameraHandler::camera_transformation.get_rotation()); 
  
    cout << "cube1 position: ";
    print_vec3(cubemaps[0]->transformation.get_translation());
    
    cout << "camera to cube1: ";
    print_vec3(glm::normalize(cubemaps[0]->transformation.get_translation() - CameraHandler::camera_transformation.get_translation()));
    
    cout << "-------" << endl;
  }

void draw_scene()
  {        
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    uniform_sky.update_int(1);
    texture_sky->bind(1);
    uniform_model_matrix.update_mat4(transformation_sky.get_matrix());
    geometry_sky->draw_as_triangles();
    uniform_sky.update_int(0);
    
    texture_scene->bind(1);
    
    uniform_model_matrix.update_mat4(transformation_scene.get_matrix());
    geometry_scene->draw_as_triangles();
    
    uniform_marker.update_int(1);
    uniform_model_matrix.update_mat4(glm::mat4(1.0));
    // geometry_line->draw_as_lines();
    uniform_marker.update_int(0);
    
    // draw the mirror stuff:
    
    if (draw_mirror)
      { // draw the mark boxes:
        uniform_marker.update_int(1);
        uniform_model_matrix.update_mat4(cubemaps[0]->transformation.get_matrix());
        geometry_box->draw_as_triangles();
        
        uniform_model_matrix.update_mat4(cubemaps[1]->transformation.get_matrix());
        
        geometry_box->draw_as_triangles();       
        uniform_marker.update_int(0);
        
        // draw the mirror:
        uniform_model_matrix.update_mat4(transformation_mirror.get_matrix());
        uniform_mirror.update_int(1);
        profiler->fragment_count_measure_begin();
        geometry_mirror->draw_as_triangles();    
        profiler->record_value(2,profiler->fragment_count_measure_end());
        uniform_mirror.update_int(0);
      }
      
    #ifdef SELF_REFLECTIONS
    // mirror has to be always drawn for self reflections
    
    uniform_model_matrix.update_mat4(transformation_mirror.get_matrix());
    uniform_mirror.update_int(1);
    geometry_mirror->draw_as_triangles();
    uniform_mirror.update_int(0);
    #endif
  }

void draw_quad()  // for the second pass
  {
    cubemaps[0]->bind_textures();
    cubemaps[1]->bind_textures();
    
    texture_camera_color->bind(0);
    texture_camera_normal->bind(1);
    texture_camera_position->bind(2);
    texture_camera_stencil->bind(3);
      
    draw_fullscreen_quad();
  }

void set_up_pass1()
  {
    shader_3d->use();
    uniform_texture_2d.update_int(1);
    uniform_view_matrix.update_mat4(view_matrix);
    uniform_light_direction.update_float_3(0.0,0.0,-1.0);
    uniform_mirror.update_int(0);
    uniform_rendering_cubemap.update_int(0);
  }
  
void set_up_pass2()
  {
    shader_quad->use(); 
    
    cubemaps[0]->update_uniforms();
    cubemaps[1]->update_uniforms();
    
    uniform_texture_color.update_int(0);
    uniform_texture_normal.update_int(1);
    uniform_texture_position.update_int(2);
    uniform_texture_stencil.update_int(3);
    
    uniform_texture_to_display.update_int(texture_to_display);
    uniform_acceleration_on.update_int(acceleration_on);
    uniform_camera_position.update_vec3(CameraHandler::camera_transformation.get_translation());
  }

void render()
  { 
    info_countdown--;
    wait_for_key_release = false;
    
    if (info_countdown < 0)
      {
        info_countdown = 64;
        print_info();
      }
    
    set_up_pass1();
    
    // set up the camera:
    uniform_view_matrix.update_mat4(CameraHandler::camera_transformation.get_matrix());
    uniform_projection_matrix.update_mat4(projection_matrix);
    
    // 1st pass:
    profiler->time_measure_begin();
    frame_buffer_camera->activate();
    draw_scene();
    frame_buffer_camera->deactivate();
    profiler->record_value(0,profiler->time_measure_end());
   
    // 2nd pass:
    shader_log->bind();
    
    profiler->time_measure_begin();
    set_up_pass2();
    draw_quad();
    
    shader_log->load_from_gpu();

    shader_log->print();

    shader_log->clear();
    shader_log->update_gpu();
    
    profiler->record_value(1,profiler->time_measure_end());
    
    ErrorWriter::checkGlErrors("rendering loop");  
    glutSwapBuffers();

    profiler->next_frame();
  }
  
void recompute_cubemap_side(ReflectionTraceCubeMap *cube_map, GLuint side) 
  {
    cout << "rendering side" << endl;
    
    frame_buffer_cube->set_textures
      (
        cube_map->get_texture_depth(),side,
        0,0,
        cube_map->get_texture_color(),side,
        cube_map->get_texture_distance(),side, 
        cube_map->get_texture_normal(),side 
      );
      
    frame_buffer_cube->activate();
    // set the camera:
    uniform_view_matrix.update_mat4(cube_map->get_camera_transformation(side).get_matrix());
    
    draw_mirror = false;
    draw_scene();
    draw_mirror = true;
    frame_buffer_cube->deactivate();
  }
  
void save_images()
  {
    cout << "saving images" << endl;
    
/*    texture_camera_color->load_from_gpu();
    texture_camera_color->get_image_data()->save_ppm("camera/color.ppm",false);
    texture_camera_depth->load_from_gpu();
    texture_camera_depth->get_image_data()->raise_to_power(256); 
    texture_camera_depth->get_image_data()->save_ppm("camera/depth.ppm",false); 
    texture_camera_position->load_from_gpu();
    texture_camera_position->get_image_data()->save_ppm("camera/position.ppm",false);
    texture_camera_normal->load_from_gpu();
    texture_camera_normal->get_image_data()->save_ppm("camera/normal.ppm",false);
    texture_camera_stencil->load_from_gpu();
    texture_camera_stencil->get_image_data()->save_ppm("camera/stencil.ppm",false);
*/  

double coeff = 0.01;

int cube_index = 0;
cubemaps[cube_index]->get_texture_normal()->save_ppms("cubemap_images/cubemap_normal");

cubemaps[cube_index]->get_texture_distance()->set_mipmap_level(0);
cubemaps[cube_index]->get_texture_distance()->load_from_gpu();
cubemaps[cube_index]->get_texture_distance()->multiply(coeff);
cubemaps[cube_index]->get_texture_distance()->save_ppms("cubemap_images/acc/cubemap_distance_mip0");

cubemaps[cube_index]->get_texture_distance()->set_mipmap_level(1);
cubemaps[cube_index]->get_texture_distance()->load_from_gpu();
cubemaps[cube_index]->get_texture_distance()->multiply(coeff);
cubemaps[cube_index]->get_texture_distance()->save_ppms("cubemap_images/acc/cubemap_distance_mip1");

cubemaps[cube_index]->get_texture_distance()->set_mipmap_level(2);
cubemaps[cube_index]->get_texture_distance()->load_from_gpu();
cubemaps[cube_index]->get_texture_distance()->multiply(coeff);
cubemaps[cube_index]->get_texture_distance()->save_ppms("cubemap_images/acc/cubemap_distance_mip2");

cubemaps[cube_index]->get_texture_distance()->set_mipmap_level(3);
cubemaps[cube_index]->get_texture_distance()->load_from_gpu();
cubemaps[cube_index]->get_texture_distance()->multiply(coeff);
cubemaps[cube_index]->get_texture_distance()->save_ppms("cubemap_images/acc/cubemap_distance_mip3");

cubemaps[cube_index]->get_texture_distance()->set_mipmap_level(4);
cubemaps[cube_index]->get_texture_distance()->load_from_gpu();
cubemaps[cube_index]->get_texture_distance()->multiply(coeff);
cubemaps[cube_index]->get_texture_distance()->save_ppms("cubemap_images/acc/cubemap_distance_mip4");

cubemaps[cube_index]->get_texture_distance()->set_mipmap_level(5);
cubemaps[cube_index]->get_texture_distance()->load_from_gpu();
cubemaps[cube_index]->get_texture_distance()->multiply(coeff);
cubemaps[cube_index]->get_texture_distance()->save_ppms("cubemap_images/acc/cubemap_distance_mip5");

cubemaps[cube_index]->get_texture_distance()->set_mipmap_level(6);
cubemaps[cube_index]->get_texture_distance()->load_from_gpu();
cubemaps[cube_index]->get_texture_distance()->multiply(coeff);
cubemaps[cube_index]->get_texture_distance()->save_ppms("cubemap_images/acc/cubemap_distance_mip6");

cubemaps[cube_index]->get_texture_distance()->set_mipmap_level(7);
cubemaps[cube_index]->get_texture_distance()->load_from_gpu();
cubemaps[cube_index]->get_texture_distance()->multiply(coeff);
cubemaps[cube_index]->get_texture_distance()->save_ppms("cubemap_images/acc/cubemap_distance_mip7");

cubemaps[cube_index]->get_texture_distance()->set_mipmap_level(8);
cubemaps[cube_index]->get_texture_distance()->load_from_gpu();
cubemaps[cube_index]->get_texture_distance()->multiply(coeff);
cubemaps[cube_index]->get_texture_distance()->save_ppms("cubemap_images/acc/cubemap_distance_mip8");

cubemaps[cube_index]->get_texture_distance()->set_mipmap_level(9);
cubemaps[cube_index]->get_texture_distance()->load_from_gpu();
cubemaps[cube_index]->get_texture_distance()->multiply(coeff);
cubemaps[cube_index]->get_texture_distance()->save_ppms("cubemap_images/acc/cubemap_distance_mip9");

cubemaps[cube_index]->get_texture_distance()->set_mipmap_level(0);
cubemaps[cube_index]->get_texture_distance()->load_from_gpu();
cubemaps[cube_index]->get_texture_distance()->multiply(1.0 / coeff);
  }
  
void recompute_cubemap()
  {
    set_up_pass1();
    uniform_rendering_cubemap.update_int(1);
    
    uniform_projection_matrix.update_mat4(ReflectionTraceCubeMap::get_projection_matrix());
    
    uniform_cubemap_position.update_vec3(cubemaps[0]->transformation.get_translation());
    cubemaps[0]->set_viewport();
    cout << "rendering cube map 1..." << endl;
    recompute_cubemap_side(cubemaps[0],GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    recompute_cubemap_side(cubemaps[0],GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
    recompute_cubemap_side(cubemaps[0],GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
    recompute_cubemap_side(cubemaps[0],GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
    recompute_cubemap_side(cubemaps[0],GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
    recompute_cubemap_side(cubemaps[0],GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
    cubemaps[0]->unset_viewport();  
    
    uniform_cubemap_position.update_vec3(cubemaps[1]->transformation.get_translation());
    cubemaps[1]->set_viewport();
    cout << "rendering cube map 2..." << endl;
    recompute_cubemap_side(cubemaps[1],GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    recompute_cubemap_side(cubemaps[1],GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
    recompute_cubemap_side(cubemaps[1],GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
    recompute_cubemap_side(cubemaps[1],GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
    recompute_cubemap_side(cubemaps[1],GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
    recompute_cubemap_side(cubemaps[1],GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
    cubemaps[1]->unset_viewport();  

    cubemaps[0]->get_texture_color()->load_from_gpu();  
    cubemaps[0]->get_texture_depth()->load_from_gpu();
    cubemaps[0]->get_texture_distance()->load_from_gpu(); 
    cubemaps[0]->get_texture_normal()->load_from_gpu();     
    cubemaps[1]->get_texture_color()->load_from_gpu();  
    cubemaps[1]->get_texture_depth()->load_from_gpu();
    cubemaps[1]->get_texture_distance()->load_from_gpu(); 
    cubemaps[1]->get_texture_normal()->load_from_gpu();   
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
          
          cubemaps[0]->compute_acceleration_texture();
          cubemaps[1]->compute_acceleration_texture();
   //        cubemaps[0]->compute_acceleration_texture_sw();
   //       cubemaps[1]->compute_acceleration_texture_sw();
          
          save_images();
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

        case GLUT_KEY_F6:
          texture_to_display = 5;
          break;

        case GLUT_KEY_F7:
          texture_to_display = 6;
          break;
          
        case GLUT_KEY_F8:
          if (!wait_for_key_release)
            {
              acceleration_on = !acceleration_on;
              cout << "acceleration: " << acceleration_on << endl;
              wait_for_key_release = true;
            }
          break;
          
        case GLUT_KEY_F11:
          cubemaps[0]->transformation.set_translation(CameraHandler::camera_transformation.get_translation());
          cubemaps[0]->transformation.add_translation(CameraHandler::camera_transformation.get_direction_forward() * 5.0f);
          break;
          
        case GLUT_KEY_F12:
          cubemaps[1]->transformation.set_translation(CameraHandler::camera_transformation.get_translation());
          cubemaps[1]->transformation.add_translation(CameraHandler::camera_transformation.get_direction_forward() * 5.0f);
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
    
    profiler = new Profiler();
    profiler->new_value("pass 1");
    profiler->new_value("pass 2");
    profiler->new_value("mirror fragments");
    
    CameraHandler::camera_transformation.set_translation(glm::vec3(13.9726,39.3919,-48.0639));
    CameraHandler::camera_transformation.set_rotation(glm::vec3(-0.1825,-10.04,0));
    
    //CameraHandler::camera_transformation.set_translation(glm::vec3(-78.2378,90.4943,-118.219));
    //CameraHandler::camera_transformation.set_rotation(glm::vec3(-0.37875,-8.60999,0));
    CameraHandler::translation_step = 2.0;
    
    glDisable(GL_CULL_FACE);    // the mirror will reverse the vertex order :/
    
    frame_buffer_cube = new FrameBuffer();
    frame_buffer_camera = new FrameBuffer();
    
    Geometry3D g1;
    geometry_line = &g1;
    geometry_line->add_vertex(-10,10,5);
    geometry_line->add_vertex(10,5,-10);
    geometry_line->add_triangle(0,1,0);
    geometry_line->update_gpu();
    
    Geometry3D g2 = make_box(1,1,1);
    geometry_box = &g2;
    geometry_box->update_gpu();
    
    Geometry3D g3 = load_obj("../resources/scene.obj");
    geometry_scene = &g3;
    geometry_scene->update_gpu();
    
    Geometry3D g4 = make_box_sharp(6,6,6);
    geometry_sky = &g4;
    geometry_sky->update_gpu();
    
    Geometry3D g5 = load_obj("../resources/teapot.obj");//make_box_sharp(3,3,3);
    //Geometry3D g5 = load_obj("../resources/self_reflection_test.obj"); 
    
    geometry_mirror = &g5;
    geometry_mirror->update_gpu();
    
    texture_sky = new Texture2D(16,16,TEXEL_TYPE_COLOR);
    texture_sky->load_ppm("../resources/sky.ppm");
    texture_sky->update_gpu();
    
    texture_scene = new Texture2D(16,16,TEXEL_TYPE_COLOR);
    
    texture_scene->load_ppm("../resources/scene.ppm");
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
    
    cubemaps[0] = new ReflectionTraceCubeMap(CUBEMAP_RESOLUTION,"cubemaps[0].texture_color","cubemaps[0].texture_distance","cubemaps[0].texture_normal","cubemaps[0].position",4,5,6);
    cubemaps[0]->update_gpu();
    
    cubemaps[1] = new ReflectionTraceCubeMap(CUBEMAP_RESOLUTION,"cubemaps[1].texture_color","cubemaps[1].texture_distance","cubemaps[1].texture_normal","cubemaps[1].position",7,8,9);
    cubemaps[1]->update_gpu();
    
    ErrorWriter::checkGlErrors("cube map init",true);
      
    cubemaps[0]->transformation.set_translation(glm::vec3(10.0,30.0,-30.0));
    cubemaps[1]->transformation.set_translation(glm::vec3(-18,35,-22));
    
    transformation_sky.set_scale(glm::vec3(100.0,100.0,100.0));
    transformation_scene.set_translation(glm::vec3(0.0,0.0,-7.0));
    transformation_scene.set_scale(glm::vec3(6,6,6));   
    
    //transformation_mirror.set_translation(glm::vec3(0,0,0)); // TEMP
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
    
    Shader shad1(file_text("shader_3d.vs",true),file_text("shader_3d.fs",true),"");
    Shader shad2(VERTEX_SHADER_QUAD_TEXT,file_text("shader_quad.fs",true),"");
    
    shader_3d = &shad1;
    shader_quad = &shad2;
    
    if (!shader_3d->loaded_succesfully() || !shader_quad->loaded_succesfully())
      {
        cerr << "Shader error, halting." << endl;
        return 1;
      }
    
    uniform_mirror.retrieve_location(shader_3d);
    uniform_rendering_cubemap.retrieve_location(shader_3d);
    uniform_light_direction.retrieve_location(shader_3d);
    uniform_sky.retrieve_location(shader_3d);
    uniform_texture_2d.retrieve_location(shader_3d);   
    uniform_model_matrix.retrieve_location(shader_3d);
    uniform_view_matrix.retrieve_location(shader_3d);
    uniform_projection_matrix.retrieve_location(shader_3d);
    uniform_marker.retrieve_location(shader_3d);
    uniform_cubemap_position.retrieve_location(shader_3d);
    
    cubemaps[0]->retrieve_uniform_locations(shader_quad);
    cubemaps[1]->retrieve_uniform_locations(shader_quad);
    uniform_texture_color.retrieve_location(shader_quad);
    uniform_texture_normal.retrieve_location(shader_quad);
    uniform_texture_position.retrieve_location(shader_quad);
    uniform_texture_stencil.retrieve_location(shader_quad);
    uniform_texture_to_display.retrieve_location(shader_quad);
    uniform_acceleration_on.retrieve_location(shader_quad);
    uniform_camera_position.retrieve_location(shader_quad);
    
    ErrorWriter::checkGlErrors("after init",true);
    
    frame_buffer_camera->set_textures(
      texture_camera_depth,GL_TEXTURE_2D,
      0,0,
      texture_camera_color,GL_TEXTURE_2D,
      texture_camera_position,GL_TEXTURE_2D,
      texture_camera_normal,GL_TEXTURE_2D,
      texture_camera_stencil,GL_TEXTURE_2D);
    
    ErrorWriter::checkGlErrors("frame buffer init",true);

    shader_log = new ShaderLog();
    shader_log->set_print_limit(20);
    shader_log->update_gpu();
    
    special_callback(GLUT_KEY_INSERT,0,0);   // compute the cubemaps
    
    ErrorWriter::checkGlErrors("shader log init",true);
    
    session->start();
    
    delete shader_log;
    delete frame_buffer_cube;
    delete texture_camera_color;
    delete frame_buffer_camera;
    delete texture_mirror;
    delete cubemaps[0];
    delete cubemaps[1];
    delete texture_scene;
    delete texture_sky;
    delete texture_mirror_depth;
    delete profiler;
    GLSession::clear();
    return 0;
  }