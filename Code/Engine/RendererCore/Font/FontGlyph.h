#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Types/Types.h>
#include <RendererCore/RendererCoreDLL.h>

struct EZ_RENDERERCORE_DLL ezKerningPair
{
  EZ_DECLARE_POD_TYPE();
  ezKerningPair() = default;

  ezUInt32 m_OtherCharacterId;
  ezInt32 m_Amount;

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);
};

/**	Describes a single character in a font of a specific size. */
struct EZ_RENDERERCORE_DLL ezFontGlyph
{
  EZ_DECLARE_POD_TYPE();

  ezUInt32 m_CharacterId = 0;             /**< Character ID, corresponding to a Unicode key. */
  ezUInt32 m_Page = 0;                    /**< Index of the texture the character is located on. */
  float m_UVX = 0, m_UVY = 0;             /**< Texture coordinates of the character in the page texture. */
  float m_UVWidth = 0, m_UVHeight = 0;    /**< Width/height of the character in texture coordinates. */
  ezUInt32 m_Width = 0, m_Height = 0;     /**< Width/height of the character in pixels. */
  ezInt32 m_XOffset = 0, m_YOffset = 0;   /**< Offset for the visible portion of the character in pixels. */
  ezInt32 m_XAdvance = 0, m_YAdvance = 0; /**< Determines how much to advance the pen after writing this character, in pixels. */

  /**
		 * Pairs that determine if certain character pairs should be closer or father together. for example "AV"
		 * combination.
		 */
  ezDynamicArray<ezKerningPair> m_KerningPairs;

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);
};
