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
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <vector>
#include <chrono>
#include <random>
#ifdef _WIN32
	#include <intrin.h>
#else
	#include <immintrin.h>
#endif

#include "../CullingThreadpool.h"
#include "../MaskedOcclusionCulling.h"

std::mt19937 gRnd;
std::uniform_real_distribution<float> gRndUniform(0, 1);

#define F_PI 3.14159265358979323846f

// if 1, makes some of the random triangles slightly out of screen and enables clipping
#define TEST_CLIPPING 0

#ifdef _WIN32
////////////////////////////////////////////////////////////////////////////////////////
// DX 11 setup & resource creation code
////////////////////////////////////////////////////////////////////////////////////////

#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

ID3D11Texture2D				*textureZ;
ID3D11Texture2D				*textureCol, *staging;
ID3D11DepthStencilView		*textureDSV;
ID3D11RenderTargetView		*textureRTV;
ID3D11Device				*device;
ID3D11DeviceContext			*context;

ID3D11RasterizerState		*rastState;
ID3D11DepthStencilState		*DSState;
ID3D11InputLayout			*layout;
ID3D11VertexShader			*VS;
ID3D11PixelShader			*PS;
std::vector<ID3D11Buffer*>	vBuffers;
ID3D11Query					*endQuery;

#define D3DVERIFY(X) if (X != S_OK) exit(1);

void InitD3D(unsigned int width, unsigned int height)
{
	const char *shader =
		"float4 VShader(float4 position : POSITION) : SV_POSITION { return position; }"
		"float4 PShader(float4 position : SV_POSITION) : SV_TARGET { return 1.0f - position.z; }";

	D3D_FEATURE_LEVEL fLevel;
	D3DVERIFY(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, &fLevel, &context));

	D3D11_TEXTURE2D_DESC tDesc;
	tDesc.Width = width;
	tDesc.Height = height;
	tDesc.MipLevels = tDesc.ArraySize = 1;
	tDesc.SampleDesc.Count = 1;
	tDesc.SampleDesc.Quality = 0;
	tDesc.CPUAccessFlags = 0;
	tDesc.MiscFlags = 0;
	tDesc.Usage = D3D11_USAGE_DEFAULT;

	tDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
	D3DVERIFY(device->CreateTexture2D(&tDesc, nullptr, &textureCol));

	tDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	tDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	D3DVERIFY(device->CreateTexture2D(&tDesc, nullptr, &textureZ));

	tDesc.Usage = D3D11_USAGE_STAGING;
	tDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tDesc.BindFlags = 0;
	tDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	D3DVERIFY(device->CreateTexture2D(&tDesc, nullptr, &staging));

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = 0;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	D3DVERIFY(device->CreateDepthStencilView(textureZ, &dsvDesc, &textureDSV));

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	D3DVERIFY(device->CreateRenderTargetView(textureCol, &rtvDesc, &textureRTV));

	// Set the viewport
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (float)width;
	viewport.Height = (float)height;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
	context->RSSetViewports(1, &viewport);

	// load and compile the two shaders
	ID3D10Blob *VSBlob, *PSBlob;
	D3DVERIFY(D3DCompile(shader, strlen(shader), "shader", nullptr, nullptr, "VShader", "vs_5_0", 0, 0, &VSBlob, nullptr));
	D3DVERIFY(D3DCompile(shader, strlen(shader), "shader", nullptr, nullptr, "PShader", "ps_5_0", 0, 0, &PSBlob, nullptr));

	// encapsulate both shaders into shader objects
	D3DVERIFY(device->CreateVertexShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), NULL, &VS));
	D3DVERIFY(device->CreatePixelShader(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(), NULL, &PS));

	// set the shader objects
	context->VSSetShader(VS, 0, 0);
	context->PSSetShader(PS, 0, 0);

	// create the input layout object
	D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	D3DVERIFY(device->CreateInputLayout(ied, 1, VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), &layout));
	context->IASetInputLayout(layout);

	D3D11_RASTERIZER_DESC rDesc;
	rDesc.FillMode = D3D11_FILL_SOLID;
	rDesc.CullMode = D3D11_CULL_NONE;
	rDesc.FrontCounterClockwise = false;
	rDesc.DepthBias = 0;
	rDesc.DepthBiasClamp = 0;
	rDesc.SlopeScaledDepthBias = 0;
	rDesc.DepthClipEnable = false;
	rDesc.ScissorEnable = false;
	rDesc.MultisampleEnable = false;
	rDesc.AntialiasedLineEnable = false;
	device->CreateRasterizerState(&rDesc, &rastState);
	context->RSSetState(rastState);

	D3D11_DEPTH_STENCIL_DESC dsDesc;
	ZeroMemory(&dsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dsDesc.DepthEnable = true;
	dsDesc.StencilEnable = false;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	device->CreateDepthStencilState(&dsDesc, &DSState);
	context->OMSetDepthStencilState(DSState, 0);

	D3D11_QUERY_DESC qDesc;
	qDesc.Query = D3D11_QUERY_EVENT;
	qDesc.MiscFlags = 0;
	D3DVERIFY(device->CreateQuery(&qDesc, &endQuery));
}

