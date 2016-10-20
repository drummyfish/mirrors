#include "../gl_wrapper.h"

#define CAMERA_STEP 0.1
#define ROTATION_STEP 0.1
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define CUBEMAP_RESOLUTION 512
#define NEAR 0.01f
#define FAR 1000.0f

TransformationTRSModel transformation_scene;
TransformationTRSModel transformation_mirror;
TransformationTRSModel transformation_sky;

Geometry3D *geometry_scene;
Geometry3D *geometry_sky;             // skybox
Geometry3D *geometry_line;            // helper marking line
Geometry3D *geometry_quad;            // quad for the second render pass
Geometry3D *geometry_mirror;
Geometry3D *geometry_box;             // box marking the cube map positions

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
UniformVariable uniform_view_matrix("view_matrix");
UniformVariable uniform_sky("sky");
UniformVariable uniform_rendering_cubemap("rendering_cubemap");
UniformVariable uniform_marker("marker");
UniformVariable uniform_model_matrix("model_matrix");
UniformVariable uniform_projection_matrix("projection_matrix");
UniformVariable uniform_camera_position("camera_position");
UniformVariable uniform_cubemap_position("cubemap_position");

UniformVariable uniform_textures_acceleration_0("acceleration_textures[0]");
UniformVariable uniform_textures_acceleration_1("acceleration_textures[1]");

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

Texture2D *acceleration_textures[2];

bool draw_mirror = true;

int texture_to_display = 1;

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
  }

void draw_quad()  // for the second pass
  {
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    
    cubemaps[0]->bind_textures();
    cubemaps[1]->bind_textures();
    
    texture_camera_color->bind(0);
    texture_camera_normal->bind(1);
    texture_camera_position->bind(2);
    texture_camera_stencil->bind(3);
    
    acceleration_textures[0]->bind(8);
    acceleration_textures[1]->bind(9);
      
    geometry_quad->draw_as_triangles(); 
    glEnable(GL_DEPTH_TEST);
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
    uniform_textures_acceleration_0.update_int(8);
    uniform_textures_acceleration_1.update_int(9);
    
    uniform_texture_to_display.update_int(texture_to_display);
    uniform_camera_position.update_vec3(CameraHandler::camera_transformation.get_translation());
  }
 
void render()
  {        
    info_countdown--;
    
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
    profiler->time_measure_begin();
    set_up_pass2();
    draw_quad();   
    
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
        cube_map->get_texture_distance(),side 
      );
      
    frame_buffer_cube->activate();
    // set the camera:
    uniform_view_matrix.update_mat4(cube_map->get_camera_transformation(side).get_matrix());
    
    draw_mirror = false;
    draw_scene();
    draw_mirror = true;
    frame_buffer_cube->deactivate();
  }
  
  /**
   * Creates an acceleration texture from given cubemap in software.
   * 
   * @param cubemap cubemap to create the texture for
   * @return newly created acceleration texture
   */
  
Texture2D *new_acceleration_texture(int cubemap_resolution)
  {
    return new Texture2D(cubemap_resolution * 3,cubemap_resolution * 2,TEXEL_TYPE_DEPTH);
  }
  
