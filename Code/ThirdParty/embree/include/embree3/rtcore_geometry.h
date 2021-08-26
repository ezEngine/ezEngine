// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "rtcore_buffer.h"

RTC_NAMESPACE_BEGIN

/* Opaque scene type */
typedef struct RTCSceneTy* RTCScene;

/* Opaque geometry type */
typedef struct RTCGeometryTy* RTCGeometry;

/* Types of geometries */
enum RTCGeometryType
{
  RTC_GEOMETRY_TYPE_TRIANGLE = 0, // triangle mesh
  RTC_GEOMETRY_TYPE_QUAD     = 1, // quad (triangle pair) mesh
  RTC_GEOMETRY_TYPE_GRID     = 2, // grid mesh

  RTC_GEOMETRY_TYPE_SUBDIVISION = 8, // Catmull-Clark subdivision surface

  RTC_GEOMETRY_TYPE_FLAT_LINEAR_CURVE   = 17, // flat (ribbon-like) linear curves

  RTC_GEOMETRY_TYPE_ROUND_BEZIER_CURVE  = 24, // round (tube-like) Bezier curves
  RTC_GEOMETRY_TYPE_FLAT_BEZIER_CURVE   = 25, // flat (ribbon-like) Bezier curves
  RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_BEZIER_CURVE  = 26, // flat normal-oriented Bezier curves
  
  RTC_GEOMETRY_TYPE_ROUND_BSPLINE_CURVE = 32, // round (tube-like) B-spline curves
  RTC_GEOMETRY_TYPE_FLAT_BSPLINE_CURVE  = 33, // flat (ribbon-like) B-spline curves
  RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_BSPLINE_CURVE  = 34, // flat normal-oriented B-spline curves

  RTC_GEOMETRY_TYPE_ROUND_HERMITE_CURVE = 40, // round (tube-like) Hermite curves
  RTC_GEOMETRY_TYPE_FLAT_HERMITE_CURVE  = 41, // flat (ribbon-like) Hermite curves
  RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_HERMITE_CURVE  = 42, // flat normal-oriented Hermite curves

  RTC_GEOMETRY_TYPE_SPHERE_POINT = 50,
  RTC_GEOMETRY_TYPE_DISC_POINT = 51,
  RTC_GEOMETRY_TYPE_ORIENTED_DISC_POINT = 52,

  RTC_GEOMETRY_TYPE_ROUND_CATMULL_ROM_CURVE = 58, // round (tube-like) Catmull-Rom curves
  RTC_GEOMETRY_TYPE_FLAT_CATMULL_ROM_CURVE  = 59, // flat (ribbon-like) Catmull-Rom curves
  RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_CATMULL_ROM_CURVE  = 60, // flat normal-oriented Catmull-Rom curves

  RTC_GEOMETRY_TYPE_USER     = 120, // user-defined geometry
  RTC_GEOMETRY_TYPE_INSTANCE = 121  // scene instance
};

/* Interpolation modes for subdivision surfaces */
enum RTCSubdivisionMode
{
  RTC_SUBDIVISION_MODE_NO_BOUNDARY     = 0,
  RTC_SUBDIVISION_MODE_SMOOTH_BOUNDARY = 1,
  RTC_SUBDIVISION_MODE_PIN_CORNERS     = 2,
  RTC_SUBDIVISION_MODE_PIN_BOUNDARY    = 3,
  RTC_SUBDIVISION_MODE_PIN_ALL         = 4,
};

/* Curve segment flags */
enum RTCCurveFlags
{
  RTC_CURVE_FLAG_NEIGHBOR_LEFT  = (1 << 0), 
  RTC_CURVE_FLAG_NEIGHBOR_RIGHT = (1 << 1) 
};

/* Arguments for RTCBoundsFunction */
struct RTCBoundsFunctionArguments
{
  void* geometryUserPtr;
  unsigned int primID;
  unsigned int timeStep;
  struct RTCBounds* bounds_o;
};

/* Bounding callback function */
typedef void (*RTCBoundsFunction)(const struct RTCBoundsFunctionArguments* args);

