#pragma once

#include <RendererCore/Basics.h>
#include <Foundation/Strings/HashedString.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezGALContext;
class ezShaderStageBinary;
struct ezVertexDeclarationInfo;

typedef ezTypedResourceHandle<class ezTextureResource> ezTextureResourceHandle;
typedef ezTypedResourceHandle<class ezConstantBufferResource> ezConstantBufferResourceHandle;
typedef ezTypedResourceHandle<class ezMeshBufferResource> ezMeshBufferResourceHandle;
typedef ezTypedResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;
typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;
typedef ezTypedResourceHandle<class ezShaderPermutationResource> ezShaderPermutationResourceHandle;

struct ezPermutationVar
{
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

  ezHashedString m_sName;
  ezHashedString m_sValue;
};
