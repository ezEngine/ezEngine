////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License.  You may obtain a copy
// of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
////////////////////////////////////////////////////////////////////////////////
#ifndef _CRT_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <algorithm>
#ifdef _WIN32
	#include <intrin.h>
#else
	#include <immintrin.h>
#endif

#include "../MaskedOcclusionCulling.h"

////////////////////////////////////////////////////////////////////////////////////////
// Image utility functions, minimal BMP writer and depth buffer tone mapping
////////////////////////////////////////////////////////////////////////////////////////

static void WriteBMP(const char *filename, const unsigned char *data, int w, int h)
{
	short header[] = { 0x4D42, 0, 0, 0, 0, 26, 0, 12, 0, (short)w, (short)h, 1, 24 };
	FILE *f = fopen(filename, "wb");
	fwrite(header, 1, sizeof(header), f);
#if USE_D3D == 1
	// Flip image because Y axis of Direct3D points in the opposite direction of bmp. If the library 
	// is configured for OpenGL (USE_D3D 0) then the Y axes would match and this wouldn't be required.
	for (int y = 0; y < h; ++y)
		fwrite(&data[(h - y - 1) * w * 3], 1, w * 3, f);
#else
	fwrite(data, 1, w * h * 3, f);
#endif
	fclose(f);
}

