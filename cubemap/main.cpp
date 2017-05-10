/*
 * OpenGL implementation of cubemap tracing algorithm for rendering
 * nonplanar mirrors. This has been done as a part of master's thesis
 * at FIT BUT, 2017.
 * 
 * author: Miloslav Číž
 */

#define NO_UNIFORM_UPDATE_ERRORS

#include "../gl_wrapper.h"

#define CAMERA_STEP 0.1
#define ROTATION_STEP 0.1

#define CAMERA_POSITION 14.8467, 49.5787, -46.4311
#define CAMERA_ROTATION -0.547712, -3.70633, 0

#define NEAR 0.01f
#define FAR 1000.0f
//#define SHADER_LOG
#define MEASURE_TIME_S 6

// global flags and parameters, set these with command line parameters:

bool use_compute_shaders = false;
bool self_reflections = false;
bool save_debug_images = false;
bool help = false;
bool profiling = false;
bool software = false;
bool measure = false;
bool fill_unresolved = false;
bool efficient = false;
bool analytical = false;

string shader_defines = "";              // defines inserted into shaders

unsigned int cubemap_resolution = 256;
unsigned int reflector = 0;
unsigned int scene = 0;
unsigned int window_width = 640;
unsigned int window_height = 480;

// ---------------------------

unsigned int measure_start_time_s;
unsigned int cubemap_rendering_time;
unsigned int acc_recompute_time;

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

Shader *shader_compute;              
Shader *shader_quad2;                // only draws a texture modified by compute shader
UniformVariable uniform_texture_color2("texture_color");

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

//-------------- compute shader related definitions --------------

typedef struct
  {
    // screen position
    GLuint x;                    // 4 bytes
    GLuint y;                    // 4 bytes
    // ray parameters
    glm::vec3 ray_position1;     // 12 bytes
    glm::vec3 ray_position2;     // 12 bytes
  } mirror_pixel;

typedef struct
  {
    GLuint number_of_pixels;
    mirror_pixel *pixels;
  } mirror_pixels_info;
  
StorageBuffer* pixel_storage_buffer;    // stores pixels for compute shader
mirror_pixels_info *mirror_pixels;      // will be integrated with pixel_storage_buffer

//---------------------------------------------------------------

