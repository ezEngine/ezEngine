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

#include "btConvexHullComputer.h"
#include "vhacdVolume.h"
#include "aabbtree.h"
#include "vhacdRaycastMesh.h"

#include <algorithm>
#include <float.h>
#include <math.h>
#include <queue>
#include <string.h>

#ifdef _MSC_VER
#    pragma warning(disable : 4458 4100)
#endif


namespace VHACD
{
/********************************************************/
/* AABB-triangle overlap test code                      */
/* by Tomas Akenine-M�ller                              */
/* Function: int32_t triBoxOverlap(float boxcenter[3],      */
/*          float boxhalfsize[3],float triverts[3][3]); */
/* History:                                             */
/*   2001-03-05: released the code in its first version */
/*   2001-06-18: changed the order of the tests, faster */
/*                                                      */
/* Acknowledgement: Many thanks to Pierre Terdiman for  */
/* suggestions and discussions on how to optimize code. */
/* Thanks to David Hunt for finding a ">="-bug!         */
/********************************************************/

#define X 0
#define Y 1
#define Z 2
#define FINDMINMAX(x0, x1, x2, min, max)                                                                               \
    min = max = x0;                                                                                                    \
    if (x1 < min)                                                                                                      \
        min = x1;                                                                                                      \
    if (x1 > max)                                                                                                      \
        max = x1;                                                                                                      \
    if (x2 < min)                                                                                                      \
        min = x2;                                                                                                      \
    if (x2 > max)                                                                                                      \
        max = x2;

#define AXISTEST_X01(a, b, fa, fb)                                                                                     \
    p0 = a * v0[Y] - b * v0[Z];                                                                                        \
    p2 = a * v2[Y] - b * v2[Z];                                                                                        \
    if (p0 < p2)                                                                                                       \
    {                                                                                                                  \
        min = p0;                                                                                                      \
        max = p2;                                                                                                      \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        min = p2;                                                                                                      \
        max = p0;                                                                                                      \
    }                                                                                                                  \
    rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];                                                                   \
    if (min > rad || max < -rad)                                                                                       \
        return 0;

#define AXISTEST_X2(a, b, fa, fb)                                                                                      \
    p0 = a * v0[Y] - b * v0[Z];                                                                                        \
    p1 = a * v1[Y] - b * v1[Z];                                                                                        \
    if (p0 < p1)                                                                                                       \
    {                                                                                                                  \
        min = p0;                                                                                                      \
        max = p1;                                                                                                      \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        min = p1;                                                                                                      \
        max = p0;                                                                                                      \
    }                                                                                                                  \
    rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];                                                                   \
    if (min > rad || max < -rad)                                                                                       \
        return 0;

#define AXISTEST_Y02(a, b, fa, fb)                                                                                     \
    p0 = -a * v0[X] + b * v0[Z];                                                                                       \
    p2 = -a * v2[X] + b * v2[Z];                                                                                       \
    if (p0 < p2)                                                                                                       \
    {                                                                                                                  \
        min = p0;                                                                                                      \
        max = p2;                                                                                                      \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        min = p2;                                                                                                      \
        max = p0;                                                                                                      \
    }                                                                                                                  \
    rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];                                                                   \
    if (min > rad || max < -rad)                                                                                       \
        return 0;

#define AXISTEST_Y1(a, b, fa, fb)                                                                                      \
    p0 = -a * v0[X] + b * v0[Z];                                                                                       \
    p1 = -a * v1[X] + b * v1[Z];                                                                                       \
    if (p0 < p1)                                                                                                       \
    {                                                                                                                  \
        min = p0;                                                                                                      \
        max = p1;                                                                                                      \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        min = p1;                                                                                                      \
        max = p0;                                                                                                      \
    }                                                                                                                  \
    rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];                                                                   \
    if (min > rad || max < -rad)                                                                                       \
        return 0;

#define AXISTEST_Z12(a, b, fa, fb)                                                                                     \
    p1 = a * v1[X] - b * v1[Y];                                                                                        \
    p2 = a * v2[X] - b * v2[Y];                                                                                        \
    if (p2 < p1)                                                                                                       \
    {                                                                                                                  \
        min = p2;                                                                                                      \
        max = p1;                                                                                                      \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        min = p1;                                                                                                      \
        max = p2;                                                                                                      \
    }                                                                                                                  \
    rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];                                                                   \
    if (min > rad || max < -rad)                                                                                       \
        return 0;

#define AXISTEST_Z0(a, b, fa, fb)                                                                                      \
    p0 = a * v0[X] - b * v0[Y];                                                                                        \
    p1 = a * v1[X] - b * v1[Y];                                                                                        \
    if (p0 < p1)                                                                                                       \
    {                                                                                                                  \
        min = p0;                                                                                                      \
        max = p1;                                                                                                      \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        min = p1;                                                                                                      \
        max = p0;                                                                                                      \
    }                                                                                                                  \
    rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];                                                                   \
    if (min > rad || max < -rad)                                                                                       \
        return 0;

int32_t PlaneBoxOverlap(const Vec3<double>& normal, const Vec3<double>& vert, const Vec3<double>& maxbox)
{
    int32_t q;
    Vec3<double> vmin, vmax;
    double v;
    for (q = X; q <= Z; q++)
    {
        v = vert[q];
        if (normal[q] > 0.0)
        {
            vmin[q] = -maxbox[q] - v;
            vmax[q] = maxbox[q] - v;
        }
        else
        {
            vmin[q] = maxbox[q] - v;
            vmax[q] = -maxbox[q] - v;
        }
    }
    if (normal * vmin > 0.0)
        return 0;
    if (normal * vmax >= 0.0)
        return 1;
    return 0;
}

