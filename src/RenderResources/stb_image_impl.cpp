// Single implementation TU for stb. stb is a single-header library: the declarations come from
// `stb_image.h` / `stb_image_write.h` in the `thirdparty/stb` submodule (nothings/stb), and exactly one
// translation unit must define the IMPLEMENTATION macros to instantiate the function definitions. This
// file is compiled into `muyo_renderer` by the `src/RenderResources/*.cpp` glob.
//
// stb_image_write is not called by this repository, but tinygltf's WriteImageData references it and
// Muyo's `stb` target implemented both, so both are instantiated here - one implementation TU rather
// than two, which is what the link needs.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
