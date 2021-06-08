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
#pragma once
#ifndef VHACD_VHACD_H
#    define VHACD_VHACD_H

#    include "vhacdMutex.h"
#    include "vhacdVolume.h"
#    include "vhacdRaycastMesh.h"
#include <atomic>
#    include <vector>

namespace VHACD
{

class PrimitiveSetBase;

class VHACD : public IVHACD
{
public:
    //! Constructor.
    VHACD()
    {
        Init();
    }
    //! Destructor.
    ~VHACD(void)
    {
    }
    uint32_t GetNConvexHulls() const
    {
        return (uint32_t)m_convexHulls.Size();
    }
    void Cancel()
    {
        SetCancel(true);
    }
    void GetConvexHull(const uint32_t index, ConvexHull& ch) const
    {
        Mesh* mesh = m_convexHulls[index];
        ch.m_nPoints = (uint32_t)mesh->GetNPoints();
        ch.m_nTriangles = (uint32_t)mesh->GetNTriangles();
        ch.m_points = mesh->GetPoints();
        ch.m_triangles = (uint32_t*)mesh->GetTriangles();
        ch.m_volume = mesh->ComputeVolume();
        Vec3<double>& center = mesh->ComputeCenter();
        ch.m_center[0] = center.X();
        ch.m_center[1] = center.Y();
        ch.m_center[2] = center.Z();
    }
    void Clean(void)
    {
        if (mRaycastMesh)
        {
            mRaycastMesh->release();
            mRaycastMesh = nullptr;
        }
        delete m_volume;
        delete m_pset;
        size_t nCH = m_convexHulls.Size();
        for (size_t p = 0; p < nCH; ++p)
        {
            delete m_convexHulls[p];
        }
        m_convexHulls.Clear();
        Init();
    }
    void Release(void)
    {
        delete this;
    }
    bool Compute(const float* const points,
                 const uint32_t nPoints,
                 const uint32_t* const triangles,
                 const uint32_t nTriangles,
                 const Parameters& params);
    bool Compute(const double* const points,
                 const uint32_t nPoints,
                 const uint32_t* const triangles,
                 const uint32_t nTriangles,
                 const Parameters& params);
    bool OCLInit(void* const oclDevice, IUserLogger* const logger = 0);
    bool OCLRelease(IUserLogger* const logger = 0);

    void SimplifyConvexHulls(const Parameters& params);
    void SimplifyConvexHull(Mesh* const ch, const size_t nvertices, const double minVolume);

    virtual bool ComputeCenterOfMass(double centerOfMass[3]) const;

    void ProcessPrimitiveSet(PrimitiveSetBase* pbase, PrimitiveSet* pset);


private:

    void SetCancel(bool cancel)
    {
        m_cancel = cancel;
    }

    bool GetCancel()
    {
        return m_cancel;
    }

    void Update(const double stageProgress, const double operationProgress, const Parameters& params)
    {
        if ( GetCancel() )
        {
            return;
        }
        m_stageProgress = stageProgress;
        m_operationProgress = operationProgress;
        if (params.m_callback)
        {
            params.m_callback->Update(
                m_overallProgress, m_stageProgress, m_operationProgress, m_stage.c_str(), m_operation.c_str());
        }
    }
    void Init()
    {
        if (mRaycastMesh)
        {
            mRaycastMesh->release();
            mRaycastMesh = nullptr;
        }
        m_dim = 64;
        m_volume = 0;
        m_volumeCH0 = 0.0;
        m_pset = 0;
        m_overallProgress = 0.0;
        m_stageProgress = 0.0;
        m_operationProgress = 0.0;
        m_stage = "";
        m_operation = "";
        SetCancel(false);
    }
    void ComputePrimitiveSet(const Parameters& params);
    void ComputeACD(const Parameters& params);


