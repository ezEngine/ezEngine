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
#ifndef VHACD_VOLUME_H
#    define VHACD_VOLUME_H

#    include "vhacdMesh.h"
#    include "vhacdVector.h"
#    include "VHACD.h"

#    include <assert.h>

#    ifdef _MSC_VER
#        pragma warning(push)
#        pragma warning(disable : 4456 4701)
#    endif

namespace VHACD
{

class RaycastMesh;

enum VOXEL_VALUE
{
    PRIMITIVE_UNDEFINED = 0,
    PRIMITIVE_OUTSIDE_SURFACE_TOWALK = 1,
    PRIMITIVE_OUTSIDE_SURFACE = 2,
    PRIMITIVE_INSIDE_SURFACE = 3,
    PRIMITIVE_ON_SURFACE = 4
};

struct Voxel
{
public:
    short m_coord[3];
    short m_data;
};

class PrimitiveSet
{
public:
    virtual ~PrimitiveSet(){};
    virtual PrimitiveSet* Create() const = 0;
    virtual const size_t GetNPrimitives() const = 0;
    virtual const size_t GetNPrimitivesOnSurf() const = 0;
    virtual const size_t GetNPrimitivesInsideSurf() const = 0;
    virtual const double GetEigenValue(AXIS axis) const = 0;
    virtual const double ComputeMaxVolumeError() const = 0;
    virtual const double ComputeVolume() const = 0;
    virtual void Clip(const Plane& plane, PrimitiveSet* const positivePart, PrimitiveSet* const negativePart) const = 0;
    virtual void Intersect(const Plane& plane,
                           SArray<Vec3<double>>* const positivePts,
                           SArray<Vec3<double>>* const negativePts,
                           const size_t sampling) const = 0;
    virtual void ComputeExteriorPoints(const Plane& plane,
                                       const Mesh& mesh,
                                       SArray<Vec3<double>>* const exteriorPts) const = 0;
    virtual void ComputeClippedVolumes(const Plane& plane, double& positiveVolume, double& negativeVolume) const = 0;
    virtual void SelectOnSurface(PrimitiveSet* const onSurfP) const = 0;
    virtual void ComputeConvexHull(Mesh& meshCH, const size_t sampling = 1) const = 0;
    virtual void ComputeBB() = 0;
    virtual void ComputePrincipalAxes() = 0;
    virtual void Convert(Mesh& mesh, const VOXEL_VALUE value) const = 0;
    const Mesh& GetConvexHull() const
    {
        return m_convexHull;
    };
    Mesh& GetConvexHull()
    {
        return m_convexHull;
    };

    bool isFirstIteration(void) const
    {
        return mIsFirstIteration;
    }

    void setIsFirstIteration(bool state)
    {
        mIsFirstIteration = state;
    }

private:
    bool mIsFirstIteration{ false };
    Mesh m_convexHull;
};

//!
class VoxelSet : public PrimitiveSet
{
    friend class Volume;

public:
    //! Destructor.
    ~VoxelSet(void);
    //! Constructor.
    VoxelSet();

    const size_t GetNPrimitives() const
    {
        return m_voxels.Size();
    }
    const size_t GetNPrimitivesOnSurf() const
    {
        return m_numVoxelsOnSurface;
    }
    const size_t GetNPrimitivesInsideSurf() const
    {
        return m_numVoxelsInsideSurface;
    }
    const double GetEigenValue(AXIS axis) const
    {
        return m_D[axis][axis];
    }
    const double ComputeVolume() const
    {
        return m_unitVolume * m_voxels.Size();
    }
    const double ComputeMaxVolumeError() const
    {
        return m_unitVolume * m_numVoxelsOnSurface;
    }
    const Vec3<short>& GetMinBBVoxels() const
    {
        return m_minBBVoxels;
    }
    const Vec3<short>& GetMaxBBVoxels() const
    {
        return m_maxBBVoxels;
    }
    const Vec3<double>& GetMinBB() const
    {
        return m_minBB;
    }
    const double& GetScale() const
    {
        return m_scale;
    }
    const double& GetUnitVolume() const
    {
        return m_unitVolume;
    }

