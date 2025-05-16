#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/RendererCoreDLL.h>
#include <Texture/Utils/TextureAtlasDesc.h>

using ezDecalAtlasResourceHandle = ezTypedResourceHandle<class ezDecalAtlasResource>;
using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;

class ezImage;

struct ezDecalAtlasResourceDescriptor
{
};

class EZ_RENDERERCORE_DLL ezDecalAtlasResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalAtlasResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezDecalAtlasResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezDecalAtlasResource, ezDecalAtlasResourceDescriptor);

public:
  ezDecalAtlasResource();

  const ezTexture2DResourceHandle& GetBaseColorTexture() const { return m_hBaseColor; }
  const ezTexture2DResourceHandle& GetNormalTexture() const { return m_hNormal; }
  const ezTexture2DResourceHandle& GetORMTexture() const { return m_hORM; }
  const ezVec2U32& GetBaseColorTextureSize() const { return m_vBaseColorSize; }
  const ezVec2U32& GetNormalTextureSize() const { return m_vNormalSize; }
  const ezVec2U32& GetORMTextureSize() const { return m_vORMSize; }
  const ezTextureAtlasRuntimeDesc& GetAtlas() const { return m_Atlas; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void ReportResourceIsMissing() override;

  void ReadDecalInfo(ezStreamReader* Stream);

  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  void CreateLayerTexture(const ezImage& img, bool bSRGB, ezTexture2DResourceHandle& out_hTexture);

  ezTextureAtlasRuntimeDesc m_Atlas;
  static ezUInt32 s_uiDecalAtlasResources;
  ezTexture2DResourceHandle m_hBaseColor;
  ezTexture2DResourceHandle m_hNormal;
  ezTexture2DResourceHandle m_hORM;
  ezVec2U32 m_vBaseColorSize;
  ezVec2U32 m_vNormalSize;
  ezVec2U32 m_vORMSize;
};