    void MergeConvexHulls(const Parameters& params);
    void ComputeBestClippingPlane(const PrimitiveSet* inputPSet,
                                  const double volume,
                                  const SArray<Plane>& planes,
                                  const Vec3<double>& preferredCuttingDirection,
                                  const double w,
                                  const double alpha,
                                  const double beta,
                                  const int32_t convexhullDownsampling,
                                  Plane& bestPlane,
                                  double& minConcavity,
                                  const Parameters& params);

    void VoxelizeMesh(const double* const points,
                      const uint32_t nPoints,
                      const int32_t* const triangles,
                      const uint32_t nTriangles,
                      const Parameters& params)
    {
        if (GetCancel())
        {
            //printf("EarlyOut:VoxelizeMesh\n");
            return;
        }

        m_timer.Tic();
        m_stage = "Voxelization";

        std::ostringstream msg;
        if (params.m_logger)
        {
            msg << "+ " << m_stage << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }

        delete m_volume;
        m_volume = 0;
        const int32_t maxIteration = 5;
        double progress = 0.0;

        // Default dimensions is the cube root of the resolution provided times the
        // default voxel dimension of 64
        double a = pow((double)(params.m_resolution), 0.33);
        m_dim = (size_t)(a * 1.5);
        // Minimum voxel resolution is 32x32x32
        if (m_dim < 32)
        {
            m_dim = 32;
        }

        {
            char scratch[512];
            snprintf(scratch, sizeof(scratch), "Voxelization step : Dimensions(%d) Resolution:%d", uint32_t(m_dim),
                     uint32_t(params.m_resolution));
            m_operation = std::string(scratch);

            Update(progress, 0.0, params);

            m_volume = new Volume(params);
            m_volume->Voxelize(points, nPoints, triangles, nTriangles, m_dim, params.m_fillMode, mRaycastMesh);

            size_t n = m_volume->GetNPrimitivesOnSurf() + m_volume->GetNPrimitivesInsideSurf();
            if (params.m_logger)
            {
                msg.str("");
                msg << "\t dim = " << m_dim << "\t-> " << n << " voxels" << std::endl;
                params.m_logger->Log(msg.str().c_str());
            }
        }

        m_voxelResolution = m_volume->m_scale;

        m_overallProgress = 10.0;

        Update(100.0, 100.0, params);

        m_timer.Toc();
        if (params.m_logger)
        {
            msg.str("");
            msg << "\t time " << m_timer.GetElapsedTime() / 1000.0 << "s" << std::endl;
            params.m_logger->Log(msg.str().c_str());
        }
    }

    bool ComputeACD(const double* const points,
                    const uint32_t nPoints,
                    const uint32_t* const triangles,
                    const uint32_t nTriangles,
                    const Parameters& params)
    {
        Init();
        if (params.m_projectHullVertices || params.m_fillMode == FillMode::RAYCAST_FILL)
        {
            mRaycastMesh = RaycastMesh::createRaycastMesh(nPoints, points, nTriangles, (const uint32_t*)triangles);
        }
        VoxelizeMesh(points, nPoints, (int32_t*)triangles, nTriangles, params);
        ComputePrimitiveSet(params);
        ComputeACD(params);
        MergeConvexHulls(params);
        SimplifyConvexHulls(params);
        if (GetCancel())
        {
            //printf("EarlyOut:ComputeACD::Clean\n");
            Clean();
            return false;
        }
        return true;
    }

private:
    RaycastMesh* mRaycastMesh{ nullptr };
    SArray<Mesh*> m_convexHulls;
    std::string m_stage;
    std::string m_operation;
    double m_overallProgress;
    double m_stageProgress;
    double m_operationProgress;
    double m_volumeCH0;
    Timer m_timer;
    size_t m_dim;
    double m_voxelResolution{ 1 };
    Volume* m_volume;
    PrimitiveSet* m_pset;
    std::atomic<bool> m_cancel{false};
};
} // namespace VHACD
#endif // VHACD_VHACD_H
