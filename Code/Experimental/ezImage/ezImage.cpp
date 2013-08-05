#include <Foundation/Basics.h>
#include "ezImage.h"

void ezImage::AllocateImageData()
{
  m_subImages.SetCount(m_uiNumMipLevels * m_uiNumFaces * m_uiNumArrayIndices);

  int uiDataSize = 0;

  for(ezUInt32 uiArrayIndex = 0; uiArrayIndex < m_uiNumArrayIndices; uiArrayIndex++)
  {
    for(ezUInt32 uiFace = 0; uiFace < m_uiNumFaces; uiFace++)
    {
      for(ezUInt32 uiMipLevel = 0; uiMipLevel < m_uiNumMipLevels; uiMipLevel++)
      {
        SubImage& subImage = GetSubImage(uiMipLevel, uiFace, uiArrayIndex);

        subImage.m_uiDataOffset = uiDataSize;
        subImage.m_uiRowPitch = CalculateRowPitch(GetWidth(uiMipLevel));
        subImage.m_uiDepthPitch = CalculateDepthPitch(GetWidth(uiMipLevel), GetHeight(uiMipLevel));

        uiDataSize += CalculateSubImagePitch(GetWidth(uiMipLevel), GetHeight(uiMipLevel), GetDepth(uiMipLevel));
      }
    }
  }

  m_data.SetCount(uiDataSize);
}