    Vec3<double> GetPoint(Vec3<short> voxel) const
    {
        return Vec3<double>(
            voxel[0] * m_scale + m_minBB[0], voxel[1] * m_scale + m_minBB[1], voxel[2] * m_scale + m_minBB[2]);
    }

    Vec3<double> GetPoint(const Voxel& voxel) const
    {
        return Vec3<double>(voxel.m_coord[0] * m_scale + m_minBB[0], voxel.m_coord[1] * m_scale + m_minBB[1],
                            voxel.m_coord[2] * m_scale + m_minBB[2]);
    }

    Vec3<double> GetPoint(Vec3<double> voxel) const
    {
        return Vec3<double>(
            voxel[0] * m_scale + m_minBB[0], voxel[1] * m_scale + m_minBB[1], voxel[2] * m_scale + m_minBB[2]);
    }

    void GetPoints(const Voxel& voxel, Vec3<double>* const pts) const;
    void ComputeConvexHull(Mesh& meshCH, const size_t sampling = 1) const;
    void Clip(const Plane& plane, PrimitiveSet* const positivePart, PrimitiveSet* const negativePart) const;
    void Intersect(const Plane& plane,
                   SArray<Vec3<double>>* const positivePts,
                   SArray<Vec3<double>>* const negativePts,
                   const size_t sampling) const;
    void ComputeExteriorPoints(const Plane& plane, const Mesh& mesh, SArray<Vec3<double>>* const exteriorPts) const;
    void ComputeClippedVolumes(const Plane& plane, double& positiveVolume, double& negativeVolume) const;
    void SelectOnSurface(PrimitiveSet* const onSurfP) const;
    void ComputeBB();
    void Convert(Mesh& mesh, const VOXEL_VALUE value) const;
    void ComputePrincipalAxes();
    PrimitiveSet* Create() const
    {
        return new VoxelSet();
    }
    void AlignToPrincipalAxes(){};
    void RevertAlignToPrincipalAxes(){};
    Voxel* const GetVoxels()
    {
        return m_voxels.Data();
    }
    const Voxel* const GetVoxels() const
    {
        return m_voxels.Data();
    }

private:
    size_t m_numVoxelsOnSurface;
    size_t m_numVoxelsInsideSurface;
    Vec3<double> m_minBB;
    double m_scale;
    SArray<Voxel, 8> m_voxels;
    double m_unitVolume;
    Vec3<double> m_minBBPts;
    Vec3<double> m_maxBBPts;
    Vec3<short> m_minBBVoxels;
    Vec3<short> m_maxBBVoxels;
    Vec3<short> m_barycenter;
    double m_Q[3][3];
    double m_D[3][3];
    Vec3<double> m_barycenterPCA;
};

//!
class Volume
{
public:
    //! Destructor.
    ~Volume(void);

    //! Constructor.
    Volume(const IVHACD::Parameters& params);

    //! Voxelize
    void Voxelize(const double* const points,
                  const uint32_t nPoints,
                  const int32_t* const triangles,
                  const uint32_t nTriangles,
                  const size_t dim,
                  FillMode fillMode,
                  RaycastMesh* raycastMesh);

    void SetVoxel(const size_t i, const size_t j, const size_t k, unsigned char value)
    {
        assert(i < m_dim[0] || i >= 0);
        assert(j < m_dim[0] || j >= 0);
        assert(k < m_dim[0] || k >= 0);
        m_data[k + j * m_dim[2] + i * m_dim[1] * m_dim[2]] = value;
    }

    unsigned char& GetVoxel(const size_t i, const size_t j, const size_t k)
    {
        assert(i < m_dim[0] || i >= 0);
        assert(j < m_dim[0] || j >= 0);
        assert(k < m_dim[0] || k >= 0);
        return m_data[k + j * m_dim[2] + i * m_dim[1] * m_dim[2]];
    }

    const unsigned char& GetVoxel(const size_t i, const size_t j, const size_t k) const
    {
        assert(i < m_dim[0] || i >= 0);
        assert(j < m_dim[0] || j >= 0);
        assert(k < m_dim[0] || k >= 0);
        return m_data[k + j * m_dim[2] + i * m_dim[1] * m_dim[2]];
    }

    const size_t GetNPrimitivesOnSurf() const
    {
        return m_numVoxelsOnSurface;
    }