int32_t TriBoxOverlap(const Vec3<double>& boxcenter,
                      const Vec3<double>& boxhalfsize,
                      const Vec3<double>& triver0,
                      const Vec3<double>& triver1,
                      const Vec3<double>& triver2)
{
    /*    use separating axis theorem to test overlap between triangle and box */
    /*    need to test for overlap in these directions: */
    /*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
    /*       we do not even need to test these) */
    /*    2) normal of the triangle */
    /*    3) crossproduct(edge from tri, {x,y,z}-directin) */
    /*       this gives 3x3=9 more tests */

    Vec3<double> v0, v1, v2;
    double min, max, p0, p1, p2, rad, fex, fey, fez; // -NJMP- "d" local variable removed
    Vec3<double> normal, e0, e1, e2;

    /* This is the fastest branch on Sun */
    /* move everything so that the boxcenter is in (0,0,0) */

    v0 = triver0 - boxcenter;
    v1 = triver1 - boxcenter;
    v2 = triver2 - boxcenter;

    /* compute triangle edges */
    e0 = v1 - v0; /* tri edge 0 */
    e1 = v2 - v1; /* tri edge 1 */
    e2 = v0 - v2; /* tri edge 2 */

    /* Bullet 3:  */
    /*  test the 9 tests first (this was faster) */
    fex = fabs(e0[X]);
    fey = fabs(e0[Y]);
    fez = fabs(e0[Z]);

    AXISTEST_X01(e0[Z], e0[Y], fez, fey);
    AXISTEST_Y02(e0[Z], e0[X], fez, fex);
    AXISTEST_Z12(e0[Y], e0[X], fey, fex);

    fex = fabs(e1[X]);
    fey = fabs(e1[Y]);
    fez = fabs(e1[Z]);

    AXISTEST_X01(e1[Z], e1[Y], fez, fey);
    AXISTEST_Y02(e1[Z], e1[X], fez, fex);
    AXISTEST_Z0(e1[Y], e1[X], fey, fex);

    fex = fabs(e2[X]);
    fey = fabs(e2[Y]);
    fez = fabs(e2[Z]);

    AXISTEST_X2(e2[Z], e2[Y], fez, fey);
    AXISTEST_Y1(e2[Z], e2[X], fez, fex);
    AXISTEST_Z12(e2[Y], e2[X], fey, fex);

    /* Bullet 1: */
    /*  first test overlap in the {x,y,z}-directions */
    /*  find min, max of the triangle each direction, and test for overlap in */
    /*  that direction -- this is equivalent to testing a minimal AABB around */
    /*  the triangle against the AABB */

    /* test in X-direction */
    FINDMINMAX(v0[X], v1[X], v2[X], min, max);
    if (min > boxhalfsize[X] || max < -boxhalfsize[X])
        return 0;

    /* test in Y-direction */
    FINDMINMAX(v0[Y], v1[Y], v2[Y], min, max);
    if (min > boxhalfsize[Y] || max < -boxhalfsize[Y])
        return 0;

    /* test in Z-direction */
    FINDMINMAX(v0[Z], v1[Z], v2[Z], min, max);
    if (min > boxhalfsize[Z] || max < -boxhalfsize[Z])
        return 0;

    /* Bullet 2: */
    /*  test if the box intersects the plane of the triangle */
    /*  compute plane equation of triangle: normal*x+d=0 */
    normal = e0 ^ e1;

    if (!PlaneBoxOverlap(normal, v0, boxhalfsize))
        return 0;
    return 1; /* box and triangle overlaps */
}

