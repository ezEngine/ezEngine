#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/RendererCoreDLL.h>

class ezShaderStageBinary;

using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;
using ezTexture3DResourceHandle = ezTypedResourceHandle<class ezTexture3DResource>;
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
using ezDecalId = ezGenericId<16, 8>;

struct EZ_RENDERERCORE_DLL ezPermutationVar
{
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

  ezHashedString m_sName;
  ezHashedString m_sValue;

  EZ_ALWAYS_INLINE bool operator==(const ezPermutationVar& other) const { return m_sName == other.m_sName && m_sValue == other.m_sValue; }
};

struct EZ_RENDERERCORE_DLL ezMeshImportTransform
{
  using StorageType = ezInt8;

  enum Enum
  {
    Blender_YUp,
    Blender_ZUp,

    Custom = 127,

    Default = Blender_YUp
  };

  static ezBasisAxis::Enum GetRightDir(ezMeshImportTransform::Enum transform, ezBasisAxis::Enum dir);
  static ezBasisAxis::Enum GetUpDir(ezMeshImportTransform::Enum transform, ezBasisAxis::Enum dir);
  static bool GetFlipForward(ezMeshImportTransform::Enum transform, bool bFlip);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezMeshImportTransform);
