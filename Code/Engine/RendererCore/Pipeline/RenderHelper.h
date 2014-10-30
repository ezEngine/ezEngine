#pragma once

#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>

class ezGALContext;

class EZ_RENDERERCORE_DLL ezRenderHelper
{
public:
  static void SetMaterialState(ezGALContext* pContext, const ezMaterialResourceHandle& hMaterial);

  static void DrawMeshBuffer(ezGALContext* pContext, const ezMeshBufferResourceHandle& hMeshBuffer,
    ezUInt32 uiPrimitiveCount = 0xFFFFFFFF, ezUInt32 uiFirstPrimitive = 0, ezUInt32 uiInstanceCount = 1);
};

