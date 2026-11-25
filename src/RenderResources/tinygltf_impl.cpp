// Single implementation TU for tinygltf, the counterpart of stb_image_impl.cpp beside it.
//
// Upstream's `tiny_gltf.cc` is three lines: TINYGLTF_IMPLEMENTATION *and* STB_IMAGE_IMPLEMENTATION and
// STB_IMAGE_WRITE_IMPLEMENTATION. Those last two instantiate a second full copy of stb_image inside
// the tinygltf library, which collided with stb_image_impl.cpp at link time - measured as LNK1169 plus
// one LNK2005 per exported symbol (`stbi_load`, `stbi_info`, ...) when helloVulkan pulled tiny_gltf.obj
// in through SceneImporter. stb is a single-header library and this repository keeps exactly one
// implementation TU, so the definitions stay here and tinygltf only gets the declarations from
// `stb_image.h`. stb_image_write is not used by this repository at all.
//
// Muyo's own copy of `tiny_gltf.cc` had the same two defines removed. Doing it here rather than in the
// submodule is the choice already made for imgui, which gets a project-local `ImGuiUserConfig.h`
// instead of a patched submodule.
//
// Compiled into `muyo_renderer` by the `src/RenderResources/*.cpp` glob, so there is no separate
// `tinygltf` library target to keep in step.
// `stb_image_impl.cpp` beside this file instantiates both stb_image and stb_image_write, which is what
// tinygltf's WriteImageData links against. (Upstream offers TINYGLTF_NO_STB_IMAGE_WRITE, but this
// version declares `tinygltf::WriteImageData` unconditionally and references it from TinyGLTF's
// constructor, so the define trades three unresolved stbi_write_* symbols for one unresolved
// WriteImageData - measured.) Muyo's `stb` target likewise implemented the writers; nothing calls them.
#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"
