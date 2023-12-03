Vulkan Mesh Shader with Free-Form Planar Area Lights
==================================

**University of Pennsylvania, CIS 565: GPU Programming and Architecture, Fianl Project**

* Licheng Cao, Tianyi Xiao, Mengxuan Huang

## Description

In this project, we use Vulkan to build mesh shader render pipelines, which render scenes illuminated by free-form planar area lights.

![](/img/result.png)

## Mesh Shader

Different from traditional render pipeline, we used task shaders (optional) and mesh shader instead of vertex shaders, tessellation shaders and geometry shaders. Both task shaders and mesh shaders operate in workgroups, which are like compute shaders. Task shaders emit mesh shader workgroups and mesh shaders generate primitives for rasterization and fragment shaders.

![](/img/mesh_shader.png)

What's more, we generate meshlets and pass them to mesh shader on CPU. We will have two index buffers to be passed, one for only vertex indices using uint, with their indices in original complete model data, and another one for triangle primitive indices with local indices per meshlet. The second index buffers use uint8_t since there won't be too many indices in each meshlet. Therefore we save the much memory bandwith.

![](/img/meshlet.png)

As shown above, when rendering this robot model, we got 37.4% improvement in memory with meshlet.

## Free-Form Area Light

The real time shading of free-form planar area lights was inspired by [a JCGT paper](https://jcgt.org/published/0011/01/01/), which is based on a previous siggraph paper.

## Third Party Credit
-[Real-Time Shading of Free-Form Area Lights using Linearly Transformed Cosines](https://jcgt.org/published/0011/01/01/)

-[Real-Time Polygonal-Light Shading with Linearly Transformed Cosines](https://eheitzresearch.wordpress.com/415-2/)

-[Introduction to Turing Mesh Shaders](https://developer.nvidia.com/blog/introduction-turing-mesh-shaders/)

### Third party Resources
- [Subway Station & R46 Subway](https://sketchfab.com/3d-models/free-subway-station-r46-subway-ae5aadde1c6f48a19b32b309417a669b)