// Slightly modified version of  Stan Melax's code for 3x3 matrix diagonalization (Thanks Stan!)
// source: http://www.melax.com/diag.html?attredirects=0
void Diagonalize(const double (&A)[3][3], double (&Q)[3][3], double (&D)[3][3])
{
    // A must be a symmetric matrix.
    // returns Q and D such that
    // Diagonal matrix D = QT * A * Q;  and  A = Q*D*QT
    const int32_t maxsteps = 24; // certainly wont need that many.
    int32_t k0, k1, k2;
    double o[3], m[3];
    double q[4] = { 0.0, 0.0, 0.0, 1.0 };
    double jr[4];
    double sqw, sqx, sqy, sqz;
    double tmp1, tmp2, mq;
    double AQ[3][3];
    double thet, sgn, t, c;
    for (int32_t i = 0; i < maxsteps; ++i)
    {
        // quat to matrix
        sqx = q[0] * q[0];
        sqy = q[1] * q[1];
        sqz = q[2] * q[2];
        sqw = q[3] * q[3];
        Q[0][0] = (sqx - sqy - sqz + sqw);
        Q[1][1] = (-sqx + sqy - sqz + sqw);
        Q[2][2] = (-sqx - sqy + sqz + sqw);
        tmp1 = q[0] * q[1];
        tmp2 = q[2] * q[3];
        Q[1][0] = 2.0 * (tmp1 + tmp2);
        Q[0][1] = 2.0 * (tmp1 - tmp2);
        tmp1 = q[0] * q[2];
        tmp2 = q[1] * q[3];
        Q[2][0] = 2.0 * (tmp1 - tmp2);
        Q[0][2] = 2.0 * (tmp1 + tmp2);
        tmp1 = q[1] * q[2];
        tmp2 = q[0] * q[3];
        Q[2][1] = 2.0 * (tmp1 + tmp2);
        Q[1][2] = 2.0 * (tmp1 - tmp2);

        // AQ = A * Q
        AQ[0][0] = Q[0][0] * A[0][0] + Q[1][0] * A[0][1] + Q[2][0] * A[0][2];
        AQ[0][1] = Q[0][1] * A[0][0] + Q[1][1] * A[0][1] + Q[2][1] * A[0][2];
        AQ[0][2] = Q[0][2] * A[0][0] + Q[1][2] * A[0][1] + Q[2][2] * A[0][2];
        AQ[1][0] = Q[0][0] * A[0][1] + Q[1][0] * A[1][1] + Q[2][0] * A[1][2];
        AQ[1][1] = Q[0][1] * A[0][1] + Q[1][1] * A[1][1] + Q[2][1] * A[1][2];
        AQ[1][2] = Q[0][2] * A[0][1] + Q[1][2] * A[1][1] + Q[2][2] * A[1][2];
        AQ[2][0] = Q[0][0] * A[0][2] + Q[1][0] * A[1][2] + Q[2][0] * A[2][2];
        AQ[2][1] = Q[0][1] * A[0][2] + Q[1][1] * A[1][2] + Q[2][1] * A[2][2];
        AQ[2][2] = Q[0][2] * A[0][2] + Q[1][2] * A[1][2] + Q[2][2] * A[2][2];
        // D = Qt * AQ
        D[0][0] = AQ[0][0] * Q[0][0] + AQ[1][0] * Q[1][0] + AQ[2][0] * Q[2][0];
        D[0][1] = AQ[0][0] * Q[0][1] + AQ[1][0] * Q[1][1] + AQ[2][0] * Q[2][1];
        D[0][2] = AQ[0][0] * Q[0][2] + AQ[1][0] * Q[1][2] + AQ[2][0] * Q[2][2];
        D[1][0] = AQ[0][1] * Q[0][0] + AQ[1][1] * Q[1][0] + AQ[2][1] * Q[2][0];
        D[1][1] = AQ[0][1] * Q[0][1] + AQ[1][1] * Q[1][1] + AQ[2][1] * Q[2][1];
        D[1][2] = AQ[0][1] * Q[0][2] + AQ[1][1] * Q[1][2] + AQ[2][1] * Q[2][2];
        D[2][0] = AQ[0][2] * Q[0][0] + AQ[1][2] * Q[1][0] + AQ[2][2] * Q[2][0];
        D[2][1] = AQ[0][2] * Q[0][1] + AQ[1][2] * Q[1][1] + AQ[2][2] * Q[2][1];
        D[2][2] = AQ[0][2] * Q[0][2] + AQ[1][2] * Q[1][2] + AQ[2][2] * Q[2][2];
        o[0] = D[1][2];
        o[1] = D[0][2];
        o[2] = D[0][1];
        m[0] = fabs(o[0]);
        m[1] = fabs(o[1]);
        m[2] = fabs(o[2]);

        k0 = (m[0] > m[1] && m[0] > m[2]) ? 0 : (m[1] > m[2]) ? 1 : 2; // index of largest element of offdiag
        k1 = (k0 + 1) % 3;
        k2 = (k0 + 2) % 3;
        if (o[k0] == 0.0)
        {
            break; // diagonal already
        }
        thet = (D[k2][k2] - D[k1][k1]) / (2.0 * o[k0]);
        sgn = (thet > 0.0) ? 1.0 : -1.0;
        thet *= sgn; // make it positive
        t = sgn / (thet + ((thet < 1.E6) ? sqrt(thet * thet + 1.0) : thet)); // sign(T)/(|T|+sqrt(T^2+1))
        c = 1.0 / sqrt(t * t + 1.0); //  c= 1/(t^2+1) , t=s/c
        if (c == 1.0)
        {
            break; // no room for improvement - reached machine precision.
        }
        jr[0] = jr[1] = jr[2] = jr[3] = 0.0;
        jr[k0] = sgn * sqrt((1.0 - c) / 2.0); // using 1/2 angle identity sin(a/2) = sqrt((1-cos(a))/2)
        jr[k0] *= -1.0; // since our quat-to-matrix convention was for v*M instead of M*v
        jr[3] = sqrt(1.0 - jr[k0] * jr[k0]);
        if (jr[3] == 1.0)
        {
            break; // reached limits of floating point precision
        }
        q[0] = (q[3] * jr[0] + q[0] * jr[3] + q[1] * jr[2] - q[2] * jr[1]);
        q[1] = (q[3] * jr[1] - q[0] * jr[2] + q[1] * jr[3] + q[2] * jr[0]);
        q[2] = (q[3] * jr[2] + q[0] * jr[1] - q[1] * jr[0] + q[2] * jr[3]);
        q[3] = (q[3] * jr[3] - q[0] * jr[0] - q[1] * jr[1] - q[2] * jr[2]);
        mq = sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
        q[0] /= mq;
        q[1] /= mq;
        q[2] /= mq;
        q[3] /= mq;
    }
}
VoxelSet::VoxelSet()
{
    m_minBB[0] = m_minBB[1] = m_minBB[2] = 0.0;
    m_minBBVoxels[0] = m_minBBVoxels[1] = m_minBBVoxels[2] = 0;
    m_maxBBVoxels[0] = m_maxBBVoxels[1] = m_maxBBVoxels[2] = 1;
    m_minBBPts[0] = m_minBBPts[1] = m_minBBPts[2] = 0;
    m_maxBBPts[0] = m_maxBBPts[1] = m_maxBBPts[2] = 1;
    m_barycenter[0] = m_barycenter[1] = m_barycenter[2] = 0;
    m_barycenterPCA[0] = m_barycenterPCA[1] = m_barycenterPCA[2] = 0.0;
    m_scale = 1.0;
    m_unitVolume = 1.0;
    m_numVoxelsOnSurface = 0;
    m_numVoxelsInsideSurface = 0;
    memset(m_Q, 0, sizeof(double) * 9);
    memset(m_D, 0, sizeof(double) * 9);
}
VoxelSet::~VoxelSet(void)
{
}
void VoxelSet::ComputeBB()
{
    const size_t nVoxels = m_voxels.Size();
    if (nVoxels == 0)
        return;
    for (int32_t h = 0; h < 3; ++h)
    {
        m_minBBVoxels[h] = m_voxels[0].m_coord[h];
        m_maxBBVoxels[h] = m_voxels[0].m_coord[h];
    }
    Vec3<double> bary(0.0);
    for (size_t p = 0; p < nVoxels; ++p)
    {
        for (int32_t h = 0; h < 3; ++h)
        {
            bary[h] += m_voxels[p].m_coord[h];
            if (m_minBBVoxels[h] > m_voxels[p].m_coord[h])
                m_minBBVoxels[h] = m_voxels[p].m_coord[h];
            if (m_maxBBVoxels[h] < m_voxels[p].m_coord[h])
                m_maxBBVoxels[h] = m_voxels[p].m_coord[h];
        }
    }
    bary /= (double)nVoxels;
    for (int32_t h = 0; h < 3; ++h)
    {
        m_minBBPts[h] = m_minBBVoxels[h] * m_scale + m_minBB[h];
        m_maxBBPts[h] = m_maxBBVoxels[h] * m_scale + m_minBB[h];
        m_barycenter[h] = (short)(bary[h] + 0.5);
    }
}
void VoxelSet::ComputeConvexHull(Mesh& meshCH, const size_t sampling) const
{
    const size_t CLUSTER_SIZE = 65536;
    const size_t nVoxels = m_voxels.Size();
    if (nVoxels == 0)
        return;

    SArray<Vec3<double>> cpoints;

    Vec3<double>* points = new Vec3<double>[CLUSTER_SIZE];
    size_t p = 0;
    size_t s = 0;
    short i, j, k;
    while (p < nVoxels)
    {
        size_t q = 0;
        while (q < CLUSTER_SIZE && p < nVoxels)
        {
            if (m_voxels[p].m_data == PRIMITIVE_ON_SURFACE)
            {
                ++s;
                if (s == sampling)
                {
                    s = 0;
                    i = m_voxels[p].m_coord[0];
                    j = m_voxels[p].m_coord[1];
                    k = m_voxels[p].m_coord[2];
                    Vec3<double> p0((i - 0.5) * m_scale, (j - 0.5) * m_scale, (k - 0.5) * m_scale);
                    Vec3<double> p1((i + 0.5) * m_scale, (j - 0.5) * m_scale, (k - 0.5) * m_scale);
                    Vec3<double> p2((i + 0.5) * m_scale, (j + 0.5) * m_scale, (k - 0.5) * m_scale);
                    Vec3<double> p3((i - 0.5) * m_scale, (j + 0.5) * m_scale, (k - 0.5) * m_scale);
                    Vec3<double> p4((i - 0.5) * m_scale, (j - 0.5) * m_scale, (k + 0.5) * m_scale);
                    Vec3<double> p5((i + 0.5) * m_scale, (j - 0.5) * m_scale, (k + 0.5) * m_scale);
                    Vec3<double> p6((i + 0.5) * m_scale, (j + 0.5) * m_scale, (k + 0.5) * m_scale);
                    Vec3<double> p7((i - 0.5) * m_scale, (j + 0.5) * m_scale, (k + 0.5) * m_scale);
                    points[q++] = p0 + m_minBB;
                    points[q++] = p1 + m_minBB;
                    points[q++] = p2 + m_minBB;
                    points[q++] = p3 + m_minBB;
                    points[q++] = p4 + m_minBB;
                    points[q++] = p5 + m_minBB;
                    points[q++] = p6 + m_minBB;
                    points[q++] = p7 + m_minBB;
                }
            }
            ++p;
        }
        btConvexHullComputer ch;
        ch.compute((double*)points, 3 * sizeof(double), (int32_t)q, -1.0, -1.0);
        for (int32_t v = 0; v < ch.vertices.size(); v++)
        {
            cpoints.PushBack(Vec3<double>(ch.vertices[v].getX(), ch.vertices[v].getY(), ch.vertices[v].getZ()));
        }
    }

    delete[] points;

    points = cpoints.Data();
    btConvexHullComputer ch;
    ch.compute((double*)points, 3 * sizeof(double), (int32_t)cpoints.Size(), -1.0, -1.0);
    meshCH.ResizePoints(0);
    meshCH.ResizeTriangles(0);
    for (int32_t v = 0; v < ch.vertices.size(); v++)
    {
        meshCH.AddPoint(Vec3<double>(ch.vertices[v].getX(), ch.vertices[v].getY(), ch.vertices[v].getZ()));
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
            meshCH.AddTriangle(Vec3<int32_t>(a, b, c));
            edge = edge->getNextEdgeOfFace();
            b = c;
            c = edge->getTargetVertex();
        }
    }
}
void VoxelSet::GetPoints(const Voxel& voxel, Vec3<double>* const pts) const
{
    short i = voxel.m_coord[0];
    short j = voxel.m_coord[1];
    short k = voxel.m_coord[2];
    pts[0][0] = (i - 0.5) * m_scale + m_minBB[0];
    pts[1][0] = (i + 0.5) * m_scale + m_minBB[0];
    pts[2][0] = (i + 0.5) * m_scale + m_minBB[0];
    pts[3][0] = (i - 0.5) * m_scale + m_minBB[0];
    pts[4][0] = (i - 0.5) * m_scale + m_minBB[0];
    pts[5][0] = (i + 0.5) * m_scale + m_minBB[0];
    pts[6][0] = (i + 0.5) * m_scale + m_minBB[0];
    pts[7][0] = (i - 0.5) * m_scale + m_minBB[0];
    pts[0][1] = (j - 0.5) * m_scale + m_minBB[1];
    pts[1][1] = (j - 0.5) * m_scale + m_minBB[1];
    pts[2][1] = (j + 0.5) * m_scale + m_minBB[1];
    pts[3][1] = (j + 0.5) * m_scale + m_minBB[1];
    pts[4][1] = (j - 0.5) * m_scale + m_minBB[1];
    pts[5][1] = (j - 0.5) * m_scale + m_minBB[1];
    pts[6][1] = (j + 0.5) * m_scale + m_minBB[1];
    pts[7][1] = (j + 0.5) * m_scale + m_minBB[1];
    pts[0][2] = (k - 0.5) * m_scale + m_minBB[2];
    pts[1][2] = (k - 0.5) * m_scale + m_minBB[2];
    pts[2][2] = (k - 0.5) * m_scale + m_minBB[2];
    pts[3][2] = (k - 0.5) * m_scale + m_minBB[2];
    pts[4][2] = (k + 0.5) * m_scale + m_minBB[2];
    pts[5][2] = (k + 0.5) * m_scale + m_minBB[2];
    pts[6][2] = (k + 0.5) * m_scale + m_minBB[2];
    pts[7][2] = (k + 0.5) * m_scale + m_minBB[2];
}
void VoxelSet::Intersect(const Plane& plane,
                         SArray<Vec3<double>>* const positivePts,
                         SArray<Vec3<double>>* const negativePts,
                         const size_t sampling) const
{
    const size_t nVoxels = m_voxels.Size();
    if (nVoxels == 0)
        return;
    const double d0 = m_scale;
    double d;
    Vec3<double> pts[8];
    Vec3<double> pt;
    Voxel voxel;
    size_t sp = 0;
    size_t sn = 0;
    for (size_t v = 0; v < nVoxels; ++v)
    {
        voxel = m_voxels[v];
        pt = GetPoint(voxel);
        d = plane.m_a * pt[0] + plane.m_b * pt[1] + plane.m_c * pt[2] + plane.m_d;
        //            if      (d >= 0.0 && d <= d0) positivePts->PushBack(pt);
        //            else if (d < 0.0 && -d <= d0) negativePts->PushBack(pt);
        if (d >= 0.0)
        {
            if (d <= d0)
            {
                GetPoints(voxel, pts);
                for (int32_t k = 0; k < 8; ++k)
                {
                    positivePts->PushBack(pts[k]);
                }
            }
            else
            {
                if (++sp == sampling)
                {
                    //                        positivePts->PushBack(pt);
                    GetPoints(voxel, pts);
                    for (int32_t k = 0; k < 8; ++k)
                    {
                        positivePts->PushBack(pts[k]);
                    }
                    sp = 0;
                }
            }
        }
        else
        {
            if (-d <= d0)
            {
                GetPoints(voxel, pts);
                for (int32_t k = 0; k < 8; ++k)
                {
                    negativePts->PushBack(pts[k]);
                }
            }
            else
            {
                if (++sn == sampling)
                {
                    //                        negativePts->PushBack(pt);
                    GetPoints(voxel, pts);
                    for (int32_t k = 0; k < 8; ++k)
                    {
                        negativePts->PushBack(pts[k]);
                    }
                    sn = 0;
                }
            }
        }
    }
}
void VoxelSet::ComputeExteriorPoints(const Plane& plane, const Mesh& mesh, SArray<Vec3<double>>* const exteriorPts) const
{
    const size_t nVoxels = m_voxels.Size();
    if (nVoxels == 0)
        return;
    double d;
    Vec3<double> pt;
    Vec3<double> pts[8];
    Voxel voxel;
    for (size_t v = 0; v < nVoxels; ++v)
    {
        voxel = m_voxels[v];
        pt = GetPoint(voxel);
        d = plane.m_a * pt[0] + plane.m_b * pt[1] + plane.m_c * pt[2] + plane.m_d;
        if (d >= 0.0)
        {
            if (!mesh.IsInside(pt))
            {
                GetPoints(voxel, pts);
                for (int32_t k = 0; k < 8; ++k)
                {
                    exteriorPts->PushBack(pts[k]);
                }
            }
        }
    }
}
void VoxelSet::ComputeClippedVolumes(const Plane& plane, double& positiveVolume, double& negativeVolume) const
{
    negativeVolume = 0.0;
    positiveVolume = 0.0;
    const size_t nVoxels = m_voxels.Size();
    if (nVoxels == 0)
        return;
    double d;
    Vec3<double> pt;
    size_t nPositiveVoxels = 0;
    for (size_t v = 0; v < nVoxels; ++v)
    {
        pt = GetPoint(m_voxels[v]);
        d = plane.m_a * pt[0] + plane.m_b * pt[1] + plane.m_c * pt[2] + plane.m_d;
        nPositiveVoxels += (d >= 0.0);
    }
    size_t nNegativeVoxels = nVoxels - nPositiveVoxels;
    positiveVolume = m_unitVolume * nPositiveVoxels;
    negativeVolume = m_unitVolume * nNegativeVoxels;
}
void VoxelSet::SelectOnSurface(PrimitiveSet* const onSurfP) const
{
    VoxelSet* const onSurf = (VoxelSet*)onSurfP;
    const size_t nVoxels = m_voxels.Size();
    if (nVoxels == 0)
        return;

    for (int32_t h = 0; h < 3; ++h)
    {
        onSurf->m_minBB[h] = m_minBB[h];
    }
    onSurf->m_voxels.Resize(0);
    onSurf->m_scale = m_scale;
    onSurf->m_unitVolume = m_unitVolume;
    onSurf->m_numVoxelsOnSurface = 0;
    onSurf->m_numVoxelsInsideSurface = 0;
    Voxel voxel;
    for (size_t v = 0; v < nVoxels; ++v)
    {
        voxel = m_voxels[v];
        if (voxel.m_data == PRIMITIVE_ON_SURFACE)
        {
            onSurf->m_voxels.PushBack(voxel);
            ++onSurf->m_numVoxelsOnSurface;
        }
    }
}
void VoxelSet::Clip(const Plane& plane, PrimitiveSet* const positivePartP, PrimitiveSet* const negativePartP) const
{
    VoxelSet* const positivePart = (VoxelSet*)positivePartP;
    VoxelSet* const negativePart = (VoxelSet*)negativePartP;
    const size_t nVoxels = m_voxels.Size();
    if (nVoxels == 0)
        return;

    for (int32_t h = 0; h < 3; ++h)
    {
        negativePart->m_minBB[h] = positivePart->m_minBB[h] = m_minBB[h];
    }
    positivePart->m_voxels.Resize(0);
    negativePart->m_voxels.Resize(0);
    positivePart->m_voxels.Allocate(nVoxels);
    negativePart->m_voxels.Allocate(nVoxels);
    negativePart->m_scale = positivePart->m_scale = m_scale;
    negativePart->m_unitVolume = positivePart->m_unitVolume = m_unitVolume;
    negativePart->m_numVoxelsOnSurface = positivePart->m_numVoxelsOnSurface = 0;
    negativePart->m_numVoxelsInsideSurface = positivePart->m_numVoxelsInsideSurface = 0;

    double d;
    Vec3<double> pt;
    Voxel voxel;
    const double d0 = m_scale;
    for (size_t v = 0; v < nVoxels; ++v)
    {
        voxel = m_voxels[v];
        pt = GetPoint(voxel);
        d = plane.m_a * pt[0] + plane.m_b * pt[1] + plane.m_c * pt[2] + plane.m_d;
        if (d >= 0.0)
        {
            if (voxel.m_data == PRIMITIVE_ON_SURFACE || d <= d0)
            {
                voxel.m_data = PRIMITIVE_ON_SURFACE;
                positivePart->m_voxels.PushBack(voxel);
                ++positivePart->m_numVoxelsOnSurface;
            }
            else
            {
                positivePart->m_voxels.PushBack(voxel);
                ++positivePart->m_numVoxelsInsideSurface;
            }
        }
        else
        {
            if (voxel.m_data == PRIMITIVE_ON_SURFACE || -d <= d0)
            {
                voxel.m_data = PRIMITIVE_ON_SURFACE;
                negativePart->m_voxels.PushBack(voxel);
                ++negativePart->m_numVoxelsOnSurface;
            }
            else
            {
                negativePart->m_voxels.PushBack(voxel);
                ++negativePart->m_numVoxelsInsideSurface;
            }
        }
    }
}
void VoxelSet::Convert(Mesh& mesh, const VOXEL_VALUE value) const
{
    const size_t nVoxels = m_voxels.Size();
    if (nVoxels == 0)
        return;
    Voxel voxel;
    Vec3<double> pts[8];
    for (size_t v = 0; v < nVoxels; ++v)
    {
        voxel = m_voxels[v];
        if (voxel.m_data == value)
        {
            GetPoints(voxel, pts);
            int32_t s = (int32_t)mesh.GetNPoints();
            for (int32_t k = 0; k < 8; ++k)
            {
                mesh.AddPoint(pts[k]);
            }
            mesh.AddTriangle(Vec3<int32_t>(s + 0, s + 2, s + 1));
            mesh.AddTriangle(Vec3<int32_t>(s + 0, s + 3, s + 2));
            mesh.AddTriangle(Vec3<int32_t>(s + 4, s + 5, s + 6));
            mesh.AddTriangle(Vec3<int32_t>(s + 4, s + 6, s + 7));
            mesh.AddTriangle(Vec3<int32_t>(s + 7, s + 6, s + 2));
            mesh.AddTriangle(Vec3<int32_t>(s + 7, s + 2, s + 3));
            mesh.AddTriangle(Vec3<int32_t>(s + 4, s + 1, s + 5));
            mesh.AddTriangle(Vec3<int32_t>(s + 4, s + 0, s + 1));
            mesh.AddTriangle(Vec3<int32_t>(s + 6, s + 5, s + 1));
            mesh.AddTriangle(Vec3<int32_t>(s + 6, s + 1, s + 2));
            mesh.AddTriangle(Vec3<int32_t>(s + 7, s + 0, s + 4));
            mesh.AddTriangle(Vec3<int32_t>(s + 7, s + 3, s + 0));
        }
    }
}
void VoxelSet::ComputePrincipalAxes()
{
    const size_t nVoxels = m_voxels.Size();
    if (nVoxels == 0)
        return;
    m_barycenterPCA[0] = m_barycenterPCA[1] = m_barycenterPCA[2] = 0.0;
    for (size_t v = 0; v < nVoxels; ++v)
    {
        Voxel& voxel = m_voxels[v];
        m_barycenterPCA[0] += voxel.m_coord[0];
        m_barycenterPCA[1] += voxel.m_coord[1];
        m_barycenterPCA[2] += voxel.m_coord[2];
    }
    m_barycenterPCA /= (double)nVoxels;

    double covMat[3][3] = { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } };
    double x, y, z;
    for (size_t v = 0; v < nVoxels; ++v)
    {
        Voxel& voxel = m_voxels[v];
        x = voxel.m_coord[0] - m_barycenter[0];
        y = voxel.m_coord[1] - m_barycenter[1];
        z = voxel.m_coord[2] - m_barycenter[2];
        covMat[0][0] += x * x;
        covMat[1][1] += y * y;
        covMat[2][2] += z * z;
        covMat[0][1] += x * y;
        covMat[0][2] += x * z;
        covMat[1][2] += y * z;
    }
    covMat[0][0] /= nVoxels;
    covMat[1][1] /= nVoxels;
    covMat[2][2] /= nVoxels;
    covMat[0][1] /= nVoxels;
    covMat[0][2] /= nVoxels;
    covMat[1][2] /= nVoxels;
    covMat[1][0] = covMat[0][1];
    covMat[2][0] = covMat[0][2];
    covMat[2][1] = covMat[1][2];
    Diagonalize(covMat, m_Q, m_D);
}
Volume::Volume(const IVHACD::Parameters& params) : m_params(params)
{
    m_dim[0] = m_dim[1] = m_dim[2] = 0;
    m_minBB[0] = m_minBB[1] = m_minBB[2] = 0.0;
    m_maxBB[0] = m_maxBB[1] = m_maxBB[2] = 1.0;
    m_numVoxelsOnSurface = 0;
    m_numVoxelsInsideSurface = 0;
    m_numVoxelsOutsideSurface = 0;
    m_scale = 1.0;
    m_data = 0;
}
Volume::~Volume(void)
{
    delete[] m_data;
}
void Volume::Allocate()
{
    delete[] m_data;
    size_t size = m_dim[0] * m_dim[1] * m_dim[2];
    m_data = new unsigned char[size];
    memset(m_data, PRIMITIVE_UNDEFINED, sizeof(unsigned char) * size);
}
void Volume::Free()
{
    delete[] m_data;
    m_data = 0;
}

