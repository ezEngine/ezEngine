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

  ezFontGlyph()
    : m_CharacterId(0)
    , m_Page(0)
    , m_UVX(0)
    , m_UVY(0)
    , m_UVWidth(0)
    , m_UVHeight(0)
    , m_Width(0)
    , m_Height(0)
    , m_XOffset(0)
    , m_YOffset(0)
    , m_XAdvance(0)
    , m_YAdvance(0)
  {

  }

  ezUInt32 m_CharacterId;         /**< Character ID, corresponding to a Unicode key. */
  ezUInt32 m_Page;                /**< Index of the texture the character is located on. */
  float m_UVX, m_UVY;             /**< Texture coordinates of the character in the page texture. */
  float m_UVWidth, m_UVHeight;    /**< Width/height of the character in texture coordinates. */
  ezUInt32 m_Width, m_Height;     /**< Width/height of the character in pixels. */
  ezInt32 m_XOffset, m_YOffset;   /**< Offset for the visible portion of the character in pixels. */
  ezInt32 m_XAdvance, m_YAdvance; /**< Determines how much to advance the pen after writing this character, in pixels. */

  /**
		 * Pairs that determine if certain character pairs should be closer or father together. for example "AV"
		 * combination.
		 */
  ezDynamicArray<ezKerningPair> m_KerningPairs;

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);
};