void D3DAddTriangles(float *verts, int nTris)
{
	ID3D11Buffer *buf = nullptr;

	D3D11_SUBRESOURCE_DATA iData;
	iData.pSysMem = verts;
	iData.SysMemPitch = 0;
	iData.SysMemSlicePitch = 0;

	D3D11_BUFFER_DESC bDesc;
	bDesc.Usage = D3D11_USAGE_DEFAULT;
	bDesc.ByteWidth = nTris * 3 * 4 * sizeof(float);
	bDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bDesc.CPUAccessFlags = bDesc.MiscFlags = bDesc.StructureByteStride = 0;
	D3DVERIFY(device->CreateBuffer(&bDesc, &iData, &buf));
	vBuffers.push_back(buf);
}

double BenchmarkTrianglesD3D(ID3D11Buffer *buf, int numTriangles, bool color)
{
	// set the render target as the back buffer
	if (color)
		context->OMSetRenderTargets(1, &textureRTV, textureDSV);
	else
		context->OMSetRenderTargets(0, nullptr, textureDSV);

	// clear the back buffer to a deep blue
	float clearColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
	context->ClearRenderTargetView(textureRTV, clearColor);
	context->ClearDepthStencilView(textureDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Setup primitivelist
	UINT stride = sizeof(float) * 4, offset = 0;
	context->IASetVertexBuffers(0, 1, &buf, &stride, &offset);
	context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Flush();

	// Draw triangles
	auto before = std::chrono::high_resolution_clock::now();
	context->Begin(endQuery);
	context->Draw(numTriangles * 3, 0);
	context->End(endQuery);
	while (context->GetData(endQuery, nullptr, 0, 0) == S_FALSE) {}
	auto after = std::chrono::high_resolution_clock::now();

	return std::chrono::duration<double>(after - before).count();
}

#endif

////////////////////////////////////////////////////////////////////////////////////////
// Simple random triangle rasterizer benchmark
////////////////////////////////////////////////////////////////////////////////////////

inline float frand() { return (float)rand() / (float)RAND_MAX; }

void GenerateRandomTriangles(float *verts, unsigned int *triIdxBtF, int nTris, int size, float width, float height)
{
    // make some of the triangles slightly out of screen to test clipping paths
#if TEST_CLIPPING
    const float clippingMul = 1.1f;
#else
    const float clippingMul = 1.0f;
#endif

	for (int idx = 0; idx < nTris; ++idx)
	{
		triIdxBtF[idx * 3 + 0] = idx * 3 + 0;
		triIdxBtF[idx * 3 + 1] = idx * 3 + 1;
		triIdxBtF[idx * 3 + 2] = idx * 3 + 2;

		while (true)
		{
			float vtx[3][3] = { { 0, 0, 1 },{ size * 2 / (float)width, 0, 1 },{ 0, size * 2 / (float)height, 1 } };
			float offset[3] = { gRndUniform(gRnd)*2.0f - 1.0f, gRndUniform(gRnd)*2.0f - 1.0f, 0 };
			float rotation = gRndUniform(gRnd) * 2 * F_PI;

			float myz = (float)(nTris - idx);
			bool triOk = true;
			float rvtx[3][3];
			for (int i = 0; i < 3; ++i)
			{
				rvtx[i][0] = cos(rotation)*vtx[i][0] - sin(rotation)*vtx[i][1] + offset[0];
				rvtx[i][1] = sin(rotation)*vtx[i][0] + cos(rotation)*vtx[i][1] + offset[1];

				if (rvtx[i][0] < -1.0f || rvtx[i][0] > 1.0f || rvtx[i][1] < -1.0f || rvtx[i][1] > 1.0f)
					triOk = false;

				float z = myz / (float)nTris;

				int vtxIdx = idx * 3 + i;
				verts[vtxIdx * 4 + 0] = rvtx[i][0] * myz * clippingMul;
				verts[vtxIdx * 4 + 1] = rvtx[i][1] * myz * clippingMul;
				verts[vtxIdx * 4 + 2] = z * myz;
				verts[vtxIdx * 4 + 3] = myz;
			}
			if (triOk)
				break;
		}
	}
}

double BenchmarkTriangles(float *verts, unsigned int *tris, int numTriangles, MaskedOcclusionCulling *moc)
{
	moc->ClearBuffer();

	auto before = std::chrono::high_resolution_clock::now();
#if TEST_CLIPPING
    moc->RenderTriangles( verts, tris, numTriangles, nullptr, MaskedOcclusionCulling::BACKFACE_CW, MaskedOcclusionCulling::CLIP_PLANE_ALL );
#else
	moc->RenderTriangles(verts, tris, numTriangles, nullptr, MaskedOcclusionCulling::BACKFACE_CW, MaskedOcclusionCulling::CLIP_PLANE_NONE);
#endif
	auto after = std::chrono::high_resolution_clock::now();

	return std::chrono::duration<double>(after - before).count();
}

double BenchmarkTrianglesThreaded(float *verts, unsigned int *tris, int numTriangles, CullingThreadpool *ctp) 
{
	ctp->ClearBuffer();

	auto before = std::chrono::high_resolution_clock::now();
#if TEST_CLIPPING
    ctp->RenderTriangles(verts, tris, numTriangles, MaskedOcclusionCulling::BACKFACE_CW, MaskedOcclusionCulling::CLIP_PLANE_ALL);
#else
    ctp->RenderTriangles( verts, tris, numTriangles, MaskedOcclusionCulling::BACKFACE_CW, MaskedOcclusionCulling::CLIP_PLANE_NONE );
#endif
	ctp->Flush();
	auto after = std::chrono::high_resolution_clock::now();

	return std::chrono::duration<double>(after - before).count();
}

////////////////////////////////////////////////////////////////////////////////////////
// Perform basic fillrate benchmarks for GPU and compare with this libray
////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	const int width = 1920, height = 1080;

	// Flush denorms to zero to avoid performance issues with small values
	_mm_setcsr(_mm_getcsr() | 0x8040);

	MaskedOcclusionCulling *moc = MaskedOcclusionCulling::Create();


#ifdef _WIN32
	// Initialize directx
	InitD3D(width, height);
#endif

	////////////////////////////////////////////////////////////////////////////////////////
	// Setup and state related code
	////////////////////////////////////////////////////////////////////////////////////////

	// Setup a rendertarget with near clip plane at w = 1.0
	moc->SetResolution(width, height);
	moc->SetNearClipPlane(1.0f);

	////////////////////////////////////////////////////////////////////////////////////////
	// Create randomized triangles for back-to-front and front-to-back rendering
	////////////////////////////////////////////////////////////////////////////////////////

	const int numTriangles[] = { 4096 * 1024, 4096 * 1024, 4096 * 1024, 2048 * 1024, 1024 * 1024, 512 * 1024, 256 * 1024 };
	const int sizes[] = { 10, 25, 50, 75, 100, 250, 500 };

	int numSizes = sizeof(sizes) / sizeof(int);

	printf("Generating randomized triangles");
	std::vector<unsigned int *> trisBtF;
	std::vector<float *>		verts;
	for (int i = 0; i < numSizes; ++i)
	{
		float *pVerts = new float[numTriangles[i] * 4 * 3];
		unsigned int *pTrisBtF = new unsigned int[numTriangles[i] * 3];
		GenerateRandomTriangles(pVerts, pTrisBtF, numTriangles[i], sizes[i], (float)width, (float)height);
		verts.push_back(pVerts);
		trisBtF.push_back(pTrisBtF);
#ifdef _WIN32
		D3DAddTriangles(pVerts, numTriangles[i]);
#endif
		printf(".");
	}

	////////////////////////////////////////////////////////////////////////////////////////
	// Perform benchmarks
	////////////////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
	printf("\nD3D Z only\n");
	printf("----\n");
	for (int i = 0; i < numSizes; ++i)
	{
		int size = sizes[i];
		double t = BenchmarkTrianglesD3D(vBuffers[i], numTriangles[i], false);
		double GPixelsPerSecond = (double)numTriangles[i] * size*size / (2.0 * 1e9 * t);
		double MTrisPerSecond = (double)numTriangles[i] / (1e6 * t);
		printf("Tri: %3dx%3d - Time: %7.2f ms, MTris/s: %6.2f GPixels/s: %5.2f \n", size, size, t * 1000.0f, MTrisPerSecond, GPixelsPerSecond);
	}
#endif

    printf( "\nInstruction set:" );
    switch( moc->GetImplementation( ) )
    {
    case MaskedOcclusionCulling::SSE2:
        printf( "SSE2\n" );
        break;
    case MaskedOcclusionCulling::SSE41:
        printf( "SSE41\n" );
        break;
    case MaskedOcclusionCulling::AVX2:
        printf( "AVX2\n" );
        break;
    case MaskedOcclusionCulling::AVX512:
        printf( "AVX512\n\n" );
        break;
    default:
        break;

    };

#if 0 && MOC_RECORDER_ENABLE
    moc->RecorderStart( "..\\FrameRecorderPlayer\\FillrateTest.mocrec" );
#endif

	printf("\n\nMasked single threaded\n");
	printf("----\n");
	for (int i = 0; i < numSizes; ++i)
	{
		int size = sizes[i];
		double t = BenchmarkTriangles(verts[i], trisBtF[i], numTriangles[i], moc);
		double GPixelsPerSecond = (double)numTriangles[i] * size*size / (2.0 * 1e9 * t);
		double MTrisPerSecond = (double)numTriangles[i] / (1e6 * t);
		printf("Tri: %3dx%3d - Time: %7.2f ms, MTris/s: %6.2f GPixels/s: %5.2f \n", size, size, t * 1000.0f, MTrisPerSecond, GPixelsPerSecond);
	}

#if 0 && MOC_RECORDER_ENABLE
    moc->RecorderStop( );
#endif

	int numThreads = std::thread::hardware_concurrency() - 1;
	printf("\n\nMasked multi threaded (%d threads)\n", numThreads);
	printf("----\n");
	CullingThreadpool ctp(numThreads, 2, numThreads);
	ctp.SetBuffer(moc);
	ctp.WakeThreads();
	for (int i = 0; i < numSizes; ++i)
	{
		int size = sizes[i];
		double t = BenchmarkTrianglesThreaded(verts[i], trisBtF[i], numTriangles[i], &ctp);
		double GPixelsPerSecond = (double)numTriangles[i] * size*size / (2.0 * 1e9 * t);
		double MTrisPerSecond = (double)numTriangles[i] / (1e6 * t);
		printf("Tri: %3dx%3d - Time: %7.2f ms, MTris/s: %6.2f GPixels/s: %5.2f \n", size, size, t * 1000.0f, MTrisPerSecond, GPixelsPerSecond);
	}
	ctp.SuspendThreads();
}