/* Arguments for RTCIntersectFunctionN */
struct RTCIntersectFunctionNArguments
{
  int* valid;
  void* geometryUserPtr;
  unsigned int primID;
  struct RTCIntersectContext* context;
  struct RTCRayHitN* rayhit;
  unsigned int N;
};

/* Intersection callback function */
typedef void (*RTCIntersectFunctionN)(const struct RTCIntersectFunctionNArguments* args);

/* Arguments for RTCOccludedFunctionN */
struct RTCOccludedFunctionNArguments
{
  int* valid;
  void* geometryUserPtr;
  unsigned int primID;
  struct RTCIntersectContext* context;
  struct RTCRayN* ray;
  unsigned int N;
};

/* Occlusion callback function */
typedef void (*RTCOccludedFunctionN)(const struct RTCOccludedFunctionNArguments* args);

/* Arguments for RTCDisplacementFunctionN */
struct RTCDisplacementFunctionNArguments
{
  void* geometryUserPtr;
  RTCGeometry geometry;
  unsigned int primID;
  unsigned int timeStep;
  const float* u;
  const float* v;
  const float* Ng_x;
  const float* Ng_y;
  const float* Ng_z;
  float* P_x;
  float* P_y;
  float* P_z;
  unsigned int N;
};

/* Displacement mapping callback function */
typedef void (*RTCDisplacementFunctionN)(const struct RTCDisplacementFunctionNArguments* args);

/* Creates a new geometry of specified type. */
RTC_API RTCGeometry rtcNewGeometry(RTCDevice device, enum RTCGeometryType type);

/* Retains the geometry (increments the reference count). */
RTC_API void rtcRetainGeometry(RTCGeometry geometry);

/* Releases the geometry (decrements the reference count) */
RTC_API void rtcReleaseGeometry(RTCGeometry geometry);

/* Commits the geometry. */
RTC_API void rtcCommitGeometry(RTCGeometry geometry);


/* Enables the geometry. */
RTC_API void rtcEnableGeometry(RTCGeometry geometry);

/* Disables the geometry. */
RTC_API void rtcDisableGeometry(RTCGeometry geometry);


/* Sets the number of motion blur time steps of the geometry. */
RTC_API void rtcSetGeometryTimeStepCount(RTCGeometry geometry, unsigned int timeStepCount);

/* Sets the motion blur time range of the geometry. */
RTC_API void rtcSetGeometryTimeRange(RTCGeometry geometry, float startTime, float endTime);
  
/* Sets the number of vertex attributes of the geometry. */
RTC_API void rtcSetGeometryVertexAttributeCount(RTCGeometry geometry, unsigned int vertexAttributeCount);

/* Sets the ray mask of the geometry. */
RTC_API void rtcSetGeometryMask(RTCGeometry geometry, unsigned int mask);

/* Sets the build quality of the geometry. */
RTC_API void rtcSetGeometryBuildQuality(RTCGeometry geometry, enum RTCBuildQuality quality);


/* Sets a geometry buffer. */
RTC_API void rtcSetGeometryBuffer(RTCGeometry geometry, enum RTCBufferType type, unsigned int slot, enum RTCFormat format, RTCBuffer buffer, size_t byteOffset, size_t byteStride, size_t itemCount);

/* Sets a shared geometry buffer. */
RTC_API void rtcSetSharedGeometryBuffer(RTCGeometry geometry, enum RTCBufferType type, unsigned int slot, enum RTCFormat format, const void* ptr, size_t byteOffset, size_t byteStride, size_t itemCount);

/* Creates and sets a new geometry buffer. */
RTC_API void* rtcSetNewGeometryBuffer(RTCGeometry geometry, enum RTCBufferType type, unsigned int slot, enum RTCFormat format, size_t byteStride, size_t itemCount);

/* Returns the pointer to the data of a buffer. */
RTC_API void* rtcGetGeometryBufferData(RTCGeometry geometry, enum RTCBufferType type, unsigned int slot);

/* Updates a geometry buffer. */
RTC_API void rtcUpdateGeometryBuffer(RTCGeometry geometry, enum RTCBufferType type, unsigned int slot);


/* Sets the intersection filter callback function of the geometry. */
RTC_API void rtcSetGeometryIntersectFilterFunction(RTCGeometry geometry, RTCFilterFunctionN filter);

