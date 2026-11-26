# Capturing the renderer with RenderDoc

Two findings, both measured and version-bound. Record the RenderDoc version with any future capture;
"capture does not work" is a statement about a RenderDoc build, not about the application.

## 1. A Wayland session must capture through GLFW's X11 backend (Xwayland)

RenderDoc **1.46** (`e4bd23b6`) under its capture launcher stops advertising loader-provided instance
extensions: `VK_KHR_wayland_surface`, `VK_EXT_layer_settings`, `VK_KHR_portability_enumeration` and
`VK_LUNARG_direct_driver_loading` disappear, while the ICD-provided `VK_KHR_xcb_surface` /
`VK_KHR_xlib_surface` remain. GLFW then selects Wayland, `glfwGetRequiredInstanceExtensions` returns
NULL, and `helloVulkan` aborts before Vulkan starts.

Hiding `WAYLAND_DISPLAY` makes GLFW select Xwayland, which is the one combination that captures on a
Wayland session ([muyo-renderer#15](https://github.com/v3c70r/muyo-renderer/issues/15), transferred from Muyo#220)). By hand, with loader 1.4.313 on
RADV REMBRANDT:

```sh
env -u WAYLAND_DISPLAY renderdoccmd capture -c /tmp/x.rdc "$PWD/build/helloVulkan"
```

`renderdoccmd` execs the program argument directly and searches neither the cwd nor `PATH`, so pass an
absolute path. Muyo's `scripts/capture.sh` does that resolution and the `WAYLAND_DISPLAY` unset in one
step and accepts any program:

```sh
scripts/capture.sh -o /tmp/x.rdc build/helloVulkan
```

GLFW must have its X11 backend. A fresh configure of the vendored GLFW builds both backends; an
existing build tree created before #220 keeps its cached `GLFW_BUILD_X11=OFF` until reconfigured with
`-DGLFW_BUILD_X11=ON`. On a machine without Wayland development packages, configure with
`-DGLFW_BUILD_WAYLAND=OFF`; the X11 path needs `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`
and `libxi-dev`.

**Wayland capture with RenderDoc 1.46 does not work on this machine.** That is the capture layer's
behaviour, not the application's: without the loader extension there is no Wayland surface to create.
Do not re-investigate it without a newer RenderDoc.

## 2. RenderDoc 1.46/1.47 filter device extensions they do not support

RenderDoc's layer filters `vkEnumerateDeviceExtensionProperties` to the list in its
`extension_support.md`, which marks both `VK_EXT_descriptor_heap` and
`VK_KHR_shader_untyped_pointers` as unsupported in 1.47 as well
([upstream #3781](https://github.com/baldurk/renderdoc/issues/3781)).

Measured with a probe linked against the same loader, under the capture launcher: AMD Radeon Graphics
(RADV REMBRANDT) reports API 1.4.0 with 203 device extensions and **neither** extension; the same probe
without RenderDoc reports API 1.4.354 with 231 extensions and both present.

So the X11 recipe above removes the WSI blocker, but the graph applications still stop at
`CreateDevice` with:

```
does not support the requested extension: VK_EXT_descriptor_heap
```

Capture of the descriptor-heap path waits on RenderDoc support
([Muyo#303](https://github.com/v3c70r/Muyo/issues/303)). Nsight Graphics 2026.2 is the alternative that
advertises both extensions.