    const size_t GetNPrimitivesInsideSurf() const
    {
        return m_numVoxelsInsideSurface;
    }

    void Convert(Mesh& mesh, const VOXEL_VALUE value) const;
    void Convert(VoxelSet& vset) const;
    void AlignToPrincipalAxes(double (&rot)[3][3]) const;

    const IVHACD::Parameters& m_params;
    Vec3<double> m_minBB;
    Vec3<double> m_maxBB;
    double m_scale;
    size_t m_dim[3]; //>! dim
    size_t m_numVoxelsOnSurface;
    size_t m_numVoxelsInsideSurface;
    size_t m_numVoxelsOutsideSurface;
    unsigned char* m_data;

private:
    void MarkOutsideSurface(
        const size_t i0, const size_t j0, const size_t k0, const size_t i1, const size_t j1, const size_t k1);
    void FillOutsideSurface();

    void FillInsideSurface();

    void ComputeBB(const double* const points, const uint32_t nPoints);

    void Allocate();
    void Free();
};

void raycastFill(Volume* v, RaycastMesh* raycastMesh);

int32_t TriBoxOverlap(const Vec3<double>& boxcenter,
                      const Vec3<double>& boxhalfsize,
                      const Vec3<double>& triver0,
                      const Vec3<double>& triver1,
                      const Vec3<double>& triver2);

inline void ComputeAlignedPoint(const double* const points, const uint32_t idx, Vec3<double>& pt)
{
    pt[0] = points[idx + 0];
    pt[1] = points[idx + 1];
    pt[2] = points[idx + 2];
}

inline void Volume::ComputeBB(const double* const points, const uint32_t nPoints)
{
    Vec3<double> pt;
    ComputeAlignedPoint(points, 0, pt);
    m_maxBB = pt;
    m_minBB = pt;
    for (uint32_t v = 1; v < nPoints; ++v)
    {
        ComputeAlignedPoint(points, v * 3, pt);
        for (int32_t i = 0; i < 3; ++i)
        {
            if (pt[i] < m_minBB[i])
                m_minBB[i] = pt[i];
            else if (pt[i] > m_maxBB[i])
                m_maxBB[i] = pt[i];
        }
    }
}

inline void Volume::Voxelize(const double* const points,
                             const uint32_t nPoints,
                             const int32_t* const triangles,
                             const uint32_t nTriangles,
                             const size_t dim,
                             FillMode fillMode,
                             RaycastMesh* raycastMesh)
{
    if (nPoints == 0)
    {
        return;
    }
    ComputeBB(points, nPoints);

    double d[3] = { m_maxBB[0] - m_minBB[0], m_maxBB[1] - m_minBB[1], m_maxBB[2] - m_minBB[2] };
    double r;
    // Equal comparison is important here to avoid taking the last branch when d[0] == d[1] with d[2] being the smallest
    // dimension. That would lead to dimensions in i and j to be a lot bigger than expected and make the amount of
    // voxels in the volume totally unmanageable.
    if (d[0] >= d[1] && d[0] >= d[2])
    {
        r = d[0];
        m_dim[0] = dim;
        m_dim[1] = 2 + static_cast<size_t>(dim * d[1] / d[0]);
        m_dim[2] = 2 + static_cast<size_t>(dim * d[2] / d[0]);
    }
    else if (d[1] >= d[0] && d[1] >= d[2])
    {
        r = d[1];
        m_dim[1] = dim;
        m_dim[0] = 2 + static_cast<size_t>(dim * d[0] / d[1]);
        m_dim[2] = 2 + static_cast<size_t>(dim * d[2] / d[1]);
    }
    else
    {
        r = d[2];
        m_dim[2] = dim;
        m_dim[0] = 2 + static_cast<size_t>(dim * d[0] / d[2]);
        m_dim[1] = 2 + static_cast<size_t>(dim * d[1] / d[2]);
    }

    m_scale = r / (dim - 1);
    double invScale = (dim - 1) / r;

    Allocate();
    m_numVoxelsOnSurface = 0;
    m_numVoxelsInsideSurface = 0;
    m_numVoxelsOutsideSurface = 0;

    Vec3<double> p[3];
    size_t i, j, k;
    size_t i0, j0, k0;
    size_t i1, j1, k1;
    Vec3<double> boxcenter;
    Vec3<double> pt;
    const Vec3<double> boxhalfsize(0.5, 0.5, 0.5);
    for (size_t t = 0, ti = 0; t < nTriangles; ++t, ti += 3)
    {
        Vec3<int32_t> tri(triangles[ti + 0], triangles[ti + 1], triangles[ti + 2]);
        for (int32_t c = 0; c < 3; ++c)
        {
            ComputeAlignedPoint(points, tri[c] * 3, pt);
            p[c][0] = (pt[0] - m_minBB[0]) * invScale;
            p[c][1] = (pt[1] - m_minBB[1]) * invScale;
            p[c][2] = (pt[2] - m_minBB[2]) * invScale;
            i = static_cast<size_t>(p[c][0] + 0.5);
            j = static_cast<size_t>(p[c][1] + 0.5);
            k = static_cast<size_t>(p[c][2] + 0.5);
            assert(i < m_dim[0] && i >= 0 && j < m_dim[1] && j >= 0 && k < m_dim[2] && k >= 0);

            if (c == 0)
            {
                i0 = i1 = i;
                j0 = j1 = j;
                k0 = k1 = k;
            }
            else
            {
                if (i < i0)
                    i0 = i;
                if (j < j0)
                    j0 = j;
                if (k < k0)
                    k0 = k;
                if (i > i1)
                    i1 = i;
                if (j > j1)
                    j1 = j;
                if (k > k1)
                    k1 = k;
            }
        }
        if (i0 > 0)
            --i0;
        if (j0 > 0)
            --j0;
        if (k0 > 0)
            --k0;
        if (i1 < m_dim[0])
            ++i1;
        if (j1 < m_dim[1])
            ++j1;
        if (k1 < m_dim[2])
            ++k1;
        for (size_t i = i0; i < i1; ++i)
        {
            boxcenter[0] = (double)i;
            for (size_t j = j0; j < j1; ++j)
            {
                boxcenter[1] = (double)j;
                for (size_t k = k0; k < k1; ++k)
                {
                    boxcenter[2] = (double)k;
                    int32_t res = TriBoxOverlap(boxcenter, boxhalfsize, p[0], p[1], p[2]);
                    unsigned char& value = GetVoxel(i, j, k);
                    if (res == 1 && value == PRIMITIVE_UNDEFINED)
                    {
                        value = PRIMITIVE_ON_SURFACE;
                        ++m_numVoxelsOnSurface;
                    }
                }
            }
        }
    }
    if (fillMode == FillMode::SURFACE_ONLY)
    {
        const size_t i0 = m_dim[0];
        const size_t j0 = m_dim[1];
        const size_t k0 = m_dim[2];
        for (size_t i = 0; i < i0; ++i)
        {
            for (size_t j = 0; j < j0; ++j)
            {
                for (size_t k = 0; k < k0; ++k)
                {
                    const unsigned char& voxel = GetVoxel(i, j, k);
                    if (voxel != PRIMITIVE_ON_SURFACE)
                    {
                        SetVoxel(i, j, k, PRIMITIVE_OUTSIDE_SURFACE);
                    }
                }
            }
        }
    }
    else if (fillMode == FillMode::FLOOD_FILL)
    {
        MarkOutsideSurface(0, 0, 0, m_dim[0], m_dim[1], 1);
        MarkOutsideSurface(0, 0, m_dim[2] - 1, m_dim[0], m_dim[1], m_dim[2]);
        MarkOutsideSurface(0, 0, 0, m_dim[0], 1, m_dim[2]);
        MarkOutsideSurface(0, m_dim[1] - 1, 0, m_dim[0], m_dim[1], m_dim[2]);
        MarkOutsideSurface(0, 0, 0, 1, m_dim[1], m_dim[2]);
        MarkOutsideSurface(m_dim[0] - 1, 0, 0, m_dim[0], m_dim[1], m_dim[2]);
        FillOutsideSurface();
        FillInsideSurface();
    }
    else if (fillMode == FillMode::RAYCAST_FILL)
    {
        raycastFill(this, raycastMesh);
    }
}
} // namespace VHACD

#    ifdef _MSC_VER
#        pragma warning(pop)
#    endif


#endif // VHACD_VOLUME_H
