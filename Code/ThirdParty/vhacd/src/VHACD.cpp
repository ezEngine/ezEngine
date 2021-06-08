/* Copyright (c) 2011 Khaled Mamou (kmamou at gmail dot com)
 All rights reserved.
 


 


 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 following conditions are met:
 


 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 disclaimer.
 


 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 disclaimer in the documentation and/or other materials provided with the distribution.
 


 3. The names of the contributors may not be used to endorse or promote products derived from this software without
 specific prior written permission.
 


 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _CRT_SECURE_NO_WARNINGS
#    define _CRT_SECURE_NO_WARNINGS
#endif

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <atomic>
#include <mutex>

#include "VHACD.h"
#include "btConvexHullComputer.h"
#include "vhacdICHull.h"
#include "vhacdMesh.h"
#include "vhacdSArray.h"
#include "vhacdTimer.h"
#include "vhacdVHACD.h"
#include "vhacdVector.h"
#include "vhacdVolume.h"
#include "FloatMath.h"
#include "SimpleJobSystem.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define ABS(a) (((a) < 0) ? -(a) : (a))
#define ZSGN(a) (((a) < 0) ? -1 : (a) > 0 ? 1 : 0)
#define MAX_DOUBLE (1.79769e+308)

#ifdef _MSC_VER
#    pragma warning(disable : 4267 4100 4244 4456)
#endif

// Scoped mutex lock
using lock_guard = std::lock_guard<std::mutex>;

inline int32_t FindMinimumElement(const float* const d, float* const m, const int32_t begin, const int32_t end)
{
    int32_t idx = -1;
    float min = (std::numeric_limits<float>::max)();
    for (size_t i = begin; i < size_t(end); ++i)
    {
        if (d[i] < min)
        {
            idx = i;
            min = d[i];
        }
    }

    *m = min;
    return idx;
}

namespace VHACD
{

IVHACD* CreateVHACD(void)
{
    return new VHACD();
}

void VHACD::ComputePrimitiveSet(const Parameters& params)
{
    VHACD_TRACE_CPUPROFILER_EVENT_SCOPE(params.m_profiler, VHACD::ComputePrimitiveSet);
    m_timer.Tic();

    m_stage = "Compute primitive set";
    m_operation = "Convert volume to pset";

    std::ostringstream msg;
    if (params.m_logger)
    {
        msg << "+ " << m_stage << std::endl;
        params.m_logger->Log(msg.str().c_str());
    }

    Update(0.0, 0.0, params);
    VoxelSet* vset = new VoxelSet;
    m_volume->Convert(*vset);
    m_pset = vset;

    delete m_volume;
    m_volume = 0;

    if (params.m_logger)
    {
        msg.str("");
        msg << "\t # primitives               " << m_pset->GetNPrimitives() << std::endl;
        msg << "\t # inside surface           " << m_pset->GetNPrimitivesInsideSurf() << std::endl;
        msg << "\t # on surface               " << m_pset->GetNPrimitivesOnSurf() << std::endl;
        params.m_logger->Log(msg.str().c_str());
    }

    m_overallProgress = 15.0;
    Update(100.0, 100.0, params);
    m_timer.Toc();
    if (params.m_logger)
    {
        msg.str("");
        msg << "\t time " << m_timer.GetElapsedTime() / 1000.0 << "s" << std::endl;
        params.m_logger->Log(msg.str().c_str());
    }
}
bool VHACD::Compute(const double* const points,
                    const uint32_t nPoints,
                    const uint32_t* const triangles,
                    const uint32_t nTriangles,
                    const Parameters& params)
{
    return ComputeACD(points, nPoints, triangles, nTriangles, params);
}

bool VHACD::Compute(const float* const points,
                    const uint32_t nPoints,
                    const uint32_t* const triangles,
                    const uint32_t nTriangles,
                    const Parameters& params)
{
    bool ret = false;

    double* doublePoints = new double[nPoints * 3];
    for (uint32_t i = 0; i < nPoints * 3; i++)
    {
        doublePoints[i] = points[i];
    }
    ret = ComputeACD(doublePoints, nPoints, triangles, nTriangles, params);
    delete[] doublePoints;
    return ret;
}

double ComputePreferredCuttingDirection(const PrimitiveSet* const tset, Vec3<double>& dir)
{
    double ex = tset->GetEigenValue(AXIS_X);
    double ey = tset->GetEigenValue(AXIS_Y);
    double ez = tset->GetEigenValue(AXIS_Z);

    double vx = (ey - ez) * (ey - ez);
    double vy = (ex - ez) * (ex - ez);
    double vz = (ex - ey) * (ex - ey);
    if (vx < vy && vx < vz)
    {
        double e = ey * ey + ez * ez;

        dir[0] = 1.0;
        dir[1] = 0.0;
        dir[2] = 0.0;

        return (e == 0.0) ? 0.0 : 1.0 - vx / e;
    }
    else if (vy < vx && vy < vz)
    {
        double e = ex * ex + ez * ez;

        dir[0] = 0.0;
        dir[1] = 1.0;
        dir[2] = 0.0;

        return (e == 0.0) ? 0.0 : 1.0 - vy / e;
    }
    else
    {
        double e = ex * ex + ey * ey;

        dir[0] = 0.0;
        dir[1] = 0.0;
        dir[2] = 1.0;

        return (e == 0.0) ? 0.0 : 1.0 - vz / e;
    }
}

void ComputeAxesAlignedClippingPlanes(const VoxelSet& vset, const short downsampling, SArray<Plane>& planes)
{
    const Vec3<short> minV = vset.GetMinBBVoxels();
    const Vec3<short> maxV = vset.GetMaxBBVoxels();
    Vec3<double> pt;
    Plane plane;
    const short i0 = minV[0];
    const short i1 = maxV[0];
    plane.m_a = 1.0;
    plane.m_b = 0.0;
    plane.m_c = 0.0;
    plane.m_axis = AXIS_X;
    for (short i = i0; i <= i1; i += downsampling)
    {
        pt = vset.GetPoint(Vec3<double>(i + 0.5, 0.0, 0.0));
        plane.m_d = -pt[0];
        plane.m_index = i;
        planes.PushBack(plane);
    }
    const short j0 = minV[1];
    const short j1 = maxV[1];
    plane.m_a = 0.0;
    plane.m_b = 1.0;
    plane.m_c = 0.0;
    plane.m_axis = AXIS_Y;
    for (short j = j0; j <= j1; j += downsampling)
    {
        pt = vset.GetPoint(Vec3<double>(0.0, j + 0.5, 0.0));
        plane.m_d = -pt[1];
        plane.m_index = j;
        planes.PushBack(plane);
    }
    const short k0 = minV[2];
    const short k1 = maxV[2];
    plane.m_a = 0.0;
    plane.m_b = 0.0;
    plane.m_c = 1.0;
    plane.m_axis = AXIS_Z;
    for (short k = k0; k <= k1; k += downsampling)
    {
        pt = vset.GetPoint(Vec3<double>(0.0, 0.0, k + 0.5));
        plane.m_d = -pt[2];
        plane.m_index = k;
        planes.PushBack(plane);
    }
}

void RefineAxesAlignedClippingPlanes(const VoxelSet& vset,
                                     const Plane& bestPlane,
                                     const short downsampling,
                                     SArray<Plane>& planes)
{
    const Vec3<short> minV = vset.GetMinBBVoxels();
    const Vec3<short> maxV = vset.GetMaxBBVoxels();
    Vec3<double> pt;
    Plane plane;

    if (bestPlane.m_axis == AXIS_X)
    {
        const short i0 = MAX(minV[0], bestPlane.m_index - downsampling);
        const short i1 = MIN(maxV[0], bestPlane.m_index + downsampling);
        plane.m_a = 1.0;
        plane.m_b = 0.0;
        plane.m_c = 0.0;
        plane.m_axis = AXIS_X;
        for (short i = i0; i <= i1; ++i)
        {
            pt = vset.GetPoint(Vec3<double>(i + 0.5, 0.0, 0.0));
            plane.m_d = -pt[0];
            plane.m_index = i;
            planes.PushBack(plane);
        }
    }
    else if (bestPlane.m_axis == AXIS_Y)
    {
        const short j0 = MAX(minV[1], bestPlane.m_index - downsampling);
        const short j1 = MIN(maxV[1], bestPlane.m_index + downsampling);
        plane.m_a = 0.0;
        plane.m_b = 1.0;
        plane.m_c = 0.0;
        plane.m_axis = AXIS_Y;
        for (short j = j0; j <= j1; ++j)
        {
            pt = vset.GetPoint(Vec3<double>(0.0, j + 0.5, 0.0));
            plane.m_d = -pt[1];
            plane.m_index = j;
            planes.PushBack(plane);
        }
    }
    else
    {
        const short k0 = MAX(minV[2], bestPlane.m_index - downsampling);
        const short k1 = MIN(maxV[2], bestPlane.m_index + downsampling);
        plane.m_a = 0.0;
        plane.m_b = 0.0;
        plane.m_c = 1.0;
        plane.m_axis = AXIS_Z;
        for (short k = k0; k <= k1; ++k)
        {
            pt = vset.GetPoint(Vec3<double>(0.0, 0.0, k + 0.5));
            plane.m_d = -pt[2];
            plane.m_index = k;
            planes.PushBack(plane);
        }
    }
}

inline double ComputeLocalConcavity(const double volume, const double volumeCH)
{
    return fabs(volumeCH - volume) / volumeCH;
}
inline double ComputeConcavity(const double volume, const double volumeCH, const double volume0)
{
    return fabs(volumeCH - volume) / volume0;
}

void VHACD::ComputeBestClippingPlane(const PrimitiveSet* inputPSet,
                                     const double volume,
                                     const SArray<Plane>& planes,
                                     const Vec3<double>& preferredCuttingDirection,
                                     const double w,
                                     const double alpha,
                                     const double beta,
                                     const int32_t convexhullDownsampling,
                                     Plane& bestPlane,
                                     double& minConcavity,
                                     const Parameters& params)
{
    VHACD_TRACE_CPUPROFILER_EVENT_SCOPE(params.m_profiler, VHACD::ComputeBestClippingPlane)
    char msg[256];
    int32_t iBest = -1;
    int32_t nPlanes = static_cast<int32_t>(planes.Size());
    int32_t done = 0;
    double minTotal = MAX_DOUBLE;
    double minBalance = MAX_DOUBLE;
    double minSymmetry = MAX_DOUBLE;
    minConcavity = MAX_DOUBLE;

    SArray<Vec3<double>>* chPts = new SArray<Vec3<double>>[2];
    Mesh* chs = new Mesh[2];
    PrimitiveSet* onSurfacePSet = inputPSet->Create();
    inputPSet->SelectOnSurface(onSurfacePSet);

    PrimitiveSet** psets = 0;
    if (!params.m_convexhullApproximation)
    {
        psets = new PrimitiveSet*[2];
        for (int32_t i = 0; i < 2; ++i)
        {
            psets[i] = inputPSet->Create();
        }
    }

    for (int32_t x = 0; x < nPlanes; ++x)
    {
        {
            Plane plane = planes[x];
            Mesh& leftCH = chs[0];
            Mesh& rightCH = chs[1];
            rightCH.ResizePoints(0);
            leftCH.ResizePoints(0);
            rightCH.ResizeTriangles(0);
            leftCH.ResizeTriangles(0);

            // compute convex-hulls
            if (params.m_convexhullApproximation)
            {
                SArray<Vec3<double>>& leftCHPts = chPts[0];
                SArray<Vec3<double>>& rightCHPts = chPts[1];
                rightCHPts.Resize(0);
                leftCHPts.Resize(0);
                onSurfacePSet->Intersect(plane, &rightCHPts, &leftCHPts, convexhullDownsampling * 32);
                inputPSet->GetConvexHull().Clip(plane, rightCHPts, leftCHPts);
                rightCH.ComputeConvexHull((double*)rightCHPts.Data(), rightCHPts.Size());
                leftCH.ComputeConvexHull((double*)leftCHPts.Data(), leftCHPts.Size());
            }
            else
            {
                PrimitiveSet* const right = psets[0];
                PrimitiveSet* const left = psets[1];
                onSurfacePSet->Clip(plane, right, left);
                right->ComputeConvexHull(rightCH, convexhullDownsampling);
                left->ComputeConvexHull(leftCH, convexhullDownsampling);
            }
            double volumeLeftCH = leftCH.ComputeVolume();
            double volumeRightCH = rightCH.ComputeVolume();

            // compute clipped volumes
            double volumeLeft = 0.0;
            double volumeRight = 0.0;
            inputPSet->ComputeClippedVolumes(plane, volumeRight, volumeLeft);
            double concavityLeft = ComputeConcavity(volumeLeft, volumeLeftCH, m_volumeCH0);
            double concavityRight = ComputeConcavity(volumeRight, volumeRightCH, m_volumeCH0);
            double concavity = (concavityLeft + concavityRight);

            // compute cost
            double balance = alpha * fabs(volumeLeft - volumeRight) / m_volumeCH0;
            double d = w * (preferredCuttingDirection[0] * plane.m_a + preferredCuttingDirection[1] * plane.m_b +
                            preferredCuttingDirection[2] * plane.m_c);
            double symmetry = beta * d;
            double total = concavity + balance + symmetry;

            {
                if (total < minTotal || (total == minTotal && x < iBest))
                {
                    minConcavity = concavity;
                    minBalance = balance;
                    minSymmetry = symmetry;
                    bestPlane = plane;
                    minTotal = total;
                    iBest = x;
                }
                ++done;
            }
        }
    }

    if (psets)
    {
        for (int32_t i = 0; i < 2; ++i)
        {
            delete psets[i];
        }
        delete[] psets;
    }
    delete onSurfacePSet;
    delete[] chPts;
    delete[] chs;
    if (params.m_logger && !GetCancel())
    {
        sprintf(msg, "\n\t\t\t Best  %04i T=%2.6f C=%2.6f B=%2.6f S=%2.6f (%1.1f, %1.1f, %1.1f, %3.3f)\n\n", iBest,
                minTotal, minConcavity, minBalance, minSymmetry, bestPlane.m_a, bestPlane.m_b, bestPlane.m_c,
                bestPlane.m_d);
        params.m_logger->Log(msg);
    }
}

class PrimitiveSetBase
{
public:
    double getConcavity(void) const
    {
        return mParameters->m_concavity;
    }

    double getAlpha(void) const
    {
        return mParameters->m_alpha;
    }

    double getBeta(void) const
    {
        return mParameters->m_beta;
    }

    uint32_t getConvexHullDownsampling(void) const
    {
        return mParameters->m_convexhullDownsampling;
    }

    uint32_t getPlaneDownsampling(void) const
    {
        return mParameters->m_planeDownsampling;
    }

    void refreshConcavity(double minConcavity)
    {
        lock_guard _lock(mMaxConcavityMutex);
        if (*mMaxConcavity < minConcavity)
        {
            *mMaxConcavity = minConcavity;
        }
    }

    void pushTemp(PrimitiveSet* pset)
    {
        lock_guard _lock(mTempMutex);
        mTemp->PushBack(pset);
    }

    void pushParts(PrimitiveSet* pset)
    {
        lock_guard _lock(mPartsMutex);
        mParts->PushBack(pset);
    }

    std::mutex mMaxConcavityMutex;
    const VHACD::Parameters* mParameters{ nullptr };
    std::mutex mPartsMutex;
    SArray<PrimitiveSet*>* mParts;
    std::mutex mTempMutex;
    SArray<PrimitiveSet*>* mTemp;
    double* mMaxConcavity;
    VHACD* mVHACD;
};


void VHACD::ProcessPrimitiveSet(PrimitiveSetBase* pbase, PrimitiveSet* pset)
{
    double volume = pset->ComputeVolume(); // Compute the volume for this primitive set

    pset->ComputeBB(); // Compute the bounding box for this primitive set.

    pset->ComputePrincipalAxes(); // Compute the principle axes.

    pset->ComputeConvexHull(pset->GetConvexHull()); // Generate the convex hull for this primitive set.

    // Compute the volume of the convex hull
    double volumeCH = fabs(pset->GetConvexHull().ComputeVolume());
    // If this is the first iteration, store the volume of the base
    if (pset->isFirstIteration())
    {
        m_volumeCH0 = volumeCH;
    }

    // Compute the concavity of this volume
    double concavity = ComputeConcavity(volume, volumeCH, m_volumeCH0);
    // Compute the volume error
    // double error = 1.01 * pset->ComputeMaxVolumeError() / m_volumeCH0;
    if (concavity > pbase->getConcavity())
    {
        Vec3<double> preferredCuttingDirection;

        double w = ComputePreferredCuttingDirection(pset, preferredCuttingDirection);

        SArray<Plane> planes;
        SArray<Plane> planesRef;
        VoxelSet* vset = (VoxelSet*)pset;

        ComputeAxesAlignedClippingPlanes(*vset, pbase->getPlaneDownsampling(), planes);

        Plane bestPlane;

        double minConcavity = MAX_DOUBLE;

        ComputeBestClippingPlane(pset, volume, planes, preferredCuttingDirection, w, concavity * pbase->getAlpha(),
                                 concavity * pbase->getBeta(), pbase->getConvexHullDownsampling(), bestPlane,
                                 minConcavity, *pbase->mParameters);

        if ((pbase->getPlaneDownsampling() > 1 || pbase->getConvexHullDownsampling() > 1))
        {
            VoxelSet* vset = (VoxelSet*)pset;
            RefineAxesAlignedClippingPlanes(*vset, bestPlane, pbase->getPlaneDownsampling(), planesRef);
            ComputeBestClippingPlane(pset, volume, planesRef, preferredCuttingDirection, w,
                                     concavity * pbase->getAlpha(), concavity * pbase->getBeta(),
                                     1, // convexhullDownsampling = 1
                                     bestPlane, minConcavity, *pbase->mParameters);
        }
        pbase->refreshConcavity(minConcavity);

        PrimitiveSet* bestLeft = pset->Create();
        PrimitiveSet* bestRight = pset->Create();

        pbase->pushTemp(bestLeft);
        pbase->pushTemp(bestRight);

        pset->Clip(bestPlane, bestRight, bestLeft);
        delete pset;
    }
    else
    {
        pbase->pushParts(pset);
    }
}

class PrimitiveSetPending
{
public:
    void execute(void)
    {
        mBase->mVHACD->ProcessPrimitiveSet(mBase, mPrimitiveSet);
    }

    PrimitiveSet* mPrimitiveSet;
    PrimitiveSetBase* mBase;
};

void primitiveSetPendingCallback(void* userPtr)
{
    PrimitiveSetPending* p = (PrimitiveSetPending*)userPtr;
    p->execute();
}

void VHACD::ComputeACD(const Parameters& params)
{
    if (GetCancel())
    {
        // printf("EarlyOut:ComputeACD\n");
        return;
    }
    VHACD_TRACE_CPUPROFILER_EVENT_SCOPE(params.m_profiler, VHACD::ComputeACD)

    m_timer.Tic();

    m_stage = "Approximate Convex Decomposition";
    m_stageProgress = 0.0;
    std::ostringstream msg;
    if (params.m_logger)
    {
        msg << "+ " << m_stage << std::endl;
        params.m_logger->Log(msg.str().c_str());
    }

    SArray<PrimitiveSet*> inputParts;
    SArray<PrimitiveSet*> parts;
    SArray<PrimitiveSet*> temp;
    inputParts.PushBack(m_pset);
    m_pset = 0;
    SArray<Plane> planes;
    SArray<Plane> planesRef;
    uint32_t sub = 0;
    bool firstIteration = true;
    m_volumeCH0 = 1.0;

    // Compute the decomposition depth based on the number of convex hulls being requested..
    uint32_t hullCount = 2;
    uint32_t depth = 1;

    while (params.m_maxConvexHulls > hullCount)
    {
        depth++;
        hullCount *= 2;
    }

    // We must always increment the decomposition depth one higher than the maximum number of hulls requested.
    // The reason for this is as follows.
    // Say, for example, the user requests 32 convex hulls exactly.  This would be a decomposition depth of 5.
    // However, when we do that, we do *not* necessarily get 32 hulls as a result.  This is because, during
    // the recursive descent of the binary tree, one or more of the leaf nodes may have no concavity and
    // will not be split.  So, in this way, even with a decomposition depth of 5, you can produce fewer than
    // 32 hulls.  So, in this case, we would set the decomposition depth to 6 (producing up to as high as 64 convex
    // hulls). Then, the merge step which combines over-described hulls down to the user requested amount, we will end
    // up getting exactly 32 convex hulls as a result. We could just allow the artist to directly control the
    // decomposition depth directly, but this would be a bit too complex and the preference is simply to let them
    // specify how many hulls they want and derive the solution from that.
    depth++;
    // Create a 16 thread job system, will divvy up the ACD work across 16 different threads in hopes
    // of achieving maximum parallelism
    simplejobsystem::SimpleJobSystem* sjs =
        params.m_asyncACD ? simplejobsystem::SimpleJobSystem::create(16, params.m_taskRunner) : nullptr;

    while (sub++ < depth && inputParts.Size() > 0)
    {
        VHACD_TRACE_CPUPROFILER_EVENT_SCOPE(params.m_profiler, VHACD::ComputeACD_Iteration)
        msg.str("");
        msg << "Subdivision level " << sub;
        m_operation = msg.str();

        if (params.m_logger)
        {
            msg.str("");
            msg << "\t Subdivision level " << sub << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }

        double maxConcavity = 0.0;
        const size_t nInputParts = inputParts.Size();

        Update(m_stageProgress, 0.0, params);

        PrimitiveSetBase pbase;
        pbase.mParameters = &params;
        pbase.mParts = &parts;
        pbase.mTemp = &temp;
        pbase.mMaxConcavity = &maxConcavity;
        pbase.mVHACD = this;

        PrimitiveSetPending* pending = new PrimitiveSetPending[nInputParts];
        for (size_t i = 0; i < nInputParts; ++i)
        {
            PrimitiveSetPending& p = pending[i];
            p.mBase = &pbase;
            p.mPrimitiveSet = inputParts[i];
            if (firstIteration)
            {
                p.mPrimitiveSet->setIsFirstIteration(true);
                firstIteration = false;
            }
        }

        // Add the jobs
        if (sjs)
        {
            for (size_t i = 0; i < nInputParts; ++i)
            {
                PrimitiveSetPending* psp = &pending[i];
                sjs->addJob(psp, primitiveSetPendingCallback);
            }
            if (sjs->startJobs())
            {
                sjs->waitForJobsToComplete();
            }
        }
        else
        {
            for (size_t i = 0; i < nInputParts; ++i)
            {
                PrimitiveSetPending* psp = &pending[i];
                psp->execute();
            }
        }


        delete[] pending;

        Update(95.0 * (1.0 - maxConcavity) / (1.0 - params.m_concavity), 100.0, params);

        inputParts = temp;
        temp.Resize(0);

        if (GetCancel())
        {
            break;
        }
    }

    if (sjs)
    {
        sjs->release();
    }

    const size_t nInputParts = inputParts.Size();
    for (size_t p = 0; p < nInputParts; ++p)
    {
        parts.PushBack(inputParts[p]);
    }

    if (GetCancel())
    {
        // printf("EarlyOut:ComputeACD::removeParts\n");
        const size_t nParts = parts.Size();
        for (size_t p = 0; p < nParts; ++p)
        {
            delete parts[p];
        }
        return;
    }

    m_overallProgress = 90.0;
    Update(m_stageProgress, 100.0, params);

    msg.str("");
    msg << "Generate convex-hulls";
    m_operation = msg.str();
    size_t nConvexHulls = parts.Size();
    if (params.m_logger)
    {
        msg.str("");
        msg << "+ Generate " << nConvexHulls << " convex-hulls " << std::endl;
        params.m_logger->Log(msg.str().c_str());
    }

    Update(m_stageProgress, 0.0, params);
    m_convexHulls.Resize(0);
    for (size_t p = 0; p < nConvexHulls && !m_cancel; ++p)
    {
        Update(m_stageProgress, p * 100.0 / nConvexHulls, params);
        m_convexHulls.PushBack(new Mesh);
        parts[p]->ComputeConvexHull(*m_convexHulls[p]);
    }

    const size_t nParts = parts.Size();
    for (size_t p = 0; p < nParts; ++p)
    {
        delete parts[p];
        parts[p] = 0;
    }
    parts.Resize(0);

    if (GetCancel() && 0)
    {
        // printf("EarlyOut:ComputeACD:clearConvexHullResults\n");
        const size_t nConvexHulls = m_convexHulls.Size();
        for (size_t p = 0; p < nConvexHulls; ++p)
        {
            delete m_convexHulls[p];
        }
        m_convexHulls.Clear();
        return;
    }

    m_overallProgress = 95.0;
    Update(100.0, 100.0, params);
    m_timer.Toc();

    if (params.m_logger)
    {
        msg.str("");
        msg << "\t time " << m_timer.GetElapsedTime() / 1000.0 << "s" << std::endl;
        params.m_logger->Log(msg.str().c_str());
    }
}
void AddPoints(const Mesh* const mesh, SArray<Vec3<double>>& pts)
{
    const int32_t n = (int32_t)mesh->GetNPoints();
    for (int32_t i = 0; i < n; ++i)
    {
        pts.PushBack(mesh->GetPoint(i));
    }
}
void ComputeConvexHull(const Mesh* const ch1, const Mesh* const ch2, SArray<Vec3<double>>& pts, Mesh* const combinedCH)
{
    pts.Resize(0);
    AddPoints(ch1, pts);
    AddPoints(ch2, pts);

    btConvexHullComputer ch;
    ch.compute((double*)pts.Data(), 3 * sizeof(double), (int32_t)pts.Size(), -1.0, -1.0);
    combinedCH->ResizePoints(0);
    combinedCH->ResizeTriangles(0);
    for (int32_t v = 0; v < ch.vertices.size(); v++)
    {
        combinedCH->AddPoint(Vec3<double>(ch.vertices[v].getX(), ch.vertices[v].getY(), ch.vertices[v].getZ()));
    }
    const int32_t nt = ch.faces.size();
    for (int32_t t = 0; t < nt; ++t)
    {
        const btConvexHullComputer::Edge* sourceEdge = &(ch.edges[ch.faces[t]]);
        int32_t a = sourceEdge->getSourceVertex();
        int32_t b = sourceEdge->getTargetVertex();
        const btConvexHullComputer::Edge* edge = sourceEdge->getNextEdgeOfFace();
        int32_t c = edge->getTargetVertex();
        while (c != a)
        {
            combinedCH->AddTriangle(Vec3<int32_t>(a, b, c));
            edge = edge->getNextEdgeOfFace();
            b = c;
            c = edge->getTargetVertex();
        }
    }
}

void VHACD::SimplifyConvexHull(Mesh* const ch, const size_t nvertices, const double minVolume)
{
    if (nvertices <= 4)
    {
        return;
    }
    ICHull icHull;

    if (mRaycastMesh)
    {
        // We project these points onto the original source mesh to increase precision
        // The voxelization process drops floating point precision so returned data points are not exactly lying on the
        // surface of the original source mesh.
        // The first step is we need to compute the bounding box of the mesh we are trying to build a convex hull for.
        // From this bounding box, we compute the length of the diagonal to get a relative size and center for point
        // projection
        uint32_t nPoints = ch->GetNPoints();
        Vec3<double>* inputPoints = ch->GetPointsBuffer();
        Vec3<double> bmin(inputPoints[0]);
        Vec3<double> bmax(inputPoints[1]);
        for (uint32_t i = 1; i < nPoints; i++)
        {
            const Vec3<double>& p = inputPoints[i];
            p.UpdateMinMax(bmin, bmax);
        }
        Vec3<double> center;
        double diagonalLength = center.GetCenter(bmin, bmax); // Get the center of the bounding box

        // This is the error threshold for determining if we should use the raycast result data point vs. the voxelized
        // result.
        double pointDistanceThreshold = m_voxelResolution * 4; // can only snap if the point is within 2 times the voxel
                                                               // grid resolution

        // If a new point is within 1/100th the diagonal length of the bounding volume we do not add it.  To do so would
        // create a thin sliver in the resulting convex hull
        double snapDistanceThreshold = diagonalLength * 0.01;
        double snapDistanceThresholdSquared = snapDistanceThreshold * snapDistanceThreshold;

        // Allocate buffer for projected vertices
        Vec3<double>* outputPoints = new Vec3<double>[nPoints];
        uint32_t outCount = 0;
        for (uint32_t i = 0; i < nPoints; i++)
        {
            Vec3<double>& inputPoint = inputPoints[i];
            Vec3<double>& outputPoint = outputPoints[outCount];
            // Compute the direction vector from the center of this mesh to the vertex
            Vec3<double> dir = inputPoint - center;
            // Normalize the direction vector.
            dir.Normalize();
            // Multiply times the diagonal length of the mesh
            dir *= diagonalLength;
            // Add the center back in again to get the destination point
            dir += center;
            // By default the output point is equal to the input point
            outputPoint = inputPoint;
            double closestPoint[3];
            // Find the closest point on the source mesh which is within the voxel resolution. If found
            // we 'snap' the convex hull input point to this surface mesh point.
            if (mRaycastMesh->getClosestPointWithinDistance(&inputPoint[0], pointDistanceThreshold, closestPoint))
            {
                outputPoint[0] = closestPoint[0];
                outputPoint[1] = closestPoint[1];
                outputPoint[2] = closestPoint[2];
            }
            // Ok, before we add this point, we do not want to create points which are extremely close to each other.
            // This will result in tiny sliver triangles which are really bad for collision detection.
            bool foundNearbyPoint = false;
            for (uint32_t j = 0; j < outCount; j++)
            {
                // If this new point is extremely close to an existing point, we do not add it!
                double squaredDistance = outputPoints[j].GetDistanceSquared(outputPoint);
                if (squaredDistance < snapDistanceThresholdSquared)
                {
                    foundNearbyPoint = true;
                    break;
                }
            }
            if (!foundNearbyPoint)
            {
                outCount++;
            }
        }
        icHull.AddPoints(outputPoints, outCount);
        delete[] outputPoints;
    }
    else
    {
        icHull.AddPoints(ch->GetPointsBuffer(), ch->GetNPoints());
    }
    icHull.Process((uint32_t)nvertices, minVolume);
    TMMesh& mesh = icHull.GetMesh();
    const size_t nT = mesh.GetNTriangles();
    const size_t nV = mesh.GetNVertices();
    ch->ResizePoints(nV);
    ch->ResizeTriangles(nT);
    mesh.GetIFS(ch->GetPointsBuffer(), ch->GetTrianglesBuffer());
}

void VHACD::SimplifyConvexHulls(const Parameters& params)
{
    if (m_cancel || params.m_maxNumVerticesPerCH < 4)
    {
        return;
    }
    VHACD_TRACE_CPUPROFILER_EVENT_SCOPE(params.m_profiler, VHACD::SimplifyConvexHulls)
    m_timer.Tic();

    m_stage = "Simplify convex-hulls";
    m_operation = "Simplify convex-hulls";

    std::ostringstream msg;
    const size_t nConvexHulls = m_convexHulls.Size();
    if (params.m_logger)
    {
        msg << "+ Simplify " << nConvexHulls << " convex-hulls " << std::endl;
        params.m_logger->Log(msg.str().c_str());
    }

    Update(0.0, 0.0, params);
    for (size_t i = 0; i < nConvexHulls && !m_cancel; ++i)
    {
        if (params.m_logger)
        {
            msg.str("");
            msg << "\t\t Simplify CH[" << std::setfill('0') << std::setw(5) << i << "] "
                << m_convexHulls[i]->GetNPoints() << " V, " << m_convexHulls[i]->GetNTriangles() << " T" << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }
        VHACD_TRACE_CPUPROFILER_EVENT_SCOPE(params.m_profiler, VHACD::SimplifyConvexHull)
        SimplifyConvexHull(m_convexHulls[i], params.m_maxNumVerticesPerCH, m_volumeCH0 * params.m_minVolumePerCH);
    }

    m_overallProgress = 100.0;
    Update(100.0, 100.0, params);
    m_timer.Toc();
    if (params.m_logger)
    {
        msg.str("");
        msg << "\t time " << m_timer.GetElapsedTime() / 1000.0 << "s" << std::endl;
        params.m_logger->Log(msg.str().c_str());
    }
}


void VHACD::MergeConvexHulls(const Parameters& params)
{
    if (GetCancel())
    {
        // printf("EarlyOut::MergeConvexHulls\n");
        return;
    }
    m_timer.Tic();

    m_stage = "Merge Convex Hulls";

    std::ostringstream msg;
    if (params.m_logger)
    {
        msg << "+ " << m_stage << std::endl;
        params.m_logger->Log(msg.str().c_str());
    }

    // Get the current number of convex hulls
    size_t nConvexHulls = m_convexHulls.Size();
    // Iteration counter
    int32_t iteration = 0;
    // While we have more than at least one convex hull and the user has not asked us to cancel the operation
    if (nConvexHulls > 1 && !m_cancel)
    {
        // Get the gamma error threshold for when to exit
        SArray<Vec3<double>> pts;
        Mesh combinedCH;

        // Populate the cost matrix
        size_t idx = 0;
        SArray<float> costMatrix;
        costMatrix.Resize(((nConvexHulls * nConvexHulls) - nConvexHulls) >> 1);
        for (size_t p1 = 1; p1 < nConvexHulls; ++p1)
        {
            const float volume1 = m_convexHulls[p1]->ComputeVolume();
            for (size_t p2 = 0; p2 < p1; ++p2)
            {
                ComputeConvexHull(m_convexHulls[p1], m_convexHulls[p2], pts, &combinedCH);
                costMatrix[idx++] = ComputeConcavity(
                    volume1 + m_convexHulls[p2]->ComputeVolume(), combinedCH.ComputeVolume(), m_volumeCH0);
            }
        }

        // Until we cant merge below the maximum cost
        size_t costSize = m_convexHulls.Size();
        while (!m_cancel)
        {
            msg.str("");
            msg << "Iteration " << iteration++;
            m_operation = msg.str();

            // Search for lowest cost
            float bestCost = (std::numeric_limits<float>::max)();
            const size_t addr = FindMinimumElement(costMatrix.Data(), &bestCost, 0, costMatrix.Size());
            if ((costSize - 1) < params.m_maxConvexHulls)
            {
                break;
            }
            const size_t addrI = (static_cast<int32_t>(sqrt(1 + (8 * addr))) - 1) >> 1;
            const size_t p1 = addrI + 1;
            const size_t p2 = addr - ((addrI * (addrI + 1)) >> 1);
            assert(p1 >= 0);
            assert(p2 >= 0);
            assert(p1 < costSize);
            assert(p2 < costSize);

            if (params.m_logger)
            {
                msg.str("");
                msg << "\t\t Merging (" << p1 << ", " << p2 << ") " << bestCost << std::endl << std::endl;
                params.m_logger->Log(msg.str().c_str());
            }

            // Make the lowest cost row and column into a new hull
            Mesh* cch = new Mesh;
            ComputeConvexHull(m_convexHulls[p1], m_convexHulls[p2], pts, cch);
            delete m_convexHulls[p2];
            m_convexHulls[p2] = cch;

            delete m_convexHulls[p1];
            std::swap(m_convexHulls[p1], m_convexHulls[m_convexHulls.Size() - 1]);
            m_convexHulls.PopBack();

            costSize = costSize - 1;

            // Calculate costs versus the new hull
            size_t rowIdx = ((p2 - 1) * p2) >> 1;
            const float volume1 = m_convexHulls[p2]->ComputeVolume();
            for (size_t i = 0; (i < p2) && (!m_cancel); ++i)
            {
                ComputeConvexHull(m_convexHulls[p2], m_convexHulls[i], pts, &combinedCH);
                costMatrix[rowIdx++] = ComputeConcavity(
                    volume1 + m_convexHulls[i]->ComputeVolume(), combinedCH.ComputeVolume(), m_volumeCH0);
            }

            rowIdx += p2;
            for (size_t i = p2 + 1; (i < costSize) && (!m_cancel); ++i)
            {
                ComputeConvexHull(m_convexHulls[p2], m_convexHulls[i], pts, &combinedCH);
                costMatrix[rowIdx] = ComputeConcavity(
                    volume1 + m_convexHulls[i]->ComputeVolume(), combinedCH.ComputeVolume(), m_volumeCH0);
                rowIdx += i;
                assert(rowIdx >= 0);
            }

            // Move the top column in to replace its space
            const size_t erase_idx = ((costSize - 1) * costSize) >> 1;
            if (p1 < costSize)
            {
                rowIdx = (addrI * p1) >> 1;
                size_t top_row = erase_idx;
                for (size_t i = 0; i < p1; ++i)
                {
                    if (i != p2)
                    {
                        costMatrix[rowIdx] = costMatrix[top_row];
                    }
                    ++rowIdx;
                    ++top_row;
                }

                ++top_row;
                rowIdx += p1;
                for (size_t i = p1 + 1; i < (costSize + 1); ++i)
                {
                    costMatrix[rowIdx] = costMatrix[top_row++];
                    rowIdx += i;
                    assert(rowIdx >= 0);
                }
            }
            costMatrix.Resize(erase_idx);
        }
    }
    m_overallProgress = 99.0;
    Update(100.0, 100.0, params);
    m_timer.Toc();
    if (params.m_logger)
    {
        msg.str("");
        msg << "\t time " << m_timer.GetElapsedTime() / 1000.0 << "s" << std::endl;
        params.m_logger->Log(msg.str().c_str());
    }
}

bool VHACD::ComputeCenterOfMass(double centerOfMass[3]) const
{
    bool ret = false;

    centerOfMass[0] = 0;
    centerOfMass[1] = 0;
    centerOfMass[2] = 0;
    // Get number of convex hulls in the result
    uint32_t hullCount = GetNConvexHulls();
    if (hullCount) // if we have results
    {
        ret = true;
        double totalVolume = 0;
        // Initialize the center of mass to zero
        centerOfMass[0] = 0;
        centerOfMass[1] = 0;
        centerOfMass[2] = 0;
        // Compute the total volume of all convex hulls
        for (uint32_t i = 0; i < hullCount; i++)
        {
            ConvexHull ch;
            GetConvexHull(i, ch);
            totalVolume += ch.m_volume;
        }
        // compute the reciprocal of the total volume
        double recipVolume = 1.0 / totalVolume;
        // Add in the weighted by volume average of the center point of each convex hull
        for (uint32_t i = 0; i < hullCount; i++)
        {
            ConvexHull ch;
            GetConvexHull(i, ch);
            double ratio = ch.m_volume * recipVolume;
            centerOfMass[0] += ch.m_center[0] * ratio;
            centerOfMass[1] += ch.m_center[1] * ratio;
            centerOfMass[2] += ch.m_center[2] * ratio;
        }
    }
    return ret;
}

} // namespace VHACD