void create_acceleration_texture_sw(ReflectionTraceCubeMap *cubemap, Texture2D *result)
  {
    int size = cubemap->get_texture_color()->image_front->get_width();
    int half_size = size / 2;
    int offset_x, offset_y;
 //   Texture2D *result = new Texture2D(size * 3,size * 2,TEXEL_TYPE_DEPTH);
    Image2D *image_data;
    Image2D *result_image_data = result->get_image_data();
    int texture_direction;
    
    for (texture_direction = 0; texture_direction < 6; texture_direction++)
      {
        switch (texture_direction)
          {
            case 0:
              image_data = cubemap->get_texture_distance()->image_front;
              offset_x = 0; offset_y = 0;
              break;
              
            case 1:
              image_data = cubemap->get_texture_distance()->image_back;
              offset_x = size; offset_y = 0;
              break;
              
            case 2:
              image_data = cubemap->get_texture_distance()->image_left;
              offset_x = 2 * size; offset_y = 0;
              break;
              
            case 3:
              image_data = cubemap->get_texture_distance()->image_right;
              offset_x = 0; offset_y = size;
              break;
              
            case 4:
              image_data = cubemap->get_texture_distance()->image_top;
              offset_x = size; offset_y = size;
              break;
              
            case 5:
            default:
              image_data = cubemap->get_texture_distance()->image_bottom;
              offset_x = 2 * size; offset_y = size;
              break;
          }
          
        float r, g, b, a, maximum, minimum;  
        int i,j,x,y,level,block_size,level_offset;
        
        level = 1;
        level_offset = 0;
        
        while (true)  // levels
          {
            block_size = size / level;
            
            if (block_size < 2)
              break;
            
            for (j = 0; j < level; j++)
              for (i = 0; i < level; i++)
                {
                  maximum = 0;
                  minimum = 99999999999;
                  
                  for (y = 0; y < block_size; y++)
                    for (x = 0; x < block_size; x++)
                      {
                        image_data->get_pixel(i * block_size + x,j * block_size + y,&r,&g,&b,&a);
                        
//r = (2 * NEAR) / (FAR + NEAR - r * (FAR - NEAR)) * (FAR - NEAR);     // linearizes depth                       
                        
                        if (r > maximum)
                          maximum = r;
                        
                        if (r < minimum)
                          minimum = r;
                      }  
                
                   result_image_data->set_pixel(offset_x + i + level_offset,offset_y + j,maximum,0,0,0);
                   result_image_data->set_pixel(offset_x + i + level_offset,offset_y + j + half_size,minimum,0,0,0);
                }
                
            level_offset += level;
            level *= 2;
          }
        
      /*  
        for (j = 0; j < size; j++)
          for (i = 0; i < size; i++)
            {
              image_data->get_pixel(i,j,&r,&g,&b,&a);
              result_image_data->set_pixel(i + offset_x,j + offset_y,r,0,0,0);
            } */
      }

 //   result->set_parameter_int(GL_TEXTURE_MIN_FILTER,GL_NEAREST);
 //   result->set_parameter_int(GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    result->update_gpu();    
  }
  
void save_images()
  {
    cout << "saving images" << endl;
    
    texture_camera_color->load_from_gpu();
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
  
    cubemaps[0]->get_texture_color()->load_from_gpu();  
    cubemaps[0]->get_texture_depth()->load_from_gpu();
    cubemaps[0]->get_texture_distance()->load_from_gpu();  
    cubemaps[0]->get_texture_color()->save_ppms("cubemap_images/cubemaps[0]");
    cubemaps[0]->get_texture_distance()->save_ppms("cubemap_images/cubemaps0_distance");
    cubemaps[0]->get_texture_depth()->raise_to_power(256);  
    cubemaps[0]->get_texture_depth()->save_ppms("cubemap_images/cubemaps0_depth");
    
    cubemaps[1]->get_texture_color()->load_from_gpu();  
    cubemaps[1]->get_texture_depth()->load_from_gpu();
    cubemaps[1]->get_texture_color()->save_ppms("cubemap_images/cubemaps1");
    cubemaps[1]->get_texture_depth()->raise_to_power(256);  
    cubemaps[1]->get_texture_depth()->save_ppms("cubemap_images/cube_map1_depth"); 
  }
  
