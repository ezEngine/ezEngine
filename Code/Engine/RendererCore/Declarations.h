#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/RendererCoreDLL.h>

class ezShaderStageBinary;
struct ezVertexDeclarationInfo;

using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;
using ezRenderToTexture2DResourceHandle = ezTypedResourceHandle<class ezRenderToTexture2DResource>;
using ezTextureCubeResourceHandle = ezTypedResourceHandle<class ezTextureCubeResource>;
using ezMeshBufferResourceHandle = ezTypedResourceHandle<class ezMeshBufferResource>;
using ezDynamicMeshBufferResourceHandle = ezTypedResourceHandle<class ezDynamicMeshBufferResource>;
using ezMeshResourceHandle = ezTypedResourceHandle<class ezMeshResource>;
using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;
using ezShaderResourceHandle = ezTypedResourceHandle<class ezShaderResource>;
using ezShaderPermutationResourceHandle = ezTypedResourceHandle<class ezShaderPermutationResource>;
using ezRenderPipelineResourceHandle = ezTypedResourceHandle<class ezRenderPipelineResource>;
using ezDecalResourceHandle = ezTypedResourceHandle<class ezDecalResource>;
using ezDecalAtlasResourceHandle = ezTypedResourceHandle<class ezDecalAtlasResource>;

struct EZ_RENDERERCORE_DLL ezPermutationVar
{
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

  ezHashedString m_sName;
  ezHashedString m_sValue;

  EZ_ALWAYS_INLINE bool operator==(const ezPermutationVar& other) const { return m_sName == other.m_sName && m_sValue == other.m_sValue; }
};
