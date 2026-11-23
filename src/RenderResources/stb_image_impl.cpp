// Single implementation TU for stb_image. stb is a single-header library: the declarations come from
// `stb_image.h` in the `thirdparty/stb` submodule (nothings/stb), and exactly one translation unit must
// define STB_IMAGE_IMPLEMENTATION to instantiate the function definitions. This file is compiled into
// `muyo_renderer` by the `src/RenderResources/*.cpp` glob.
//
// stb_image_write is not used by this repository; only `stbi_load` is called.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
