#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Texture.h>

ezGALTexture::ezGALTexture(const ezGALTextureCreationDescription& Description)
  : ezGALResource(Description)
{
}

ezGALTexture::~ezGALTexture()
{
  EZ_ASSERT_DEV(m_hDefaultRenderTargetView.IsInvalidated(), "");
  EZ_ASSERT_DEV(m_RenderTargetViews.IsEmpty(), "Dangling render target views");
}

ezVec3U32 ezGALTexture::GetMipMapSize(ezUInt32 uiMipLevel) const
{
  ezVec3U32 size = {m_Description.m_uiWidth, m_Description.m_uiHeight, m_Description.m_uiDepth};
  size.x = ezMath::Max(1u, size.x >> uiMipLevel);
  size.y = ezMath::Max(1u, size.y >> uiMipLevel);
  size.z = ezMath::Max(1u, size.z >> uiMipLevel);
  return size;
}

ezGALTextureRange ezGALTexture::ClampRange(ezGALTextureRange range) const
{
  const ezUInt32 uiSlices = (m_Description.m_Type == ezGALTextureType::TextureCube || m_Description.m_Type == ezGALTextureType::TextureCubeArray) ? m_Description.m_uiArraySize * 6 : m_Description.m_uiArraySize;
  if (range.m_uiArraySlices == EZ_GAL_ALL_ARRAY_SLICES)
  {
    range.m_uiArraySlices = static_cast<ezUInt8>(uiSlices - range.m_uiBaseArraySlice);
  }
  if (range.m_uiMipLevels == EZ_GAL_ALL_MIP_LEVELS)
  {
    range.m_uiMipLevels = static_cast<ezUInt8>(m_Description.m_uiMipLevelCount - range.m_uiBaseMipLevel);
  }
  return range;
}
