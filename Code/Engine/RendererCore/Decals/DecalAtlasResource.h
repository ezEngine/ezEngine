#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Basics.h>

typedef ezTypedResourceHandle<class ezDecalAtlasResource> ezDecalAtlasResourceHandle;
typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

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

  struct DecalInfo
  {
    ezString m_sIdentifier;
    ezRectU32 m_BaseColorRect;
    ezRectU32 m_NormalRect;
  };

  /// \brief Returns the one global decal atlas resource
  static ezDecalAtlasResourceHandle GetDecalAtlasResource();

  const ezTexture2DResourceHandle& GetBaseColorTexture() const { return m_hBaseColor; }
  const ezTexture2DResourceHandle& GetNormalTexture() const { return m_hNormal; }
  const ezVec2U32& GetBaseColorTextureSize() const { return m_BaseColorSize; }
  const ezVec2U32& GetNormalTextureSize() const { return m_NormalSize; }
  const ezMap<ezUInt32, DecalInfo>& GetAllDecals() const { return m_Decals; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void ReportResourceIsMissing() override;

  void ReadDecalInfo(ezStreamReader* Stream);

  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  void CreateLayerTexture(const ezImage& img, bool bSRGB, ezTexture2DResourceHandle& out_hTexture);

  ezMap<ezUInt32, DecalInfo> m_Decals;
  static ezUInt32 s_uiDecalAtlasResources;
  ezTexture2DResourceHandle m_hBaseColor;
  ezTexture2DResourceHandle m_hNormal;
  ezVec2U32 m_BaseColorSize;
  ezVec2U32 m_NormalSize;
};

