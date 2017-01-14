#pragma once

#include <RendererCore/Basics.h>
#include <Foundation/Strings/HashedString.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezGALContext;
class ezShaderStageBinary;
struct ezVertexDeclarationInfo;

typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;
typedef ezTypedResourceHandle<class ezTextureCubeResource> ezTextureCubeResourceHandle;
typedef ezTypedResourceHandle<class ezMeshBufferResource> ezMeshBufferResourceHandle;
typedef ezTypedResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;
typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;
typedef ezTypedResourceHandle<class ezShaderPermutationResource> ezShaderPermutationResourceHandle;
typedef ezTypedResourceHandle<class ezRenderPipelineResource> ezRenderPipelineResourceHandle;

struct ezPermutationVar
{
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

  ezHashedString m_sName;
  ezHashedString m_sValue;

  EZ_FORCE_INLINE bool operator==(const ezPermutationVar& other) const
  {
    return m_sName == other.m_sName && m_sValue == other.m_sValue;
  }
};