/* Sets the occlusion filter callback function of the geometry. */
RTC_API void rtcSetGeometryOccludedFilterFunction(RTCGeometry geometry, RTCFilterFunctionN filter);

/* Sets the user-defined data pointer of the geometry. */
RTC_API void rtcSetGeometryUserData(RTCGeometry geometry, void* ptr);

/* Gets the user-defined data pointer of the geometry. */
RTC_API void* rtcGetGeometryUserData(RTCGeometry geometry);

/* Set the point query callback function of a geometry. */
RTC_API void rtcSetGeometryPointQueryFunction(RTCGeometry geometry, RTCPointQueryFunction pointQuery);

/* Sets the number of primitives of a user geometry. */
RTC_API void rtcSetGeometryUserPrimitiveCount(RTCGeometry geometry, unsigned int userPrimitiveCount);

/* Sets the bounding callback function to calculate bounding boxes for user primitives. */
RTC_API void rtcSetGeometryBoundsFunction(RTCGeometry geometry, RTCBoundsFunction bounds, void* userPtr);

/* Set the intersect callback function of a user geometry. */
RTC_API void rtcSetGeometryIntersectFunction(RTCGeometry geometry, RTCIntersectFunctionN intersect);

/* Set the occlusion callback function of a user geometry. */
RTC_API void rtcSetGeometryOccludedFunction(RTCGeometry geometry, RTCOccludedFunctionN occluded);

/* Invokes the intersection filter from the intersection callback function. */
RTC_API void rtcFilterIntersection(const struct RTCIntersectFunctionNArguments* args, const struct RTCFilterFunctionNArguments* filterArgs);

/* Invokes the occlusion filter from the occlusion callback function. */
RTC_API void rtcFilterOcclusion(const struct RTCOccludedFunctionNArguments* args, const struct RTCFilterFunctionNArguments* filterArgs);


/* Sets the instanced scene of an instance geometry. */
RTC_API void rtcSetGeometryInstancedScene(RTCGeometry geometry, RTCScene scene);

/* Sets the transformation of an instance for the specified time step. */
RTC_API void rtcSetGeometryTransform(RTCGeometry geometry, unsigned int timeStep, enum RTCFormat format, const void* xfm);

/* Returns the interpolated transformation of an instance for the specified time. */
RTC_API void rtcGetGeometryTransform(RTCGeometry geometry, float time, enum RTCFormat format, void* xfm);


/* Sets the uniform tessellation rate of the geometry. */
RTC_API void rtcSetGeometryTessellationRate(RTCGeometry geometry, float tessellationRate);

/* Sets the number of topologies of a subdivision surface. */
RTC_API void rtcSetGeometryTopologyCount(RTCGeometry geometry, unsigned int topologyCount);

/* Sets the subdivision interpolation mode. */
RTC_API void rtcSetGeometrySubdivisionMode(RTCGeometry geometry, unsigned int topologyID, enum RTCSubdivisionMode mode);

/* Binds a vertex attribute to a topology of the geometry. */
RTC_API void rtcSetGeometryVertexAttributeTopology(RTCGeometry geometry, unsigned int vertexAttributeID, unsigned int topologyID);

/* Sets the displacement callback function of a subdivision surface. */
RTC_API void rtcSetGeometryDisplacementFunction(RTCGeometry geometry, RTCDisplacementFunctionN displacement);

/* Returns the first half edge of a face. */
RTC_API unsigned int rtcGetGeometryFirstHalfEdge(RTCGeometry geometry, unsigned int faceID);

/* Returns the face the half edge belongs to. */
RTC_API unsigned int rtcGetGeometryFace(RTCGeometry geometry, unsigned int edgeID);

/* Returns next half edge. */
RTC_API unsigned int rtcGetGeometryNextHalfEdge(RTCGeometry geometry, unsigned int edgeID);

/* Returns previous half edge. */
RTC_API unsigned int rtcGetGeometryPreviousHalfEdge(RTCGeometry geometry, unsigned int edgeID);