void print_info()
  {
    if (!profiling && !measure)
      return;
        
    profiler->print();
    cout << "approximate time/mirror pixel (us): " <<  (1000.0 * profiler->get_average_value(1) / profiler->get_average_value(2)) << endl;
    profiler->reset();
    
    cout << "last (all) cubemap rendering (ms): " << cubemap_rendering_time << endl;
    cout << "last (all) acc. struct. recompute (ms): " << acc_recompute_time << endl;
    
    if (!measure)
      {
        cout << "-----" << endl;
    
        cout << "camera position: ";
        print_vec3(CameraHandler::camera_transformation.get_translation());
   
        cout << "camera rotation: ";
        print_vec3(CameraHandler::camera_transformation.get_rotation()); 
  
        cout << "cube1 position: ";
        print_vec3(cubemaps[0]->transformation.get_translation());
    
        cout << "camera to cube1: ";
        print_vec3(glm::normalize(cubemaps[0]->transformation.get_translation() - CameraHandler::camera_transformation.get_translation()));
      }    

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
      
    if (self_reflections)
      {
        // mirror has to be always drawn for self reflections
        uniform_model_matrix.update_mat4(transformation_mirror.get_matrix());
        uniform_mirror.update_int(1);
        geometry_mirror->draw_as_triangles();
        uniform_mirror.update_int(0);
      }
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
    
    if (! measure && info_countdown < 0)
      {
        info_countdown = 32;
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
   
    if (use_compute_shaders)
      {
        pixel_storage_buffer->bind();   
        pixel_storage_buffer->clear();
        pixel_storage_buffer->update_gpu();
      }

    profiler->time_measure_begin(); 
    set_up_pass2();
    draw_quad();
    
    if (use_compute_shaders)
      {
        // third pass with compute shader
      
        pixel_storage_buffer->load_from_gpu();
        cout << mirror_pixels->number_of_pixels << endl; 
    
        shader_compute->use();
      
        texture_camera_color->bind_image(0);
      
        cubemaps[0]->get_texture_color()->bind_image(1,0,GL_READ_ONLY);
      
        cubemaps[0]->get_texture_color()->bind_image(2,0,GL_READ_ONLY);
        cubemaps[0]->get_texture_color()->bind_image(3,1,GL_READ_ONLY);
        cubemaps[0]->get_texture_color()->bind_image(4,2,GL_READ_ONLY);
        cubemaps[0]->get_texture_color()->bind_image(5,3,GL_READ_ONLY);
      
        shader_compute->run_compute(100/* pix / 8*/,1,1);
        
        shader_quad2->use();
        texture_camera_color->bind(0);
        uniform_texture_color2.update_int(0);
        draw_quad();
      }
   
    #ifdef SHADER_LOG    
      shader_log->load_from_gpu();
      shader_log->print();
      shader_log->clear();
      shader_log->update_gpu();
    #endif
    
    profiler->record_value(1,profiler->time_measure_end());
    
    ErrorWriter::checkGlErrors("rendering loop");  
    glutSwapBuffers();

    profiler->next_frame();
    
    if (measure && profiler->get_cpu_seconds() - measure_start_time_s >= MEASURE_TIME_S)
      {
        print_info();
        glutLeaveMainLoop();
      }
  }
  
void recompute_cubemap_side(ReflectionTraceCubeMap *cube_map, GLuint side) 
  {
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
    if (!save_debug_images)
      return;
        
    cout << "saving images" << endl;

    double coeff = 0.01;

    int cube_index = 0;
    cubemaps[cube_index]->get_texture_normal()->save_ppms("cubemap_images/cubemap_normal");
    cubemaps[cube_index]->get_texture_color()->save_ppms("cubemap_images/cubemap_color");

    for (unsigned int mip_level = 0; mip_level < cubemaps[cube_index]->get_texture_distance()->get_number_of_mipmap_levels(); mip_level++)
      {
        cubemaps[cube_index]->get_texture_distance()->set_mipmap_level(mip_level);
        cubemaps[cube_index]->get_texture_distance()->load_from_gpu();
        cubemaps[cube_index]->get_texture_distance()->multiply(coeff);
        cubemaps[cube_index]->get_texture_distance()->save_ppms("cubemap_images/acc/cubemap_distance_mip" + std::to_string(mip_level));
      }
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

void recompute_all()
  {      
    cout << "rendering cubemaps..." << endl;
    
    profiler->time_measure_begin();
    recompute_cubemap();
    cubemap_rendering_time = profiler->time_measure_end();
    
    cout << "recomputing acceleration structures..." << endl;
    
    profiler->time_measure_begin();

    if (use_compute_shaders)
      {
        cubemaps[0]->compute_cs_acceleration_texture();
        cubemaps[1]->compute_cs_acceleration_texture();
      }
    else
      {
        if (software)
          {
            cubemaps[0]->compute_acceleration_texture_sw();
            cubemaps[1]->compute_acceleration_texture_sw();
          }
        else
          {
            cubemaps[0]->compute_acceleration_texture();
            cubemaps[1]->compute_acceleration_texture();
          }
      }
      
    acc_recompute_time = profiler->time_measure_end();
    
    ErrorWriter::checkGlErrors("acceleration structure recompute",true);
          
    save_images();    
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
          recompute_all();
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
          
        case GLUT_KEY_F9:
          CameraHandler::camera_transformation.set_translation(glm::vec3(CAMERA_POSITION));
          CameraHandler::camera_transformation.set_rotation(glm::vec3(CAMERA_ROTATION));
          break;
          
        case GLUT_KEY_F11:
          cubemaps[0]->transformation.set_translation(CameraHandler::camera_transformation.get_translation());
          cubemaps[0]->transformation.add_translation(CameraHandler::camera_transformation.get_direction_forward() * 5.0f);
          recompute_all();
          break;
          
        case GLUT_KEY_F12:
          cubemaps[1]->transformation.set_translation(CameraHandler::camera_transformation.get_translation());
          cubemaps[1]->transformation.add_translation(CameraHandler::camera_transformation.get_direction_forward() * 5.0f);
          recompute_all();
          break;
          
        default:
          break;
      }
  }
  
void handle_args(int argc, char **argv)
  {
    for (int i = 1; i < argc; i++)
      {
        if (strcmp(argv[i],"-h") == 0)
          {
            cout << "nonplanar mirror reflections" << endl;
            cout << "Miloslav Číž, 2017" << endl << endl;
            cout << "This program experiments with a new algorithm for rendering mirrors." << endl << endl;
            cout << "controls:" << endl;
            
            cout << "WSAD                  move" << endl;
            cout << "PgUp/PgDn/Home/End    rotate mirror" << endl;
            cout << "arrow keys            move mirror" << endl;
            cout << "insert                recompute cubemaps" << endl;
            cout << "F11, F12              move 1st/2nd cubemap" << endl << endl;
            cout << "F8                    turn acceleration off/on" << endl;
            cout << "F1                    render reflections" << endl;
            cout << "F2                    render normals" << endl;
            cout << "F3                    render position" << endl;
            cout << "F4                    render stencil" << endl;
            cout << "F5                    render iterations" << endl;
            cout << "F9                    reset camera" << endl;
            
            cout << "command line arguments:" << endl; 
            cout << "-f        fill unresolved intersections with env. mapping" << endl;
            cout << "-e        efficient sampling" << endl;
            cout << "-a        analytical intersections" << endl;
            cout << "-c        compute shaders" << endl;
            cout << "-s        self reflections" << endl;
            cout << "-i        save debug images" << endl;
            cout << "-p        profiling and other info" << endl;
            cout << "-s        use SW for acc computation" << endl;
            cout << "-n        no acceleration" << endl;
            cout << "-m        measure performance" << endl;
            cout << "-WN       set different window resolutions, N = 0 ... 3" << endl;
            cout << "-CN       set cubemap resolution (non-cs only), N = 0 .. 3 " << endl;
            cout << "-MN       mirror geometry model, N = 0 .. 2 " << endl;
            cout << "-SN       scene model, N = 0 .. 2" << endl;
            
            help = true;
          }
        else if (strcmp(argv[i],"-f") == 0)
          {
            fill_unresolved = true;
            shader_defines += "#define FILL_UNRESOLVED\n";
          }
        else if (strcmp(argv[i],"-e") == 0)
          {
            efficient = true;
            shader_defines += "#define EFFICIENT_SAMPLING\n";
          }
        else if (strcmp(argv[i],"-a") == 0)
          {
            analytical = true;
            shader_defines += "#define ANALYTICAL_INTERSECTION\n";
            shader_defines += "#define USE_ACCELERATION_LEVELS 6\n";
          }
        else if (strcmp(argv[i],"-c") == 0)
          {
            use_compute_shaders = true;
            shader_defines += "#define COMPUTE_SHADER\n";
            cubemap_resolution = 1024;
          }
        else if (strcmp(argv[i],"-s") == 0)
          {
            self_reflections = true;
            shader_defines += "#define SELF_REFLECTIONS\n";
          }
        else if (strcmp(argv[i],"-s") == 0)
          {
            software = true;
          }
        else if (strcmp(argv[i],"-i") == 0)
          {
            save_debug_images = true;
          }
        else if (strcmp(argv[i],"-p") == 0)
          {
            profiling = true;
          }
        else if (strcmp(argv[i],"-W0") == 0)
          {
            window_width = 320; window_height = 240;
          }
        else if (strcmp(argv[i],"-W1") == 0)
          {
            // leave default
          }
        else if (strcmp(argv[i],"-W2") == 0)
          {
            window_width = 800; window_height = 600;
          }
        else if (strcmp(argv[i],"-W3") == 0)
          {
            window_width = 1024; window_height = 768;
          }
        else if (strcmp(argv[i],"-C0") == 0)
          {
            cubemap_resolution = 128;
          }
        else if (strcmp(argv[i],"-C1") == 0)
          {
            // leave default
          }
        else if (strcmp(argv[i],"-C3") == 0)
          {
            cubemap_resolution = 512;
          }
        else if (strcmp(argv[i],"-n") == 0)
          {
            acceleration_on = 0;
          }
        else if (strcmp(argv[i],"-C4") == 0)
          {
            cubemap_resolution = 1024;
          }
        else if (strcmp(argv[i],"-M0") * strcmp(argv[i],"-M1") * strcmp(argv[i],"-M2") == 0)
          {
            reflector = glm::min(2,glm::max(0,argv[i][2] - '0'));
          }
        else if (strcmp(argv[i],"-S0") * strcmp(argv[i],"-S1") * strcmp(argv[i],"-S2") == 0)
          {
            scene = glm::min(2,glm::max(0,argv[i][2] - '0'));
          }
        else if (strcmp(argv[i],"-m") == 0)
          {
            measure = true;
          }
        else
          {
            cout << "unrecognized option: " << argv[i] << ", ignoring" << endl;
          }
      }
  }
  
int main(int argc, char** argv)
  { 
    // parse command line arguments:
    
    handle_args(argc,argv);
    
    if (help)
      return 0;

    if (measure)
      ErrorWriter::enabled = false;
    
    cout << "window resolution: " << window_width << " x " << window_height << endl;
    cout << "cubemap resolution: " << cubemap_resolution << endl;
    cout << "use compute shaders: " << use_compute_shaders << endl;
    cout << "self reflections: " << self_reflections << endl;
    cout << "efficient sampling: " << efficient << endl;
    cout << "software: " << software << endl;
    cout << "acceleration: " << acceleration_on << endl;
    cout << "analytical intersection: " << analytical << endl;
    cout << "scene:" << scene << endl; 
    cout << "--------" << endl;

    GLSession *session;
    session = GLSession::get_instance();
    session->keyboard_callback = CameraHandler::key_callback;
    session->mouse_callback = CameraHandler::mouse_click_callback;
    session->mouse_pressed_motion_callback = CameraHandler::mouse_move_callback;
    session->mouse_not_pressed_motion_callback = CameraHandler::mouse_move_callback;
    session->special_callback = special_callback;
    session->window_size[0] = window_width;
    session->window_size[1] = window_height;
    session->init(render);
    
    profiler = new Profiler();
    profiler->new_value("pass 1");
    profiler->new_value("pass 2");
    profiler->new_value("mirror fragments");
    
    CameraHandler::camera_transformation.set_translation(glm::vec3(CAMERA_POSITION));
    CameraHandler::camera_transformation.set_rotation(glm::vec3(CAMERA_ROTATION));
    
    CameraHandler::translation_step = 2.0;
    
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
    
    Geometry3D g4 = make_box_sharp(6,6,6);
    g4.flip_triangles();
    geometry_sky = &g4;
    geometry_sky->update_gpu();
    
    Geometry3D g5;
    
    switch (reflector)
      {
        case 0: g5 = load_obj("../resources/teapot.obj"); break;
        case 1: g5 = make_box_sharp(1,1,1); break;
        case 2: g5 = load_obj("../resources/self_reflection_test.obj"); break;
        default: break;
      }
      
    geometry_mirror = &g5;
    geometry_mirror->update_gpu();
    
    texture_sky = new Texture2D(16,16,TEXEL_TYPE_COLOR);
    texture_sky->load_ppm("../resources/sky.ppm");
    texture_sky->update_gpu();
    
    texture_scene = new Texture2D(16,16,TEXEL_TYPE_COLOR);
    
    Geometry3D g3;
    
    switch (scene)
      {
        case 0:
          g3 = load_obj("../resources/scene.obj");
          texture_scene->load_ppm("../resources/scene.ppm");
          transformation_scene.set_translation(glm::vec3(0.0,0.0,-7.0));
          transformation_scene.set_scale(glm::vec3(6,6,6));
          break;

        case 1:
          g3 = load_obj("../resources/sponza simple.obj");
          texture_scene->load_ppm("../resources/sponza simple.ppm");
          transformation_scene.set_translation(glm::vec3(60.0,2.0,-30.0));
          transformation_scene.set_scale(glm::vec3(60,60,60));
          break;

        case 2:
        default:
          g3 = load_obj("../resources/library.obj");
          texture_scene->load_ppm("../resources/library.ppm");
          transformation_scene.set_translation(glm::vec3(0.0,2.0,-30.0));
          transformation_scene.set_rotation(glm::vec3(0,3.14,0));
          transformation_scene.set_scale(glm::vec3(40,40,40));
          break;
      }
 
    geometry_scene = &g3;
    geometry_scene->update_gpu();
    
    texture_scene->update_gpu();

    texture_camera_color = new Texture2D(window_width,window_height,TEXEL_TYPE_COLOR);
    texture_camera_color->update_gpu();
    
    texture_camera_depth = new Texture2D(window_width,window_height,TEXEL_TYPE_DEPTH);
    texture_camera_depth->update_gpu();
    
    texture_camera_position = new Texture2D(window_width,window_height,TEXEL_TYPE_COLOR);
    texture_camera_position->update_gpu();
    
    texture_camera_normal = new Texture2D(window_width,window_height,TEXEL_TYPE_COLOR);
    texture_camera_normal->update_gpu();
    
    texture_camera_stencil = new Texture2D(window_width,window_height,TEXEL_TYPE_COLOR);  // couldn't get stencil texture to work => using color instead
    texture_camera_stencil->update_gpu();
    
    cubemaps[0] = new ReflectionTraceCubeMap(cubemap_resolution,"cubemaps[0].texture_color","cubemaps[0].texture_distance","cubemaps[0].texture_normal","cubemaps[0].position",4,5,6);
    cubemaps[0]->update_gpu();
    
    cubemaps[1] = new ReflectionTraceCubeMap(cubemap_resolution,"cubemaps[1].texture_color","cubemaps[1].texture_distance","cubemaps[1].texture_normal","cubemaps[1].position",7,8,9);
    cubemaps[1]->update_gpu();
    
    ErrorWriter::checkGlErrors("cube map init",true);
      
    cubemaps[0]->transformation.set_translation(glm::vec3(10.6235,41.3533,-47.5263));
    cubemaps[1]->transformation.set_translation(glm::vec3(-18,35,-22));
    
    transformation_sky.set_scale(glm::vec3(100.0,100.0,100.0));
    
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
    
    Shader shad1(file_text("shader_3d.vs",true,""),file_text("shader_3d.fs",true,""),"");
    Shader shad2(VERTEX_SHADER_QUAD_TEXT,file_text("shader_quad.fs",true,shader_defines),"",0,false);
    
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
    
    
    shader_quad->use();
    
    cubemaps[0]->update_uniforms();
    cubemaps[1]->update_uniforms();
    uniform_texture_color.update_int(0);
    uniform_texture_normal.update_int(1);
    uniform_texture_position.update_int(2);
    uniform_texture_stencil.update_int(3);    
    
    shader_quad->validate();    // delayed validation, for AMD GPUs
    
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
    
    recompute_all();   // compute the cubemaps
    
    ErrorWriter::checkGlErrors("shader log init",true);
    
    if (use_compute_shaders)
      {
        pixel_storage_buffer = new StorageBuffer(sizeof(mirror_pixel) * window_width * window_height,1);
        mirror_pixels = (mirror_pixels_info *) pixel_storage_buffer->get_data_pointer();
    
        shader_compute = new Shader("","",file_text("shader.cs",true,""));
    
        Shader shad3(VERTEX_SHADER_QUAD_TEXT,file_text("shader_quad2.fs",true,""),"");
        shader_quad2 = &shad3;
        uniform_texture_color2.retrieve_location(shader_quad2);

        ErrorWriter::checkGlErrors("compute shader init",true);
      }

    measure_start_time_s = profiler->get_cpu_seconds();
      
    session->start();
    
    if (use_compute_shaders)
      {
        delete pixel_storage_buffer;
        delete shader_compute;
      }
    
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
