#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Containers/ArrayMap.h>
#include <RendererCore/Font/FontGlyph.h>
#include <RendererCore/Font/FontBitmap.h>
#include <RendererCore/RendererCoreDLL.h>

typedef ezTypedResourceHandle<class ezFontResource> ezFontResourceHandle;

struct EZ_RENDERERCORE_DLL ezFontResourceDescriptor
{
};

struct EZ_RENDERERCORE_DLL ezRawFont
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezRawFont);

  ezRawFont() = default;

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);

  ezString m_Name;
  ezString m_FamilyName;
  ezMap<ezUInt32, ezRawFontBitmap> m_BitmapPerSize;
};

/**
	 * Font resource containing data about textual characters and how to render text. Contains one or multiple font
	 * bitmaps, each for a specific size.
	 */
class EZ_RENDERERCORE_DLL ezFontResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFontResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezFontResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezFontResource, ezFontResourceDescriptor);

public:
  ezFontResource();
  ~ezFontResource() = default;

  /**
		 * Returns font bitmap for a specific font size.
		 *
		 * @param[in]	size	Size of the bitmap in points.
		 * @return				Bitmap object if it exists, false otherwise.
		 */
  const ezFontBitmap& GetBitmap(ezUInt32 size) const;

  /**	
		 * Finds the available font bitmap size closest to the provided size.
		 *
		 * @param[in]	size	Size of the bitmap in points.
		 * @return				Nearest available bitmap size.
		 */
  ezInt32 GetClosestSize(ezUInt32 size) const;

  const ezString& GetName() const
  {
    return m_Name;
  }

  const ezString& GetFamilyName() const
  {
    return m_FamilyName;
  }


private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezString m_Name;
  ezString m_FamilyName;
  ezArrayMap<ezUInt32, ezFontBitmap> m_FontDataPerSize;
  ezUInt32 m_GPUMemory = 0;
};

class EZ_RENDERERCORE_DLL ezFontResourceLoader : public ezResourceLoaderFromFile
{
public:
  virtual bool IsResourceOutdated(const ezResource* pResource) const override;
};
