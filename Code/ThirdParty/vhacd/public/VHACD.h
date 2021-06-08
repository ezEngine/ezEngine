/* Copyright (c) 2011 Khaled Mamou (kmamou at gmail dot com)
 All rights reserved.
 
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 3. The names of the contributors may not be used to endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once
#ifndef VHACD_H
#    define VHACD_H

#if VHACD_SHARED_LIBRARY
#  ifdef VHACD_BUILDING_LIB
#    define VHACD_DLL __declspec(dllexport)
#  else
#    define VHACD_DLL __declspec(dllimport)
#  endif
#else
#  define VHACD_DLL
#endif

#    define VHACD_VERSION_MAJOR 3
#    define VHACD_VERSION_MINOR 1

// Changes for version 3.0 : April 17, 2020 : John W. Ratcliff mailto:jratcliffscarab@gmail.com
//
// * An optional callback method called 'NotifyVHACDComplete' was added to the IUserCallback interface.
// * This method is called when an asynchronous V-HACD operation is been completed. The application can
// * use this call back as a trigger to wake up the app to start processing the V-HACD results. The
// * original polled method 'IsReady' is still valid and can continue to be used.
//
// * Some code cleanup was done, removed obsolete OpenCL, OpenMP and SIMD code.
// * The purpose of this change was to make the code more vanilla C++ and to make it easier to maintain
// * Removed the obsolete tetrahedral 'mode' as the library currently only supports voxels.
// * Again this was to make the code base smaller and easier to maintain.
// *
// * A new optional virtual interface called 'IUserProfiler' was provided.
// * This allows the user to provide an optional profiling callback interface to assist in
// * diagnosing performance issues. This change was made by Danny Couture at Epic for the UE4 integration.
// * Some profiling macros were also declared in support of this feature.
// *
// * Another new optional virtual interface called 'IUserTaskRunner' was provided.
// * This interface is used to run logical 'tasks' in a background thread. If none is provided
// * then a default implementation using std::thread will be executed.
// * This change was made by Danny Couture at Epic to speed up the voxelization step.
// *
// * Some performance optimizations were made to the code. They include the following:
// *
// * During the voxelization step there was code to rotate every point in the source mesh.
// * However this feature was not actually being used, so it was removed to improve performance
// * during the voxelization stage.
// *
// * Optimizations made to the voxelization step by Danny Couture at Epic include:
// * -Fix a comparison bug in VHACD Volume::Voxelize causing the number of
// *   voxels to explode when x == y and z < x.
// * - Rewrite voxel flood fill algorithm toward cache friendliness and get
// *      rid of extra data structure causing high memory usage on large voxels.
// * - Make the data ordering in vhacdVolume GetVoxel more cache friendly
// *   toward loops iterating over(i, j, k) in that order.
// *
// * During the voxelization phase, the original implementation would attempt to voxelize the input mesh
// * multiple times until it hit some target number of voxels. This step was not needed, as the number of
// * of desired voxels implies a certain voxel resolution so instead it just does the voxelization step
// * in a single pass at the specified detail level.
// *
// * The final performance optimization was to unroll the ComputeACD step so it can be run in parallel.
// * A new helper class was introduced called 'SimpleJobSystem' which can run micro-tasks across multiple threads.
// * When computing the ACD (approximate convex decomposition) the code iterates through all input sub-meshes
// * and computes the concavity, the optimal splitting plane and, optionally, splits the input mesh by a plane.
// * This can happen hundreds of even thousands of times. The original code did this in a for loop using a single
// * thread. The change made was so that this operation could be done in parallel. Since this operation is extremely
// * memory bound, it only provides about a 2x performance improvement, but that is still twice as fast at least.
// * After the voxelization optimizations the ComputeACD step remained slowest part of V-HACD.
// *
// * A new feature was introduced called the 'FillMode'. By default V-HACD performs a flood fill operation after
// * the voxelization step to determine which voxels lie 'inside' the source mesh surface and which are 'outside'.
// * Most of the time this flood fill approach works fine. However, if the source mesh has a hole anywhere in it
// * it can result in no voxels being marked as 'inside the surface'.  When this happens the convex decomposition
// * will treat the mesh as being 'hollow' inside. A good example of this failure case is the Stanford bunny which
// * has a hole on the bottom.
// *
// * A new FillMode was introduced called 'RAYCAST_FILL' which can correct for this. If the FillMode is set to
// * 'RAYCAST_FILL' then rather than doing a flood fill, instead each voxel will be categorized by performing
// * a raycast in the six cardinal directions. If 3 or more of those raycasts hit the 'inside' of the source mesh
// * then that voxel will be marked as being 'inside'. This can correct for the edge case when the source mesh has
// * a voxel sized hole in it.
// *
// * Finally, there are some cases where you might actually want the source mesh to be treated as being a 'hollow'
// * object.  For certain types of simulations this is actually desirable. So, there is a new FillMode called
// * 'SURFACE_ONLY' which will not mark any voxel as being 'inside' which will treat any source mesh as being 'hollow'
// * inside and any deep recursion of the convex decomposition will converge towards the 'skin' of the object.
// * For example, here is a video clip showing a simulation of a torus, without any holes, as a hollow object.
// *     https://www.youtube.com/watch?v=CIOoWN4CDb4
// *
// * Finally an important bug fix is included with this update.
// * When using V-HACD by default, it works by converting the source mesh into a voxel grid and the performing
// * the convex decomposition relative to that voxelization. This has a number of benefits, both performance as
// * well as it greatly simplifies the plane splitting step vs. trying to perform a full CSG operation on an
// * arbitrary triangle mesh.
// *
// * However, one downside of doing this is that the resulting convex hulls suffer a loss of precision. Each
// * point in the final convex hull does not rest on the original source mesh but, instead, is based on the voxel
// * resolution. If you use extremely high voxel resolutions then this loss of precision is less of an issue but
// * using high voxel resolutions can make V-HACD run much more slowly.  If you use a low voxel resolution V-HACD
// * can run much more quickly but the output convex hull points will not be coplanar to the original source mesh
// * and can result in poor simulation behavior.
// *
// * To fix this a new option was introduced called 'm_projectHullVertices'. If th is bool is set to true then
// * as a final step each point on the convex hull output will be 'projected' onto the source mesh. In the previous
// * release of V-HACD this 'projection' was done by performing a raycast operation. However, it was discovered that
// * on some types of meshes this operation could fail producing completely bad convex hulls.
// *
// * This bug was fixed by switching to a 'closest point' algorithm instead of a raycast. A new code snippet was
// * introduced called 'aabbtree', originally written by Miles Macklin, that can compute the closest point on a triangle
// * mesh, within a radius, in a highly efficient manner.


// Changes for version 2.3
//
// m_gamma : Has been removed.  This used to control the error metric to merge convex hulls.  Now it uses the
// 'm_maxConvexHulls' value instead. m_maxConvexHulls : This is the maximum number of convex hulls to produce from the
// merge operation; replaces 'm_gamma'.
//
// Note that decomposition depth is no longer a user provided value.  It is now derived from the
// maximum number of hulls requested.
//
// As a convenience to the user, each convex hull produced now includes the volume of the hull as well as it's center.
//
// This version supports a convenience method to automatically make V-HACD run asynchronously in a background thread.
// To get a fully asynchronous version, call 'CreateVHACD_ASYNC' instead of 'CreateVHACD'.  You get the same interface
// however, now when computing convex hulls, it is no longer a blocking operation.  All callback messages are still
// returned in the application's thread so you don't need to worry about mutex locks or anything in that case. To tell
// if the operation is complete, the application should call 'IsReady'.  This will return true if the last approximation
// operation is complete and will dispatch any pending messages. If you call 'Compute' while a previous operation was
// still running, it will automatically cancel the last request and begin a new one.  To cancel a currently running
// approximation just call 'Cancel'.

#    include <stdint.h>
#    include <functional>

#    define VHACD_PREPROCESSOR_JOIN(x, y) VHACD_PREPROCESSOR_JOIN_INNER(x, y)
#    define VHACD_PREPROCESSOR_JOIN_INNER(x, y) x##y

// Since we're using a virtual interface to abstract the profiling, expect it to have
// some kind of performance impact when the work inside it is getting too small.
// This is currently used to get a coarse level of detail and some sense of where the time is spent without
// impacting the overall performance.
#    define VHACD_TRACE_CPUPROFILER_EVENT_SCOPE(Profiler, Name)                                                        \
        static int32_t VHACD_PREPROCESSOR_JOIN(__CpuProfilerEventSpecId, __LINE__);                                    \
        if (Profiler && VHACD_PREPROCESSOR_JOIN(__CpuProfilerEventSpecId, __LINE__) == 0)                              \
        {                                                                                                              \
            VHACD_PREPROCESSOR_JOIN(__CpuProfilerEventSpecId, __LINE__) = Profiler->AllocTagId(#Name);                 \
        }                                                                                                              \
        ::VHACD::ProfilerEventScope VHACD_PREPROCESSOR_JOIN(__CpuProfilerEventScope, __LINE__)(                        \
            Profiler, VHACD_PREPROCESSOR_JOIN(__CpuProfilerEventSpecId, __LINE__));

#    define VHACD_TRACE_BOOKMARK(Profiler, Text)                                                                       \
        if (Profiler)                                                                                                  \
        {                                                                                                              \
            Profiler->Bookmark(Text);                                                                                  \
        }


namespace VHACD
{

enum class FillMode
{
    FLOOD_FILL, // This is the default behavior, after the voxelization step it uses a flood fill to determine 'inside'
                // from 'outside'. Howerver, meshes with holes can fail and create hollow results.
    SURFACE_ONLY, // Only consider the 'surface', will create 'skins' with hollow centers.
    RAYCAST_FILL, // Uses raycasting to determine inside from outside.
};

class VHACD_DLL IVHACD
{
public:
    class IUserCallback
    {
    public:
        virtual ~IUserCallback(){};

        // Be aware that if you are running V-HACD asynchronously (in a background thread) this callback will come from
        // a different thread. So if your print/logging code isn't thread safe, take that into account.
        virtual void Update(const double overallProgress,
                            const double stageProgress,
                            const double operationProgress,
                            const char* const stage,
                            const char* const operation) = 0;

        // This is an optional user callback which is only called when running V-HACD asynchronously.
        // This is a callback performed to notify the user that the
        // convex decomposition background process is completed. This call back will occur from
        // a different thread so the user should take that into account.
        virtual void NotifyVHACDComplete(void)
        {
        }
    };

    class IUserLogger
    {
    public:
        virtual ~IUserLogger(){};
        virtual void Log(const char* const msg) = 0;
    };

    class IUserProfiler
    {
    public:
        virtual ~IUserProfiler(){};
        virtual uint32_t AllocTagId(const char* const tagName) = 0;
        virtual void EnterTag(uint32_t tagId) = 0;
        virtual void ExitTag() = 0;
        virtual void Bookmark(const char* const text) = 0;
    };

    class IUserTaskRunner
    {
    public:
        virtual ~IUserTaskRunner(){};
        virtual void* StartTask(std::function<void()> func) = 0;
        virtual void JoinTask(void* Task) = 0;
    };

    class ConvexHull
    {
    public:
        double* m_points;
        uint32_t* m_triangles;
        uint32_t m_nPoints;
        uint32_t m_nTriangles;
        double m_volume;
        double m_center[3];
    };

    class Parameters
    {
    public:
        Parameters(void)
        {
            Init();
        }
        void Init(void)
        {
            m_fillMode = FillMode::FLOOD_FILL;
            m_resolution = 1000000;
            m_concavity = 0.001;
            m_planeDownsampling = 4;
            m_convexhullDownsampling = 4;
            m_alpha = 0.05;
            m_beta = 0.05;
            m_maxNumVerticesPerCH = 64;
            m_minVolumePerCH = 0.0001;
            m_callback = nullptr;
            m_logger = nullptr;
            m_profiler = nullptr;
            m_taskRunner = nullptr;
            m_convexhullApproximation = true;
            m_maxConvexHulls = 1024;
            m_projectHullVertices = true; // This will project the output convex hull vertices onto the original source
                                          // mesh to increase the floating point accuracy of the results
            m_asyncACD = true; // whether or not to perform the ACD operation in parallel
        }
        double m_concavity;
        double m_alpha;
        double m_beta;
        double m_minVolumePerCH;
        IUserCallback* m_callback;
        IUserLogger* m_logger;
        IUserProfiler* m_profiler;
        IUserTaskRunner* m_taskRunner;
        uint32_t m_resolution;
        uint32_t m_maxNumVerticesPerCH;
        uint32_t m_planeDownsampling;
        uint32_t m_convexhullDownsampling;
        uint32_t m_convexhullApproximation;
        uint32_t m_maxConvexHulls;
        bool m_projectHullVertices;
        FillMode m_fillMode{ FillMode::FLOOD_FILL };
        bool m_asyncACD{ true };
    };

    virtual void Cancel() = 0;

    virtual bool Compute(const float* const points,
                         const uint32_t countPoints,
                         const uint32_t* const triangles,
                         const uint32_t countTriangles,
                         const Parameters& params) = 0;

    virtual bool Compute(const double* const points,
                         const uint32_t countPoints,
                         const uint32_t* const triangles,
                         const uint32_t countTriangles,
                         const Parameters& params) = 0;
    virtual uint32_t GetNConvexHulls() const = 0;
    virtual void GetConvexHull(const uint32_t index, ConvexHull& ch) const = 0;
    virtual void Clean(void) = 0; // release internally allocated memory
    virtual void Release(void) = 0; // release IVHACD

    // Will compute the center of mass of the convex hull decomposition results and return it
    // in 'centerOfMass'.  Returns false if the center of mass could not be computed.
    virtual bool ComputeCenterOfMass(double centerOfMass[3]) const = 0;

    // In synchronous mode (non-multi-threaded) the state is always 'ready'
    // In asynchronous mode, this returns true if the background thread is not still actively computing
    // a new solution.  In an asynchronous config the 'IsReady' call will report any update or log
    // messages in the caller's current thread.
    virtual bool IsReady(void) const
    {
        return true;
    }

protected:
    virtual ~IVHACD(void)
    {
    }
};

struct ProfilerEventScope
{
    IVHACD::IUserProfiler* mProfiler;

    ProfilerEventScope(IVHACD::IUserProfiler* InProfiler, uint32_t InTagId) : mProfiler(InProfiler)
    {
        if (mProfiler)
        {
            mProfiler->EnterTag(InTagId);
        }
    }

    ~ProfilerEventScope()
    {
        if (mProfiler)
        {
            mProfiler->ExitTag();
        }
    }
};

VHACD_DLL IVHACD* CreateVHACD(void);
VHACD_DLL IVHACD* CreateVHACD_ASYNC(void);
} // namespace VHACD
#endif // VHACD_H