static void TonemapDepth(float *depth, unsigned char *image, int w, int h)
{
	// Find min/max w coordinate (discard cleared pixels)
	float minW = FLT_MAX, maxW = 0.0f;
	for (int i = 0; i < w*h; ++i)
	{
		if (depth[i] > 0.0f)
		{
			minW = std::min(minW, depth[i]);
			maxW = std::max(maxW, depth[i]);
		}
	}

	// Tonemap depth values
	for (int i = 0; i < w*h; ++i)
	{
		int intensity = 0;
		if (depth[i] > 0)
			intensity = (unsigned char)(223.0*(depth[i] - minW) / (maxW - minW) + 32.0);
			
		image[i * 3 + 0] = intensity;
		image[i * 3 + 1] = intensity;
		image[i * 3 + 2] = intensity;
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// Tutorial example code
////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	// Flush denorms to zero to avoid performance issues with small values
	_mm_setcsr(_mm_getcsr() | 0x8040);

	MaskedOcclusionCulling *moc = MaskedOcclusionCulling::Create();

	////////////////////////////////////////////////////////////////////////////////////////
	// Print which version (instruction set) is being used
	////////////////////////////////////////////////////////////////////////////////////////

	MaskedOcclusionCulling::Implementation implementation = moc->GetImplementation();
	switch (implementation) {
	case MaskedOcclusionCulling::SSE2: printf("Using SSE2 version\n"); break;
	case MaskedOcclusionCulling::SSE41: printf("Using SSE41 version\n"); break;
	case MaskedOcclusionCulling::AVX2: printf("Using AVX2 version\n"); break;
	case MaskedOcclusionCulling::AVX512: printf("Using AVX-512 version\n"); break;
	}

	////////////////////////////////////////////////////////////////////////////////////////
	// Setup and state related code
	////////////////////////////////////////////////////////////////////////////////////////

	// Setup a 1920 x 1080 rendertarget with near clip plane at w = 1.0
	const int width = 1920, height = 1080;
	moc->SetResolution(width, height);
	moc->SetNearClipPlane(1.0f);

	// Clear the depth buffer
	moc->ClearBuffer();

	////////////////////////////////////////////////////////////////////////////////////////
	// Render some occluders
	////////////////////////////////////////////////////////////////////////////////////////
	struct ClipspaceVertex { float x, y, z, w; };

	// A triangle that intersects the view frustum
	ClipspaceVertex triVerts[] = { { 5, 0, 0, 10 }, { 30, 0, 0, 20 }, { 10, 50, 0, 40 } };
	unsigned int triIndices[] = { 0, 1, 2 };

	// Render the triangle
	moc->RenderTriangles((float*)triVerts, triIndices, 1);

	// A clockwise winded (backfacing) triangle
	ClipspaceVertex cwTriVerts[] = { { 7, -7, 0, 20 },{ 7.5, -7, 0, 20 },{ 7, -7.5, 0, 20 } };
	unsigned int cwTriIndices[] = { 0, 1, 2 };

	// Render with counter-clockwise backface culling, the triangle is rendered
	moc->RenderTriangles((float*)cwTriVerts, cwTriIndices, 1, nullptr, MaskedOcclusionCulling::BACKFACE_CCW);

	// A quad completely within the view frustum
	ClipspaceVertex quadVerts[] = { { -150, -150, 0, 200 }, { -10, -65, 0, 75 }, { 0, 0, 0, 20 }, { -40, 10, 0, 50 } };
	unsigned int quadIndices[] = { 0, 1, 2, 0, 2, 3 };

	// Render the quad. As an optimization, indicate that clipping is not required as it is 
	// completely inside the view frustum
	moc->RenderTriangles((float*)quadVerts, quadIndices, 2, nullptr, MaskedOcclusionCulling::BACKFACE_CW, MaskedOcclusionCulling::CLIP_PLANE_NONE);

	// A triangle specified on struct of arrays (SoA) form
	float SoAVerts[] = {
		 10, 10,   7, // x-coordinates
		-10, -7, -10, // y-coordinates
		 10, 10,  10  // w-coordinates
	};

	// Set vertex layout (stride, y offset, w offset)
	MaskedOcclusionCulling::VertexLayout SoAVertexLayout(sizeof(float), 3 * sizeof(float), 6 * sizeof(float));

	// Render triangle with SoA layout
	moc->RenderTriangles((float*)SoAVerts, triIndices, 1, nullptr, MaskedOcclusionCulling::BACKFACE_CW, MaskedOcclusionCulling::CLIP_PLANE_ALL, SoAVertexLayout);


	////////////////////////////////////////////////////////////////////////////////////////
	// Perform some occlusion queries
	////////////////////////////////////////////////////////////////////////////////////////

	// A triangle, partly overlapped by the quad
	ClipspaceVertex oqTriVerts[] = { { 0, 50, 0, 200 }, { -60, -60, 0, 200 }, { 20, -40, 0, 200 } };
	unsigned int oqTriIndices[] = { 0, 1, 2 };

	// Perform an occlusion query. The triangle is visible and the query should return VISIBLE
	MaskedOcclusionCulling::CullingResult result;
	result = moc->TestTriangles((float*)oqTriVerts, oqTriIndices, 1);
	if (result == MaskedOcclusionCulling::VISIBLE)
		printf("Tested triangle is VISIBLE\n");
	else if (result == MaskedOcclusionCulling::OCCLUDED)
		printf("Tested triangle is OCCLUDED\n");
	else if (result == MaskedOcclusionCulling::VIEW_CULLED)
		printf("Tested triangle is outside view frustum\n");

	// Render the occlusion query triangle to show its position
	moc->RenderTriangles((float*)oqTriVerts, oqTriIndices, 1);


	// Perform an occlusion query testing if a rectangle is visible. The rectangle is completely 
	// behind the previously drawn quad, so the query should indicate that it's occluded
	result = moc->TestRect(-0.6f, -0.6f, -0.4f, -0.4f, 100);
	if (result == MaskedOcclusionCulling::VISIBLE)
		printf("Tested rect is VISIBLE\n");
	else if (result == MaskedOcclusionCulling::OCCLUDED)
		printf("Tested rect is OCCLUDED\n");
	else if (result == MaskedOcclusionCulling::VIEW_CULLED)
		printf("Tested rect is outside view frustum\n");

	// Compute a per pixel depth buffer from the hierarchical depth buffer, used for visualization.
	float *perPixelZBuffer = new float[width * height];
	moc->ComputePixelDepthBuffer(perPixelZBuffer, false);

	// Tonemap the image
	unsigned char *image = new unsigned char[width * height * 3];
	TonemapDepth(perPixelZBuffer, image, width, height);
	WriteBMP("image.bmp", image, width, height);
	delete[] image;

	// Destroy occlusion culling object and free hierarchical z-buffer
	MaskedOcclusionCulling::Destroy(moc);

}
