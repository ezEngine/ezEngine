#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>
#include <Core/ResourceManager/Resource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageEnums.h>
#include <RendererCore/Font/FontGlyph.h>
#include <RendererCore/RendererCoreDLL.h>

typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

/**	data about every character for a bitmap font of a specific size. */
struct ezFontBitmapBase
{
  /** Font size for which the data is contained. */
  ezUInt32 m_Size;

  /** Y offset to the baseline on which the characters are placed, in pixels. */
  ezInt32 m_BaselineOffset;

  /** Height of a single line of the font, in pixels. */
  ezUInt32 m_LineHeight;

  /** Character to use when data for a character is missing. */
  ezFontGlyph m_MissingGlyph;

  /** Width of a space in pixels. */
  ezUInt32 m_SpaceWidth;

  /** All characters in the font referenced by character ID. */
  ezMap<ezUInt32, ezFontGlyph> m_Characters;
};

/**	Contains textures and data about every character for a bitmap font of a specific size. */
struct EZ_RENDERERCORE_DLL ezFontBitmap : public ezFontBitmapBase
{
  ezFontBitmap() = default;

  /** Textures in which the character's pixels are stored. */
  ezDynamicArray<ezTexture2DResourceHandle> m_TexturePages;

  /**	Returns a character description for the character with the specified Unicode key. */
  const ezFontGlyph& GetFontGlyph(ezUInt32 charId) const;
};

/**	Contains textures and data about every character for a bitmap font of a specific size. */
struct ezRawFontBitmap : public ezFontBitmapBase
{
  ezRawFontBitmap() = default;

  /** Images in which the character's pixels are stored. */
  ezDynamicArray<ezImage> m_Textures;

  EZ_RENDERERCORE_DLL ezResult Serialize(ezStreamWriter& stream) const;
  EZ_RENDERERCORE_DLL ezResult Deserialize(ezStreamReader& stream);
};
