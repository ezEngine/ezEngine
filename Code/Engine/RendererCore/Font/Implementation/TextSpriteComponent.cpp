#include <RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Font/TextSpriteComponent.h>
#include <RendererCore/Messages/SetColorMessage.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextSpriteBlendMode, 1)
  EZ_ENUM_CONSTANTS(ezTextSpriteBlendMode::Masked, ezTextSpriteBlendMode::Transparent, ezTextSpriteBlendMode::Additive)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
ezTempHashedString ezTextSpriteBlendMode::GetPermutationValue(Enum blendMode)
{
  switch (blendMode)
  {
    case ezTextSpriteBlendMode::Masked:
      return "BLEND_MODE_MASKED";
    case ezTextSpriteBlendMode::Transparent:
      return "BLEND_MODE_TRANSPARENT";
    case ezTextSpriteBlendMode::Additive:
      return "BLEND_MODE_ADDITIVE";
  }

  return "";
}

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextSpriteRenderData, 1, ezRTTIDefaultAllocator<ezTextSpriteRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


void ezTextSpriteRenderData::FillBatchIdAndSortingKey()
{
  const ezUInt32 uiTextureIDHash = m_hTexture.GetResourceIDHash();

  // Generate batch id from mode and texture
  ezUInt32 data[] = {(ezUInt32)m_BlendMode, uiTextureIDHash};
  m_uiBatchId = ezHashingUtils::xxHash32(data, sizeof(data));

  // Sort by mode and then by texture
  m_uiSortingKey = (m_BlendMode << 30) | (uiTextureIDHash & 0x3FFFFFFF);
}