void Volume::MarkOutsideSurface(
    const size_t i0, const size_t j0, const size_t k0, const size_t i1, const size_t j1, const size_t k1)
{
    VHACD_TRACE_CPUPROFILER_EVENT_SCOPE(m_params.m_profiler, VHACDVolume::MarkOutsideSurface);
    for (size_t i = i0; i < i1; ++i)
    {
        for (size_t j = j0; j < j1; ++j)
        {
            for (size_t k = k0; k < k1; ++k)
            {
                unsigned char& v = GetVoxel(i, j, k);
                if (v == PRIMITIVE_UNDEFINED)
                {
                    v = PRIMITIVE_OUTSIDE_SURFACE_TOWALK;
                }
            }
        }
    }
}

inline void WalkForward(int64_t start, int64_t end, unsigned char* ptr, int64_t stride, int64_t maxDistance)
{
    for (int64_t i = start, count = 0; count < maxDistance && i < end && *ptr == PRIMITIVE_UNDEFINED;
         ++i, ptr += stride, ++count)
    {
        *ptr = PRIMITIVE_OUTSIDE_SURFACE_TOWALK;
    }
}

inline void WalkBackward(int64_t start, int64_t end, unsigned char* ptr, int64_t stride, int64_t maxDistance)
{
    for (int64_t i = start, count = 0; count < maxDistance && i >= end && *ptr == PRIMITIVE_UNDEFINED;
         --i, ptr -= stride, ++count)
    {
        *ptr = PRIMITIVE_OUTSIDE_SURFACE_TOWALK;
    }
}