void recompute_cubemap()
  {
    set_up_pass1();
    uniform_rendering_cubemap.update_int(1);
    
    uniform_projection_matrix.update_mat4(ReflectionTraceCubeMap::get_projection_matrix());
    
    uniform_cubemap_position.update_vec3(cubemaps[0]->transformation.get_translation());
    cubemaps[0]->setViewport();
    cout << "rendering cube map 1..." << endl;
    recompute_cubemap_side(cubemaps[0],GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    recompute_cubemap_side(cubemaps[0],GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
    recompute_cubemap_side(cubemaps[0],GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
    recompute_cubemap_side(cubemaps[0],GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
    recompute_cubemap_side(cubemaps[0],GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
    recompute_cubemap_side(cubemaps[0],GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
    cubemaps[0]->unsetViewport();  
    
    uniform_cubemap_position.update_vec3(cubemaps[1]->transformation.get_translation());
    cubemaps[1]->setViewport();
    cout << "rendering cube map 2..." << endl;
    recompute_cubemap_side(cubemaps[1],GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    recompute_cubemap_side(cubemaps[1],GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
    recompute_cubemap_side(cubemaps[1],GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
    recompute_cubemap_side(cubemaps[1],GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
    recompute_cubemap_side(cubemaps[1],GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
    recompute_cubemap_side(cubemaps[1],GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
    cubemaps[1]->unsetViewport();  

    cubemaps[0]->get_texture_color()->load_from_gpu();  
    cubemaps[0]->get_texture_depth()->load_from_gpu();
    cubemaps[0]->get_texture_distance()->load_from_gpu();      
    cubemaps[1]->get_texture_color()->load_from_gpu();  
    cubemaps[1]->get_texture_depth()->load_from_gpu();
    
    //save_images();
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
          
          for (int i = 0; i < 2; i++)
            {
              create_acceleration_texture_sw(cubemaps[i],acceleration_textures[i]);
           //   acceleration_textures[i]->get_image_data()->raise_to_power(256);
              acceleration_textures[i]->update_gpu();
            }
          
          cout << "acceleration max: " << acceleration_textures[0]->get_max_value() << endl;
          
       //   acceleration_textures[0]->get_image_data()->save_ppm("cubemap_images/acceleration.ppm");
       //   acceleration_textures[1]->get_image_data()->save_ppm("cubemap_images/acceleration2.ppm");
       //   save_images();
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
    
    CameraHandler::camera_transformation.set_translation(glm::vec3(-78.2378,90.4943,-118.219));
    CameraHandler::camera_transformation.set_rotation(glm::vec3(-0.37875,-8.60999,0));
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
    
    Geometry3D g3 = load_obj("scene.obj");
    geometry_scene = &g3;
    geometry_scene->update_gpu();
    
    Geometry3D g4 = make_box_sharp(6,6,6);
    geometry_sky = &g4;
    geometry_sky->update_gpu();
    
    //Geometry3D g5 = load_obj("teapot.obj");
    Geometry3D g5 = make_box_sharp(3,3,3);
    
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

    for (int i = 0; i < 2; i++)
      {
        acceleration_textures[i] = new Texture2D(CUBEMAP_RESOLUTION * 3,CUBEMAP_RESOLUTION * 2,TEXEL_TYPE_COLOR); //new_acceleration_texture(CUBEMAP_RESOLUTION);
        acceleration_textures[i]->update_gpu();
      }
    
    cubemaps[0] = new ReflectionTraceCubeMap(CUBEMAP_RESOLUTION,"cubemaps[0].texture_color","cubemaps[0].texture_distance","cubemaps[0].position",4,5);
    cubemaps[0]->update_gpu();
    
    cubemaps[1] = new ReflectionTraceCubeMap(CUBEMAP_RESOLUTION,"cubemaps[1].texture_color","cubemaps[1].texture_distance","cubemaps[1].position",6,7);
    cubemaps[1]->update_gpu();
    
    ErrorWriter::checkGlErrors("cube map init",true);
      
    cubemaps[0]->transformation.set_translation(glm::vec3(0.0,30.0,-30.0));
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
    Shader shad2(file_text("shader_quad.vs",true),file_text("shader_quad.fs",true),"");
    
    shader_3d = &shad1;
    shader_quad = &shad2;
    
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
    uniform_camera_position.retrieve_location(shader_quad);
    uniform_textures_acceleration_0.retrieve_location(shader_quad);
    uniform_textures_acceleration_1.retrieve_location(shader_quad);
    
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
    delete cubemaps[0];
    delete cubemaps[1];
    delete acceleration_textures[0];
    delete acceleration_textures[1];
    delete texture_scene;
    delete texture_sky;
    delete texture_mirror_depth;
    delete profiler;
    GLSession::clear();
    return 0;
  }