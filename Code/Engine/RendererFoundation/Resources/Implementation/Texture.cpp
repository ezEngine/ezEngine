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
  const ezUInt16 uiSlices = (m_Description.m_Type == ezGALTextureType::TextureCube || m_Description.m_Type == ezGALTextureType::TextureCubeArray) ? (ezUInt16)m_Description.m_uiArraySize * 6 : (ezUInt16)m_Description.m_uiArraySize;
  const ezUInt8 uiMipLevels = (ezUInt8)m_Description.m_uiMipLevelCount;
  if (range.m_uiArraySlices == EZ_GAL_ALL_ARRAY_SLICES)
  {
    range.m_uiArraySlices = static_cast<ezUInt16>(uiSlices - range.m_uiBaseArraySlice);
  }
  if (range.m_uiMipLevels == EZ_GAL_ALL_MIP_LEVELS)
  {
    range.m_uiMipLevels = static_cast<ezUInt8>(uiMipLevels - range.m_uiBaseMipLevel);
  }
  EZ_ASSERT_DEBUG(range.m_uiBaseArraySlice + range.m_uiArraySlices <= uiSlices, "Invalid ezGALTextureRange: Base array slice {} + array slices {} is bigger than texture's slice count {}", range.m_uiBaseArraySlice, range.m_uiArraySlices, uiSlices);
  EZ_ASSERT_DEBUG(range.m_uiBaseMipLevel + range.m_uiMipLevels <= uiMipLevels, "Invalid ezGALTextureRange: Base mip level {} + mip levels {} is bigger than texture's mip level count {}", range.m_uiBaseMipLevel, range.m_uiMipLevels, uiMipLevels);
  return range;
}