void Volume::FillOutsideSurface()
{
    VHACD_TRACE_CPUPROFILER_EVENT_SCOPE(m_params.m_profiler, VHACDVolume::FillOutsideSurface);

    size_t voxelsWalked = 0;
    const int64_t i0 = m_dim[0];
    const int64_t j0 = m_dim[1];
    const int64_t k0 = m_dim[2];

    // Avoid striding too far in each direction to stay in L1 cache as much as possible.
    // The cache size required for the walk is roughly (4 * walkDistance * 64) since
    // the k direction doesn't count as it's walking byte per byte directly in a cache lines.
    // ~16k is required for a walk distance of 64 in each directions.
    const size_t walkDistance = 64;

    // using the stride directly instead of calling GetVoxel for each iterations saves
    // a lot of multiplications and pipeline stalls due to data dependencies on imul.
    const size_t istride = &GetVoxel(1, 0, 0) - &GetVoxel(0, 0, 0);
    const size_t jstride = &GetVoxel(0, 1, 0) - &GetVoxel(0, 0, 0);
    const size_t kstride = &GetVoxel(0, 0, 1) - &GetVoxel(0, 0, 0);

    // It might seem counter intuitive to go over the whole voxel range multiple times
    // but since we do the run in memory order, it leaves us with far fewer cache misses
    // than a BFS algorithm and it has the additional benefit of not requiring us to
    // store and manipulate a fifo for recursion that might become huge when the number
    // of voxels is large.
    // This will outperform the BFS algorithm by several orders of magnitude in practice.
    do
    {
        VHACD_TRACE_CPUPROFILER_EVENT_SCOPE(m_params.m_profiler, VHACDVolume::FillOutsideSurface_Iteration);

        voxelsWalked = 0;
        for (int64_t i = 0; i < i0; ++i)
        {
            for (int64_t j = 0; j < j0; ++j)
            {
                for (int64_t k = 0; k < k0; ++k)
                {
                    unsigned char& voxel = GetVoxel(i, j, k);
                    if (voxel == PRIMITIVE_OUTSIDE_SURFACE_TOWALK)
                    {
                        voxelsWalked++;
                        voxel = PRIMITIVE_OUTSIDE_SURFACE;

                        // walk in each direction to mark other voxel that should be walked.
                        // this will generate a 3d pattern that will help the overall
                        // algorithm converge faster while remaining cache friendly.
                        WalkForward(k + 1, k0, &voxel + kstride, kstride, walkDistance);
                        WalkBackward(k - 1, 0, &voxel - kstride, kstride, walkDistance);

                        WalkForward(j + 1, j0, &voxel + jstride, jstride, walkDistance);
                        WalkBackward(j - 1, 0, &voxel - jstride, jstride, walkDistance);

                        WalkForward(i + 1, i0, &voxel + istride, istride, walkDistance);
                        WalkBackward(i - 1, 0, &voxel - istride, istride, walkDistance);
                    }
                }
            }
        }

        m_numVoxelsOutsideSurface += voxelsWalked;
    } while (voxelsWalked != 0);
}


