This repository contains experiments with new algorithms for rendering
non-planar mirrors using OpenGL. It is a part of Master's Thesis
project at FIT BUT.

The repository structure is following:

  - gl_wrapper.h: Header-only library for simple work with OpenGL and
    resources, made specifically for this project.
  - shader_log_include.txt: Helper shader code to allow logging from
    GLSL.
  - resources: Resources (models, textures, ...) used in the project.
  - hello wrapper: Hello world made with gl_wrapper.
  - planar mirror: Simple planar mirror rendering, implemented with
    gl_wrapper.
  - environment mapping: Implementation of cubemap-based environment
    mapping with gl_wrapper.
  - compute shader: Example code for work with compute shaders.
  - cubemap: Implementation of the new non-planar mirror rendering
    algorithm. Run ./main -h for more info.

Everything can be built with "make". The program was only tested on
Ubuntu Linux. Dependecies of the project (gl_wrapper.h library) are:

  - OpenGL 4.5
  - freeglut
  - glew
  - glm

The main part of the project is located in "cubemap" folder. To get more
info, make the program and run "./main -h".

The repository can be found at GitHub: https://github.com/drummyfish/mirrors.
All code is provided under GPL license.

Miloslav Číž, 2017