/* Returns opposite half edge. */
RTC_API unsigned int rtcGetGeometryOppositeHalfEdge(RTCGeometry geometry, unsigned int topologyID, unsigned int edgeID);


/* Arguments for rtcInterpolate */
struct RTCInterpolateArguments
{
  RTCGeometry geometry;
  unsigned int primID;
  float u;
  float v;
  enum RTCBufferType bufferType;
  unsigned int bufferSlot;
  float* P;
  float* dPdu;
  float* dPdv;
  float* ddPdudu;
  float* ddPdvdv;
  float* ddPdudv;
  unsigned int valueCount;
};

/* Interpolates vertex data to some u/v location and optionally calculates all derivatives. */
RTC_API void rtcInterpolate(const struct RTCInterpolateArguments* args);

/* Interpolates vertex data to some u/v location. */
RTC_FORCEINLINE void rtcInterpolate0(RTCGeometry geometry, unsigned int primID, float u, float v, enum RTCBufferType bufferType, unsigned int bufferSlot, float* P, unsigned int valueCount)
{
  struct RTCInterpolateArguments args;
  args.geometry = geometry;
  args.primID = primID;
  args.u = u;
  args.v = v;
  args.bufferType = bufferType;
  args.bufferSlot = bufferSlot;
  args.P = P;
  args.dPdu = NULL;
  args.dPdv = NULL;
  args.ddPdudu = NULL;
  args.ddPdvdv = NULL;
  args.ddPdudv = NULL;
  args.valueCount = valueCount;
  rtcInterpolate(&args);
}

/* Interpolates vertex data to some u/v location and calculates first order derivatives. */
RTC_FORCEINLINE void rtcInterpolate1(RTCGeometry geometry, unsigned int primID, float u, float v, enum RTCBufferType bufferType, unsigned int bufferSlot,
                                     float* P, float* dPdu, float* dPdv, unsigned int valueCount)
{
  struct RTCInterpolateArguments args;
  args.geometry = geometry;
  args.primID = primID;
  args.u = u;
  args.v = v;
  args.bufferType = bufferType;
  args.bufferSlot = bufferSlot;
  args.P = P;
  args.dPdu = dPdu;
  args.dPdv = dPdv;
  args.ddPdudu = NULL;
  args.ddPdvdv = NULL;
  args.ddPdudv = NULL;
  args.valueCount = valueCount;
  rtcInterpolate(&args);
}

/* Interpolates vertex data to some u/v location and calculates first and second order derivatives. */
RTC_FORCEINLINE void rtcInterpolate2(RTCGeometry geometry, unsigned int primID, float u, float v, enum RTCBufferType bufferType, unsigned int bufferSlot,
                                     float* P, float* dPdu, float* dPdv, float* ddPdudu, float* ddPdvdv, float* ddPdudv, unsigned int valueCount)
{
  struct RTCInterpolateArguments args;
  args.geometry = geometry;
  args.primID = primID;
  args.u = u;
  args.v = v;
  args.bufferType = bufferType;
  args.bufferSlot = bufferSlot;
  args.P = P;
  args.dPdu = dPdu;
  args.dPdv = dPdv;
  args.ddPdudu = ddPdudu;
  args.ddPdvdv = ddPdvdv;
  args.ddPdudv = ddPdudv;
  args.valueCount = valueCount;
  rtcInterpolate(&args);
}

/* Arguments for rtcInterpolateN */
struct RTCInterpolateNArguments
{
  RTCGeometry geometry;
  const void* valid;
  const unsigned int* primIDs;
  const float* u;
  const float* v;
  unsigned int N;
  enum RTCBufferType bufferType;
  unsigned int bufferSlot;
  float* P;
  float* dPdu;
  float* dPdv;
  float* ddPdudu;
  float* ddPdvdv;
  float* ddPdudv;
  unsigned int valueCount;
};

/* Interpolates vertex data to an array of u/v locations. */
RTC_API void rtcInterpolateN(const struct RTCInterpolateNArguments* args);

/* RTCGrid primitive for grid mesh */
struct RTCGrid
{
  unsigned int startVertexID;
  unsigned int stride;
  unsigned short width,height; // max is a 32k x 32k grid
};

RTC_NAMESPACE_END