void Volume::FillInsideSurface()
{
    VHACD_TRACE_CPUPROFILER_EVENT_SCOPE(m_params.m_profiler, VHACDVolume::FillInsideSurface);
    const size_t i0 = m_dim[0];
    const size_t j0 = m_dim[1];
    const size_t k0 = m_dim[2];

    for (size_t i = 0; i < i0; ++i)
    {
        for (size_t j = 0; j < j0; ++j)
        {
            for (size_t k = 0; k < k0; ++k)
            {
                unsigned char& v = GetVoxel(i, j, k);
                if (v == PRIMITIVE_UNDEFINED)
                {
                    v = PRIMITIVE_INSIDE_SURFACE;
                    ++m_numVoxelsInsideSurface;
                }
            }
        }
    }
}

void Volume::Convert(Mesh& mesh, const VOXEL_VALUE value) const
{
    VHACD_TRACE_CPUPROFILER_EVENT_SCOPE(m_params.m_profiler, VHACDVolume::Convert);
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
                if (voxel == value)
                {
                    Vec3<double> p0((i - 0.5) * m_scale, (j - 0.5) * m_scale, (k - 0.5) * m_scale);
                    Vec3<double> p1((i + 0.5) * m_scale, (j - 0.5) * m_scale, (k - 0.5) * m_scale);
                    Vec3<double> p2((i + 0.5) * m_scale, (j + 0.5) * m_scale, (k - 0.5) * m_scale);
                    Vec3<double> p3((i - 0.5) * m_scale, (j + 0.5) * m_scale, (k - 0.5) * m_scale);
                    Vec3<double> p4((i - 0.5) * m_scale, (j - 0.5) * m_scale, (k + 0.5) * m_scale);
                    Vec3<double> p5((i + 0.5) * m_scale, (j - 0.5) * m_scale, (k + 0.5) * m_scale);
                    Vec3<double> p6((i + 0.5) * m_scale, (j + 0.5) * m_scale, (k + 0.5) * m_scale);
                    Vec3<double> p7((i - 0.5) * m_scale, (j + 0.5) * m_scale, (k + 0.5) * m_scale);

                    int32_t s = (int32_t)mesh.GetNPoints();

                    mesh.AddPoint(p0 + m_minBB);
                    mesh.AddPoint(p1 + m_minBB);
                    mesh.AddPoint(p2 + m_minBB);
                    mesh.AddPoint(p3 + m_minBB);
                    mesh.AddPoint(p4 + m_minBB);
                    mesh.AddPoint(p5 + m_minBB);
                    mesh.AddPoint(p6 + m_minBB);
                    mesh.AddPoint(p7 + m_minBB);

                    mesh.AddTriangle(Vec3<int32_t>(s + 0, s + 2, s + 1));
                    mesh.AddTriangle(Vec3<int32_t>(s + 0, s + 3, s + 2));
                    mesh.AddTriangle(Vec3<int32_t>(s + 4, s + 5, s + 6));
                    mesh.AddTriangle(Vec3<int32_t>(s + 4, s + 6, s + 7));
                    mesh.AddTriangle(Vec3<int32_t>(s + 7, s + 6, s + 2));
                    mesh.AddTriangle(Vec3<int32_t>(s + 7, s + 2, s + 3));
                    mesh.AddTriangle(Vec3<int32_t>(s + 4, s + 1, s + 5));
                    mesh.AddTriangle(Vec3<int32_t>(s + 4, s + 0, s + 1));
                    mesh.AddTriangle(Vec3<int32_t>(s + 6, s + 5, s + 1));
                    mesh.AddTriangle(Vec3<int32_t>(s + 6, s + 1, s + 2));
                    mesh.AddTriangle(Vec3<int32_t>(s + 7, s + 0, s + 4));
                    mesh.AddTriangle(Vec3<int32_t>(s + 7, s + 3, s + 0));
                }
            }
        }
    }
}

