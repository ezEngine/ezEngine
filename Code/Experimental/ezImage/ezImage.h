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
  T* GetDataPointer();

  template<typename T>
  const T* GetDataPointer() const;

  template<typename T>
  const T* GetSubImagePointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const;

  template<typename T>
  T* GetSubImagePointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex);

  template<typename T>
  const T* GetPixelPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 x, ezUInt32 y, ezUInt32 z) const;

  template<typename T>
  T* GetPixelPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 x, ezUInt32 y, ezUInt32 z);

  template<typename T>
  const T* GetBlockPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 uiBlockX, ezUInt32 uiBlockY, ezUInt32 z) const;

  template<typename T>
  T* GetBlockPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 uiBlockX, ezUInt32 uiBlockY, ezUInt32 z);

  void AllocateImageData();

  void SetRowPitch(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 uiRowPitch)
  {
    EZ_ASSERT(ezImageFormat::GetType(m_format) == ezImageFormatType::LINEAR, "Row pitch can only be set for linear formats.");
    GetSubImage(uiMipLevel, uiFace, uiArrayIndex).m_uiRowPitch = uiRowPitch;
  }

  ezUInt32 GetRowPitch(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const
  {
    EZ_ASSERT(ezImageFormat::GetType(m_format) == ezImageFormatType::LINEAR, "Row pitch can only be retrieved for linear formats.");
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

  ezUInt32 GetNumBlocksX(ezUInt32 uiMipLevel = 0) const
  {
    EZ_ASSERT(ezImageFormat::GetType(m_format) == ezImageFormatType::BLOCK_COMPRESSED,
      "Number of blocks can only be retrieved for block compressed formats.");
    ezUInt32 uiBlockSize = 4;
    return (GetWidth(uiMipLevel) + uiBlockSize - 1) / uiBlockSize;
  }

  ezUInt32 GetNumBlocksY(ezUInt32 uiMipLevel = 0) const
  {
    EZ_ASSERT(ezImageFormat::GetType(m_format) == ezImageFormatType::BLOCK_COMPRESSED,
      "Number of blocks can only be retrieved for block compressed formats.");
    ezUInt32 uiBlockSize = 4;
    return (GetHeight(uiMipLevel) + uiBlockSize - 1) / uiBlockSize;
  }

private:
  struct SubImage
  {
    EZ_DECLARE_POD_TYPE();

    int m_uiRowPitch;
    int m_uiDepthPitch;
    int m_uiDataOffset;
  };

  inline void ValidateSubImageIndices(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const;

  inline SubImage& GetSubImage(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex);

  inline const SubImage& GetSubImage(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const;

  ezUInt32 m_uiRowAlignment;
  ezUInt32 m_uiDepthAlignment;
  ezUInt32 m_uiSubImageAlignment;

  ezDynamicArray<SubImage> m_subImages;
  ezDynamicArray<ezUInt8> m_data;
};

#include <ezImage_inl.h>