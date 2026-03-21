# MaskedOcclusionCulling

This code accompanies the research paper ["Masked Software Occlusion Culling"](https://software.intel.com/en-us/articles/masked-software-occlusion-culling),
and implements an efficient alternative to the hierarchical depth buffer algorithm. Our algorithm decouples depth values and coverage, and operates directly
on the hierarchical depth buffer. It lets us efficiently parallelize both coverage computations and hierarchical depth buffer updates.

## Update May 2018

Added the ability to merge 2 depth buffers, this allows both an alterative method for parallelizing buffer creation and a way to reduce silhouette bleed when input data cannot be roughly sorted from front to back, for example rendering large terrain patches with foreground occluders in an open world game engine.

## Requirements

This code is mainly optimized for AVX capable CPUs. However, we also provide SSE 4.1 and SSE 2 implementations for backwards compatibility. The appropriate 
implementation will be chosen during run-time based on the CPU's capabilities.

## Notes on build time

The code is optimized for runtime performance and may require a long time to compile due to heavy code inlining. This can be worked around by compiling 
a library file. An alternative solution is to disable *whole program optimizations* for the `MaskedOcclusionCulling.cpp`, 
`MaskedOcclusionCullingAVX2.cpp` and `MaskedOcclusionCullingAVX512.cpp` files. It does not impact runtime performance, but greatly reduces the time of program linking. 

## <a name="cs"></a>Notes on coordinate systems and winding

Most inputs are given as clip space (x,y,w) coordinates assuming the same right handed coordinate system as used by DirectX and OpenGL (x positive right, y
positive up and w positive in the view direction). Note that we use the clip space w coordinate for depth and disregard the z coordinate. Internally our
masked hierarchical depth buffer stores *depth = 1 / w*. 

The `TestRect()` function is an exception and instead accepts normalized device coordinates (NDC), *(x' = x/w, y' = y/w)*, where the visible screen region
maps to the range [-1,1] for *x'* and *y'* (x positive right and y positive up). Again, this is consistent with both DirectX and OpenGL behavior.

By default, the screen space coordinate system used internally to access our hierarchical depth buffer follows DirectX conventions (y positive down), which is
**not** consistent with OpenGL (y positive up). This can be configured by changing the `USE_D3D` define. The screen space coordinate system affects the layout
of the buffer returned by the `ComputePixelDepthBuffer()` function, scissor rectangles (which are specified in screen space coordinates), and rasterization
tie-breaker rules if `PRECISE_COVERAGE` is enabled.

## API / Tutorial

We have made an effort to keep the API as simple and minimal as possible. The rendering functions are quite similar to submitting DirectX or OpenGL drawcalls
and we hope they will feel natural to anyone with graphics programming experience. In the following we will use the example project as a tutorial to showcase
the API. Please refer to the documentation in the header file for further details.

### Setup

We begin by creating a new instance of the occlusion culling object. The object is created using the static `Create()` function rather than a standard
constructor, and can be destroyed using the `Destroy()` function. The reason for using the factory `Create()`/`Destroy()` design pattern is that we want to
support custom (aligned) memory allocators, and that the library choses either the AVX-512, AVX or SSE implementation based on the CPU's capabilities.

```C++
MaskedOcclusionCulling *moc = MaskedOcclusionCulling::Create();

...

MaskedOcclusionCulling::Destroy(moc);
```

The created object is empty and has no hierarchical depth buffer attached, so we must first allocate a buffer using the `SetResolution()` function. This function can
also be used later to resize the hierarchical depth buffer, causing it to be re-allocated. Note that the resolution width must be a multiple of 8, and the height
a multiple of 4. This is a limitation of the occlusion culling algorithm.

```C++
int width = 1920;
int height = 1080;
moc.SetResolution(width, height);   // Set full HD resolution
```
After setting the resolution we can start rendering occluders and performing occlusion queries. We must first clear the hierarchical depth buffer

```C++
// Clear hierarchical depth buffer to far depth
moc.ClearDepthBuffer();
```

**Optional** The `SetNearClipPlane()` function can be used to configure the distance to the near clipping plane to make the occlusion culling renderer match your DX/GL
renderer. The default value for the near plane is 0 which should work as expected unless your application relies on having onscreen geometry clipped by
the near plane.

```C++
float nearClipDist = 1.0f;
moc.SetNearClipPlane(nearClipDist); // Set near clipping dist (optional)
```

### Occluder rendering

The `RenderTriangles()` function renders triangle meshes to the hierarchical depth buffer. Similar to DirectX/OpenGL, meshes are constructed from a vertex array
and an triangle index array. By default, the vertices are given as *(x,y,z,w)* floating point clip space coordinates, but the *z*-coordinate is ignored and
instead we use *depth = 1 / w*. We expose a `TransformVertices()` utility function to transform vertices from *(x,y,z,1)* model/world space to *(x,y,z,w)* clip
space, but you can use your own transform code as well. For more information on the `TransformVertices()` function, please refer to the documentaiton in the
header file.

The triangle index array is identical to a DirectX or OpenGL triangle list and connects vertices to form triangles. Every three indices in the array form a new
triangle, so the size of the array must be a multiple of 3. Note that we only support triangle lists, and we currently have no plans on supporting other primitives
such as strips or fans.

```C++
struct ClipSpaceVertex { float x, y, z, w; };

// Create an example triangle. The z component of each vertex is not used by the
// occlusion culling system. 
ClipspaceVertex triVerts[] = { { 5, 0, 0, 10 }, { 30, 0, 0, 20 }, { 10, 50, 0, 40 } };
unsigned int triIndices[] = { 0, 1, 2 };
unsigned int nTris = 1;

// Render an example triangle
moc.RenderTriangles(triVerts, triIndices, nTris);
```

**Transform** It is possible to include a transform when calling `RenderTriangles()`, by passing the modelToClipSpace parameter.  This is equivalent to calling `TransformVertices()`, followed
by `RenderTriangles()`, but performing the transform as shown in the example below typically
leads to better performance.

```C++
// Example matrix swapping the x and y coordinates
float swapxyMatrix[4][4] = {
	{0,1,0,0},
	{1,0,0,0},
	{0,0,1,0},
	{0,0,0,1}};

// Render triangle with transform.
moc.RenderTriangles(triVerts, triIndices, nTris, swapxyMatrix);
```

**Backface Culling** By default, clockwise winded triangles are considered backfacing and are culled when rasterizing occluders. However, you can 
configure the `RenderTriangles()` function to backface cull either clockwise or counter-clockwise winded triangles, or to disable backface culling
for two-sided rendering.

```C++
// A clockwise winded (normally backfacing) triangle
ClipspaceVertex cwTriVerts[] = { { 7, -7, 0, 20 },{ 7.5, -7, 0, 20 },{ 7, -7.5, 0, 20 } };
unsigned int cwTriIndices[] = { 0, 1, 2 };

// Render with counter-clockwise backface culling, the triangle is drawn
moc->RenderTriangles((float*)cwTriVerts, cwTriIndices, 1, nullptr, BACKFACE_CCW);
```

The rasterization code only handles counter-clockwise winded triangles, so configurable backface culling is implemented by re-winding clockwise winded triangles 
on the fly. Therefore, other culling modes than `BACKFACE_CW` may decrease performance slightly.

**Clip Flags** `RenderTriangles()` accepts an additional parameter to optimize polygon clipping. The calling application may disable any clipping plane if it can
guarantee that the mesh does not intersect said clipping plane. In the example below we have a quad which is entirely on screen, and we can disable
all clipping planes. **Warning** it is unsafe to incorrectly disable clipping planes and this may cause the program to crash or perform out of bounds
memory accesses. Consider this a power user feature (use `CLIP_PLANE_ALL` to clip against the full frustum when in doubt).

```C++
// Create a quad completely within the view frustum
ClipspaceVertex quadVerts[]
	= { { -150, -150, 0, 200 },{ -10, -65, 0, 75 },{ 0, 0, 0, 20 },{ -40, 10, 0, 50 } };
unsigned int quadIndices[] = { 0, 1, 2, 0, 2, 3 };
unsigned int nTris = 2;

// Render the quad. As an optimization, indicate that clipping is not required
moc.RenderTriangles((float*)quadVerts, quadIndices, nTris, nullptr, BACKFACE_CW, CLIP_PLANE_NONE);
```

**Vertex Storage Layout** Finally, the `RenderTriangles()` supports configurable vertex storage layout. The code so far has used an array of structs (AoS) layout based 
on the `ClipSpaceVertex` struct, and this is the default behaviour. You may use the `VertexLayout` struct to configure the memory layout of the vertex data. Note that 
the vertex pointer passed to the `RenderTriangles()` should point at the *x* coordinate of the first vertex, so there is no x coordinate offset specified in the struct.

```C++
struct VertexLayout
{
	int mStride;  // Stride between vertices
	int mOffsetY; // Offset to vertex y coordinate
	int mOffsetW; // Offset to vertex w coordinate
};
```

For example, you can configure a struct of arrays (SoA) layout as follows

```C++
// A triangle specified on struct of arrays (SoA) form
float SoAVerts[] = {
	 10, 10,   7, // x-coordinates
	-10, -7, -10, // y-coordinates
	 10, 10,  10  // w-coordinates
};

// Set vertex layout (stride, y offset, w offset)
VertexLayout SoAVertexLayout(sizeof(float), 3 * sizeof(float), 6 * sizeof(float));

// Render triangle with SoA layout
moc.RenderTriangles((float*)SoAVerts, triIndices, 1, nullptr, BACKFACE_CW, CLIP_PLANE_ALL, SoAVertexLayout);
```

Vertex layout may affect performance. We have seen no large performance impact when using either SoA or AoS layout, but generally speaking the
vertex position data should be packed as compactly into memory as possible to minimize number of cache misses. It is, for example, not advicable to bundle vertex
position data together with normals, texture coordinates, etc. and using a large stride.

### Occlusion queries

After rendering a few occluder meshes you can begin to perform occlusion queries. There are two functions for occlusion queries, called `TestTriangles()` and
`TestRect()`. The `TestTriangles()` function is identical to `RenderTriangles()` with the exception being that it performs an occlusion query and does not
update the hierarchical depth buffer. The result of the occlusion query is returned as an enum, which indicates if the triangles are `VISIBLE`, `OCCLUDED`, or were
`VIEW_CULLED`. Here, `VIEW_CULLED` means that all triangles were either frustum or back face culling, so no occlusion culling test had to be performed.

```C++
// A triangle that is partly, but not completely, overlapped by the quad rendered before
ClipspaceVertex oqTriVerts[] = { { 0, 50, 0, 200 },{ -60, -60, 0, 200 },{ 20, -40, 0, 200 } };
unsigned int oqTriIndices[] = { 0, 1, 2 };
unsigned int nTris = 1;

// Perform an occlusion query. The triangle is visible and the query should return VISIBLE
CullingResult result = moc.TestTriangles((float*)oqTriVerts, oqTriIndices, nTris);
```

The `TestRect()` function performs an occlusion query for a rectangular screen space region with a given depth. It can be used to, for example, quickly test
the projected bounding box of an object to determine if the entire object is visible or not. The function is considerably faster than `TestTriangles()` becuase
it does not require input assembly, clipping, or triangle rasterization. The queries are typically less accurate as screen space bounding rectangles tend to
grow quite large, but we've personally seen best overall performance using this type of culling.

```C++
// Perform an occlusion query testing if a rectangle is visible. The rectangle is completely
// behind the previously drawn quad, so the query should indicate that it's occluded
result = moc.TestRect(-0.6f, -0.6f, -0.4f, -0.4f, 100);
```

Unlike the other functions the input to `TestRect()` is normalized device coordinates (NDC). Normalized device coordinates are projected clip space coordinates
*(x' = x/w, y' = y/w)* and the visible screen maps to the range [-1,1] for both the *x'* and *y'* coordinate. The w coordinate is still given in clip space,
however. It is up to the application to compute projected bounding rectangles from the object's bounding shapes.

### Debugging and visualization

We expose a utility function, `ComputePixelDepthBuffer()` that can be used to visualize the hierarchical depth buffer used internally by the occlusion culling
system. The function fills in a complete per-pixel depth buffer, but the internal representation is hierarchical with just two depth values and a mask stored per
tile. It is not reasonable to expect the image to completely match the exact depth buffer, and you may notice some areas where backrgound objects leak through
the foreground. Leakage is part of the algorithm (and one reason for the high performance), and we have
not found it to be problematic. However, if you experience issues due to leakage you may want to disable the `QUICK_MASK` define, described in more detail in the
section on [hierarchical depth buffer updates](#update).

```C++
// Compute a per pixel depth buffer from the hierarchical depth buffer, used for visualization.
float *perPixelZBuffer = new float[width * height];
moc.ComputePixelDepthBuffer(perPixelZBuffer);
```

We also support basic instrumentation to help with profiling and debugging. By defining `ENABLE_STATS` in the header file, the occlusion culling code will
gather statistics about the number of occluders rendered and occlusion queries performed. For more details about the statistics, see the
`OcclusionCullingStatistics` struct. The statistics can be queried using the `GetStatistics()` function, which will simply return a zeroed struct if `ENABLE_STATS`
is not defined. Note that instrumentation reduces performance somewhat and should generally be disabled in release builds.

```C++
OcclusionCullingStatistics stats = moc.GetStatistics();
```

### Memory management

As shown in the example below, you may optionally provide callback functions for allocating and freeing memory when creating a
`MaskedOcclusionCulling` object. The functions must support aligned allocations.

```C++
void *alignedAllocCallback(size_t alignment, size_t size)
{
	...
}

void alignedFreeCallback(void *ptr)
{
	...
}

MaskedOcclusionCulling *moc = MaskedOcclusionCulling::Create(alignedAllocCallback, alignedFreeCallback);
```

## <a name="update"></a>Hierarchical depth buffer update algorithm and render order

The library contains two update algorithms / heuristics for the hierarchical depth buffer, one focused on speed and one focused on accuracy. The
active algorithm can be configured using the `QUICK_MASK` define. Setting the define (default) enables algorithm is described in the research paper
["Masked Software Occlusion Culling"](https://software.intel.com/en-us/articles/masked-software-occlusion-culling), which has a good balance between low
leakage and good performance. Not defining `QUICK_MASK` enables the mergine heuristic used in the paper
["Masked Depth Culling for Graphics Hardware"](http://dl.acm.org/citation.cfm?id=2818138). It is more accurate, with less leakage, but also has lower performance.

If you experience problems due to leakage you may want to use the more accurate update algorithm. However, rendering order can also affect the quality
of the hierarchical depth buffer, with the best order being rendering objects front-to-back. We perform early depth culling tests during occluder

rendering, so rendering in front-to-back order will not only improve quality, but also greatly improve performance of occluder rendering. If your scene
is stored in a hierarchical data structure, it is often possible to modify the traversal algorithm to traverse nodes in approximate front-to-back order,
see the research paper ["Masked Software Occlusion Culling"](https://software.intel.com/en-us/articles/masked-software-occlusion-culling) for an example.

## <a name="interleaved"></a>Interleaving occluder rendering and occlusion queries

The library supports *light weight* switching between occluder rendering and occlusion queries. While it is still possible to do occlusion culling
as a standard two pass algorithm (first render all occluders, then perform all queries) it is typically beneficial to interleave occluder rendering with
queries.

This is especially powerful when rendering objects in front-to-back order. After drawing the first few occluder triangles, you can start performing
occlusion queries, and if the occlusion query indicate that an object is occluded there is no need to draw the occluder mesh for that object. This
can greatly improve the performance of the occlusion culling pass in itself. As described in further detail in the research paper
["Masked Software Occlusion Culling"](https://software.intel.com/en-us/articles/masked-software-occlusion-culling), this may be used to perform early exits in
BVH traversal code.

## Rasterization precision

The library supports high precision rasterization through Bresenham interpolation, and this may be enabled by changing the `PRECISE_COVERAGE` define in 
the header file. The high precision rasterizer is somewhat slower (5-15%) than using the default rasterizer, but is compliant with DirectX 11 and OpenGL 
rasterization rules. We have empirically verified it on a large set of randomly generated on-screen triangles. While there still may be differences to GPU 
rasterization due to clipping or vertex transform precision differences, we have not noticed any differences in rasterized coverage in our test scenes. Note 
that tie breaker rules and vertex rounding behaves differently between DirectX and OpenGL due to the direction of the screen space Y axis. The `USE_D3D` define
(enabled by default) can be used to toggle between DirectX or OpenGL behaviour.

## Multi-threading and binned rendering

Multi-threading is supported through a binning rasterizer. The `MaskedOcclusionCulling` class exposes two functions, `BinTriangles()` and `RenderTrilist()`
that may be used to perform binning, and render all triangles assigned to a bin. Using binned rasterization makes it simple to guarantee that no two threads are
accessing the same part of the framebuffer, as rendering is limited to a particular bin, or region of the screen.

Binned rendering starts by performing geometry processing (primitive assembly, vertex transform, clipping, and projection) followed by a binning step, where
triangles are written to all bins they overlap. This is performed using the `BinTriangles()` function, which is very similar to the `RenderTriangles()`
function, but provides some additional parameters for specifying the number of bins the screen is split into. The calling application also needs to pass a
pointer to an array of `TriList` object, with one instance per bin. Each `TriList` object points to a "scratchpad" memory buffer, and all triangles overlapping
that bin will be written to the buffer.

```C++
const int binsW = 4;
const int binsH = 4;

float *dataBuffer = new float[binsW*BinsH*1024*3*3]; // Allocate storage for 1k triangles in each trilist
TriList *triLists  = new TriList[binsW*binsH];       // Allocate trilists for 4x4 = 16 bins
for (int i = 0; i < binsW*BinsH; ++i)
{
	triLists[i].mNumTriangles = 1024; // triangle list capacity
	triLists[i].mTriIdx = 0; // Set triangle write pointer to first element
	triLists[i].mData = dataBuffer + i*1024*1024;
}

// Perform geometry processing and write triangles to the triLists of all bins they overlap.
moc.BinTriangles(triVerts, triIndices, nTris, triLists, binsW, binsW);
```

After generating the triangle lists for each bin, the triangles may be rendered using the `RenderTrilist()` function and the rendering region should be
limited using a scissor rectangle. It should be noted that the `BinTriangles()` function makes assumptions on the size of the bins, and the calling
application must therefore always compute the scissor region of each bin, relying on the `ComputeBinWidthHeight()` utility function as shown in the 
example below. Note that the scissor rectangle is specified in screen space coordinates which depends on the `USE_D3D` define.

```C++
unsigned int binWidth, binHeight;
moc.ComputeBinWidthHeight(mBinsW, mBinsH, binWidth, binHeight);

for (int by = 0; by < binsH; ++by)
{
	for (int bx = 0; bx < binsW ; ++bx)
	{
		// Compute scissor rectangle that matches the one assumed by BinTriangles()
		// note that the ScissorRect is specified in pixel coordinates, with (0,0)
		// being the bottom left corner
		ScissorRect binRect;
		binRect.minX = bx*binWidth;
		binRect.maxX = bx + 1 == binsW ? screenWidth : (bx + 1) * binWidth;
		binRect.minY = by*binHeight;
		binRect.maxY = by + 1 == binsH ? screenHeight : (by + 1) * binHeight;

		// Render all triangles overlapping the current bin.
		moc.RenderTrilist(triLists[bx + by*4], &binRect);
	}
}
```

### Multi-threading example

This library includes a multi-threading example in the `CullingThreadpool` class. The class interface is similar to that of `MaskedOcclusionCulling`, but occluder
rendering is performed asynchronously. Calling the `CullingThreadpool::RenderTriangles()` function adds a render job to a command queue and immediately return
to the calling thread, rather than immediately performing the rendering work. Internally, the class uses the `BinTriangles()` and `RenderTrilist()` functions to
bin all triangles of the `CullingThreadpool::RenderTriangles()` call, and distribute the tiles. At any time, there may be a number of binning jobs, and tile
rendering jobs unprocessed, and the scheduler picks the most urgent job and process it first. If a thread runs out of available jobs, task stealing is used as a
means of improving load-balancing.

The occlusion query functions `CullingThreadpool::TestTriangles()` and `CullingThreadpool::TestRect()` immediately return the result of the query. However, the
query depends on the contents of the hierarchical depth buffer you may need to wait for the worker threads to finish to make sure the query is performed on the
most up to date version of the buffer, this can be accomplished by calling `CullingThreadpool::Flush()`. It is not always necessary to work with the most up to
date version of the hierarchical depth buffer for a query. While the result may be incorrect, it is still always conservative in that occluded objects may be
classified as visible, but not the other way around. Since the `CullingThreadpool::Flush()` causes a wait it may be beneficial to work against a slightly out of
date version of the hierarchical depth buffer if your application will cause a lot of flushes. We found this particularly true when implementing threading in
our interleaved BVH traversal algorithm (see the ["Masked Software Occlusion Culling"](https://software.intel.com/en-us/articles/masked-software-occlusion-culling)
paper) where each BVH traversal step is based on the outcome of an occlusion query interleaved with occluder rendering for the BVH-leaves.

The `CullingThreadpool` class was written as an example and not the de-facto threading approach. In some cases we believe it is possible to improve performance
further by threading occlusion queries, or thread the entire occlusion culling system, including scene graph traversal. However, it does provide a simple means
of enabling multi-threading in a traditional single threaded application as the APIs is very similar to the `MaskedOcclusionCulling` class, and may be called from
a single threaded application. As previously mentioned we integrated this implementation in our interleaved BVH traversal algorithm (see the ["Masked Software Occlusion Culling"](https://software.intel.com/en-us/articles/masked-software-occlusion-culling)
paper) and noted speedup of roughly *3x*, running on four threads, compared to our previous single threaded implementation.

## Compiling

The code has been reworked to support more platforms and compilers, such as [Intel C++ Compiler](https://software.intel.com/en-us/intel-compilers), [G++](https://gcc.gnu.org/) 
and [LLVM/Clang](http://releases.llvm.org/download.html). The original Visual Studio 2015 projects remain and works with both ICC and Microsoft's compilers. Other compilers 
are supported through [CMake](https://cmake.org/). See the `CMakeLists.txt` files in the `Example` and `FillrateTest` folders. You can use CMake to generate a 
Visual Studio project for Clang on Windows:

```
md <path to library>\Example\Clang
cd <path to library>\Example\Clang
cmake -G"Visual Studio 14 2015 Win64" -T"LLVM-vs2014" ..
```

or build the library with G++/Clang on linux systems (the `D3DValidate` sample only works on Windows as it relies on Direct 3D)

```
mkdir <path to library>/Example/Release
cd <path to library>/Example/Release
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

Note that AVX-512 support is only experimental at the moment, and has only been verified through [Intel SDE](https://software.intel.com/en-us/articles/pre-release-license-agreement-for-intel-software-development-emulator-accept-end-user-license-agreement-and-download).
If using the original visual studio project, you need to "opt in" for AVX-512 support by setting `#define USE_AVX512 1`. When building with CMake you can
enable AVX support using the `-DUSE_AVX512=ON` option:

```
cmake -DUSE_AVX512=ON -G"Visual Studio 14 2015 Win64" -T"LLVM-vs2014" ..
```

## Version History

* Version 1.4: 
  * Added support for merging 2 depth buffers as detailed in GDC 2018 presenation.
  * Fixed Profiling counters to be thread safe removing a race condition when runing the CullingThreadpool class.
* Version 1.3: 
  * **Experimental**: Added support for AVX-512 capable CPUs. Currently only verified through [emulator](https://software.intel.com/en-us/articles/intel-software-development-emulator).
  * Added multiplatform support. Code now compiles on Visual C++ Compiler, Intel C++ Compiler, GCC, and Clang.
  * Added configurable backface culling, to support two-sided occluder rendering.
* Version 1.2: 
  * Added support for threading, through a binning rasterizer. The `CullingThreadpool` class implements an example multi-threaded task system with a very similar 
    API to the `MaskedOcclusionCulling`class.
  * Added support for higher precision rasterization, with DirectX and OpenGL compliant rasterization rules.
  * **Note:** The default screen space coordinate system has been changed from OpenGL to DirectX conventions. If you upgrade from an older version of the library
    this will flip the y coordinate of scissor boxes and the images returned by `ComputePixelDepthBuffer()`. Disabling the `USE_D3D` define changes back to OpenGL conventions.
* Version 1.1: 
  * Added support for SSE4.1 and SSE2 capable CPUs for backwards compatibility. The SSE versions must emulate some operations using
  simpler instructions, and are therefore less efficient, with the SSE2 version having the lowest performance.
* Version 1.0: 
  * Initial revision, only support for AVX2 capable CPUs

## Differences to the research paper

This code does not exactly match implementation used in
["Masked Software Occlusion Culling"](https://software.intel.com/en-us/articles/masked-software-occlusion-culling), and performance may vary slightly
from what is presented in the research paper. We aimed for making the API as simple as possible and have removed many limitations, in particular
requirements on input data being aligned to SIMD boundaries. This affects performance slightly in both directions. Unaligned loads and
gathers are more costly, but unaligned data may be packed more efficiently in memory leading to fewer cache misses.

## License agreement

Copyright Intel(R) Corporation 2016-2025.

See the Apache 2.0 license.txt for full license agreement details.

Disclaimer:

This software is subject to the U.S. Export Administration Regulations and other U.S.
law, and may not be exported or re-exported to certain countries (Cuba, Iran, North
Korea, Sudan, and Syria) or to persons or entities prohibited from receiving U.S.
exports (including Denied Parties, Specially Designated Nationals, and entities on the
Bureau of Export Administration Entity List or involved with missile technology or
nuclear, chemical or biological weapons)..