void Volume::Convert(VoxelSet& vset) const
{
    for (int32_t h = 0; h < 3; ++h)
    {
        vset.m_minBB[h] = m_minBB[h];
    }
    vset.m_voxels.Allocate(m_numVoxelsInsideSurface + m_numVoxelsOnSurface);
    vset.m_scale = m_scale;
    vset.m_unitVolume = m_scale * m_scale * m_scale;
    const short i0 = (short)m_dim[0];
    const short j0 = (short)m_dim[1];
    const short k0 = (short)m_dim[2];
    Voxel voxel;
    vset.m_numVoxelsOnSurface = 0;
    vset.m_numVoxelsInsideSurface = 0;
    for (short i = 0; i < i0; ++i)
    {
        for (short j = 0; j < j0; ++j)
        {
            for (short k = 0; k < k0; ++k)
            {
                const unsigned char& value = GetVoxel(i, j, k);
                if (value == PRIMITIVE_INSIDE_SURFACE)
                {
                    voxel.m_coord[0] = i;
                    voxel.m_coord[1] = j;
                    voxel.m_coord[2] = k;
                    voxel.m_data = PRIMITIVE_INSIDE_SURFACE;
                    vset.m_voxels.PushBack(voxel);
                    ++vset.m_numVoxelsInsideSurface;
                }
                else if (value == PRIMITIVE_ON_SURFACE)
                {
                    voxel.m_coord[0] = i;
                    voxel.m_coord[1] = j;
                    voxel.m_coord[2] = k;
                    voxel.m_data = PRIMITIVE_ON_SURFACE;
                    vset.m_voxels.PushBack(voxel);
                    ++vset.m_numVoxelsOnSurface;
                }
            }
        }
    }
}

