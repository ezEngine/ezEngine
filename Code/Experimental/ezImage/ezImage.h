#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Basics/Assert.h>

#include "ezImageHeader.h"
#include "ezImageDefinitions.h"
#include "Foundation/Containers/DynamicArray.h"
#include "Foundation/Math/Implementation/Math_inl.h"

class ezImage : public ezImageHeader
{
public:
  ezImage() : m_uiRowAlignment(1), m_uiDepthAlignment(1), m_uiSubImageAlignment(1)
  {
  }

  void SetDataSize(ezUInt32 uiSize)
  {
    m_data.SetCount(uiSize + 16);
  }

  ezUInt32 GetDataSize() const
  {
    return ezMath::Max(static_cast<int>(m_data.GetCount()) - 16, 0);
  }

  template<typename T>
  T* GetDataPointer()
  {
    return reinterpret_cast<T*>(&m_data[0]);
  }

  template<typename T>
  const T* GetDataPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const
  {
    return reinterpret_cast<const T*>(&m_data[GetSubImage(uiMipLevel, uiFace, uiArrayIndex).m_uiDataOffset]);
  }

  template<typename T>
  T* GetDataPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex)
  {
    return reinterpret_cast<T*>(&m_data[GetSubImage(uiMipLevel, uiFace, uiArrayIndex).m_uiDataOffset]);
  }

  template<typename T>
  const T* GetDataPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 x, ezUInt32 y, ezUInt32 z) const
  {
    const ezUInt8* pPointer = GetDataPointer<ezUInt8>(uiMipLevel, uiFace, uiArrayIndex);

    pPointer += z * GetDepthPitch(uiMipLevel, uiFace, uiArrayIndex);
    pPointer += y * GetRowPitch(uiMipLevel, uiFace, uiArrayIndex);
    pPointer += x * ezImageFormat::GetBitsPerPixel(m_format) / 8;

    return reinterpret_cast<const T*>(pPointer);
  }

  template<typename T>
  T* GetDataPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 x, ezUInt32 y, ezUInt32 z)
  {
    ezUInt8* basePointer = GetDataPointer<ezUInt8>(uiMipLevel, uiFace, uiArrayIndex);

    basePointer += z * GetDepthPitch(uiMipLevel, uiFace, uiArrayIndex);
    basePointer += y * GetRowPitch(uiMipLevel, uiFace, uiArrayIndex);
    basePointer += x * ezImageFormat::GetBitsPerPixel(m_format) / 8;

    return reinterpret_cast<T*>(basePointer);
  }

  void AllocateImageData();

  void SetRowPitch(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 uiRowPitch)
  {
    GetSubImage(uiMipLevel, uiFace, uiArrayIndex).m_uiRowPitch = uiRowPitch;
  }

  ezUInt32 GetRowPitch(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const
  {
    return GetSubImage(uiMipLevel, uiFace, uiArrayIndex).m_uiRowPitch;
  }

  void SetDepthPitch(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 uiDepthPitch)
  {
    GetSubImage(uiMipLevel, uiFace, uiArrayIndex).m_uiDepthPitch = uiDepthPitch;
  }

  ezUInt32 GetDepthPitch(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const
  {
    return GetSubImage(uiMipLevel, uiFace, uiArrayIndex).m_uiDepthPitch;
  }

  void SetDataOffSet(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 uiDataOffset)
  {
    GetSubImage(uiMipLevel, uiFace, uiArrayIndex).m_uiDataOffset = uiDataOffset;
  }

  ezUInt32 GetDataOffSet(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const
  {
    return GetSubImage(uiMipLevel, uiFace, uiArrayIndex).m_uiDataOffset;
  }

  void SetRowAlignment(ezUInt32 rowAlignment)
  {
    m_uiRowAlignment = rowAlignment;
  }

  ezUInt32 GetRowAlignment() const
  {
    return m_uiRowAlignment;
  }

  void SetDepthAlignment(ezUInt32 depthAlignment)
  {
    m_uiDepthAlignment = depthAlignment;
  }

  ezUInt32 GetDepthAlignment() const
  {
    return m_uiDepthAlignment;
  }

  void SetSubImageAlignment(ezUInt32 subImageAlignment)
  {
    m_uiSubImageAlignment = subImageAlignment;
  }

  ezUInt32 GetSubImageAlignment() const
  {
    return m_uiSubImageAlignment;
  }


private:
  struct SubImage
  {
    EZ_DECLARE_POD_TYPE();

    int m_uiRowPitch;
    int m_uiDepthPitch;
    int m_uiDataOffset;
  };

  void ValidateSubImageIndices(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const
  {
    EZ_ASSERT(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    EZ_ASSERT(uiFace < m_uiNumFaces,  "Invalid uiFace");
    EZ_ASSERT(uiArrayIndex < m_uiNumArrayIndices, "Invalid array slice");
  }

  SubImage& GetSubImage(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex)
  {
    ValidateSubImageIndices(uiMipLevel, uiFace, uiArrayIndex);
    return m_subImages[uiMipLevel + m_uiNumMipLevels * (uiFace + m_uiNumFaces * uiArrayIndex)];
  }

  const SubImage& GetSubImage(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const
  {
    ValidateSubImageIndices(uiMipLevel, uiFace, uiArrayIndex);
    return m_subImages[uiMipLevel + m_uiNumMipLevels * (uiFace + m_uiNumFaces * uiArrayIndex)];
  }

  ezUInt32 CalculateRowPitch(ezUInt32 uiWidth) const
  {
	  return ((uiWidth * ezImageFormat::GetBitsPerPixel(m_format) / 8 - 1) / m_uiRowAlignment + 1) * m_uiRowAlignment;
  }

  ezUInt32 CalculateDepthPitch(ezUInt32 uiWidth, ezUInt32 uiHeight) const
  {
	  return ((uiHeight * CalculateRowPitch(uiWidth) - 1) / m_uiDepthAlignment + 1) * m_uiDepthAlignment;
  }

  ezUInt32 CalculateSubImagePitch(ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiDepth) const
  {
	  return ((uiDepth * CalculateDepthPitch(uiWidth, uiHeight) - 1) / m_uiSubImageAlignment + 1) * m_uiSubImageAlignment;
  }

  ezUInt32 m_uiRowAlignment;
  ezUInt32 m_uiDepthAlignment;
  ezUInt32 m_uiSubImageAlignment;

  ezDynamicArray<SubImage> m_subImages;
  ezDynamicArray<ezUInt8> m_data;
};