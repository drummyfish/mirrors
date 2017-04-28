This repository contains experiments with new algorithms for rendering
non-planar mirrors using OpenGL. What follows is a clarification of
the project structure:

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