void traceRay(RaycastMesh* raycastMesh, const double* start, const double* dir, uint32_t& insideCount, uint32_t& outsideCount)
{
    double outT, u, v, w, faceSign;
    uint32_t faceIndex;
    bool hit = raycastMesh->raycast(start, dir, outT, u, v, w, faceSign, faceIndex);
    if (hit)
    {
        if (faceSign >= 0)
        {
            insideCount++;
        }
        else
        {
            outsideCount++;
        }
    }
}

inline void initVec3(double* dest, uint32_t vindex, double x, double y, double z)
{
    dest[vindex * 3 + 0] = x;
    dest[vindex * 3 + 1] = y;
    dest[vindex * 3 + 2] = z;
}

void raycastFill(Volume* volume, RaycastMesh* raycastMesh)
{
    if (!raycastMesh)
    {
        return;
    }

    double scale = float(volume->m_scale);
    double bmin[3];

    bmin[0] = volume->m_minBB[0];
    bmin[1] = volume->m_minBB[1];
    bmin[2] = volume->m_minBB[2];

    const size_t i0 = volume->m_dim[0];
    const size_t j0 = volume->m_dim[1];
    const size_t k0 = volume->m_dim[2];

    for (size_t i = 0; i < i0; ++i)
    {
        for (size_t j = 0; j < j0; ++j)
        {
            for (size_t k = 0; k < k0; ++k)
            {
                const unsigned char& voxel = volume->GetVoxel(i, j, k);
                if (voxel != VHACD::PRIMITIVE_ON_SURFACE)
                {
                    double start[3];
                    start[0] = float(i) * scale + bmin[0];
                    start[1] = float(j) * scale + bmin[1];
                    start[2] = float(k) * scale + bmin[2];
                    uint32_t insideCount = 0;
                    uint32_t outsideCount = 0;

                    double directions[6 * 3];

                    initVec3(directions, 0, 1, 0, 0);
                    initVec3(directions, 1, 1, 0, 0);
                    initVec3(directions, 2, 0, 1, 0);
                    initVec3(directions, 3, 0, -1, 0);
                    initVec3(directions, 4, 0, 0, 1);
                    initVec3(directions, 5, 0, 0, -1);
                    for (uint32_t r = 0; r < 6; r++)
                    {
                        traceRay(raycastMesh, start, &directions[r * 3], insideCount, outsideCount);
                        // Early out if we hit the outside of the mesh
                        if (outsideCount)
                        {
                            break;
                        }
                        // Early out if we accumulated 3 inside hits
                        if (insideCount >= 3)
                        {
                            break;
                        }
                    }

                    if (outsideCount == 0 && insideCount >= 3)
                    {
                        volume->SetVoxel(i, j, k, VHACD::PRIMITIVE_INSIDE_SURFACE);
                    }
                    else
                    {
                        volume->SetVoxel(i, j, k, VHACD::PRIMITIVE_OUTSIDE_SURFACE);
                    }
                }
            }
        }
    }
}

} // namespace VHACD
