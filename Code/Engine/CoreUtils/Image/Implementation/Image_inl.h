#pragma once

ezImage::ezImage()
{

}

ezUInt32 ezImage::GetNumBlocksX(ezUInt32 uiMipLevel) const
{
  EZ_ASSERT_DEV(ezImageFormat::GetType(m_format) == ezImageFormatType::BLOCK_COMPRESSED,
    "Number of blocks can only be retrieved for block compressed formats.");
  ezUInt32 uiBlockSize = 4;
  return (GetWidth(uiMipLevel) + uiBlockSize - 1) / uiBlockSize;
}

ezUInt32 ezImage::GetNumBlocksY(ezUInt32 uiMipLevel) const
{
  EZ_ASSERT_DEV(ezImageFormat::GetType(m_format) == ezImageFormatType::BLOCK_COMPRESSED,
    "Number of blocks can only be retrieved for block compressed formats.");
  ezUInt32 uiBlockSize = 4;
  return (GetHeight(uiMipLevel) + uiBlockSize - 1) / uiBlockSize;
}

ezUInt32 ezImage::GetDataSize() const
{
  return ezMath::Max(static_cast<int>(m_data.GetCount()) - 16, 0);
}


template<typename T>
const T* ezImage::GetDataPointer() const
{
  return reinterpret_cast<const T*>(&m_data[0]);
}

template<typename T>
T* ezImage::GetDataPointer()
{
  return const_cast<T*>(static_cast<const ezImage*>(this)->GetDataPointer<T>());
}

template<typename T>
const T* ezImage::GetSubImagePointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const
{
  return reinterpret_cast<const T*>(&m_data[GetSubImage(uiMipLevel, uiFace, uiArrayIndex).m_uiDataOffset]);
}

template<typename T>
T* ezImage::GetSubImagePointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex)
{
  return const_cast<T*>(static_cast<const ezImage*>(this)->GetSubImagePointer<T>(uiMipLevel, uiFace, uiArrayIndex));
}

template<typename T>
const T* ezImage::GetPixelPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 x, ezUInt32 y, ezUInt32 z) const
{
  EZ_ASSERT_DEBUG(ezImageFormat::GetType(m_format) == ezImageFormatType::LINEAR, "Pixel pointer can only be retrieved for linear formats.");
  EZ_ASSERT_DEV(x < this->m_uiWidth, "x out of bounds");
  EZ_ASSERT_DEV(y < this->m_uiHeight, "y out of bounds");
  EZ_ASSERT_DEV(z < this->m_uiDepth, "z out of bounds");

  const ezUInt8* pPointer = GetSubImagePointer<ezUInt8>(uiMipLevel, uiFace, uiArrayIndex);

  pPointer += z * GetDepthPitch(uiMipLevel);
  pPointer += y * GetRowPitch(uiMipLevel);
  pPointer += x * ezImageFormat::GetBitsPerPixel(m_format) / 8;

  return reinterpret_cast<const T*>(pPointer);
}

template<typename T>
T* ezImage::GetPixelPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 x, ezUInt32 y, ezUInt32 z)
{
  return const_cast<T*>(static_cast<const ezImage*>(this)->GetPixelPointer<T>(uiMipLevel, uiFace, uiArrayIndex, x, y, z));
}


template<typename T>
const T* ezImage::GetBlockPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 uiBlockX, ezUInt32 uiBlockY, ezUInt32 z) const
{
  EZ_ASSERT_DEBUG(ezImageFormat::GetType(m_format) == ezImageFormatType::BLOCK_COMPRESSED,
    "Block pointer can only be retrieved for block compressed formats.");

  const ezUInt8* basePointer = GetSubImagePointer<ezUInt8>(uiMipLevel, uiFace, uiArrayIndex);

  basePointer += z * GetDepthPitch(uiMipLevel);

  const ezUInt32 uiBlockSize = 4;

  const ezUInt32 uiNumBlocksX = GetWidth(uiMipLevel) / uiBlockSize;

  const ezUInt32 uiBlockIndex = uiBlockX + uiNumBlocksX * uiBlockY;

  basePointer += uiBlockIndex * uiBlockSize * uiBlockSize * ezImageFormat::GetBitsPerPixel(m_format) / 8;

  return reinterpret_cast<const T*>(basePointer);
}

template<typename T>
T* ezImage::GetBlockPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 uiBlockX, ezUInt32 uiBlockY, ezUInt32 z)
{
  return const_cast<T*>(static_cast<const ezImage*>(this)->GetBlockPointer<T>(uiMipLevel, uiFace, uiArrayIndex, uiBlockX, uiBlockY, z));
}


const ezImage::SubImage& ezImage::GetSubImage(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const
{
  ValidateSubImageIndices(uiMipLevel, uiFace, uiArrayIndex);
  return m_subImages[uiMipLevel + m_uiNumMipLevels * (uiFace + m_uiNumFaces * uiArrayIndex)];
}

ezImage::SubImage& ezImage::GetSubImage(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex)
{
  ValidateSubImageIndices(uiMipLevel, uiFace, uiArrayIndex);
  return m_subImages[uiMipLevel + m_uiNumMipLevels * (uiFace + m_uiNumFaces * uiArrayIndex)];
}

void ezImage::ValidateSubImageIndices(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const
{
  EZ_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
  EZ_ASSERT_DEV(uiFace < m_uiNumFaces, "Invalid uiFace");
  EZ_ASSERT_DEV(uiArrayIndex < m_uiNumArrayIndices, "Invalid array slice");
}

ezUInt32 ezImage::GetRowPitch(ezUInt32 uiMipLevel) const
{
  EZ_ASSERT_DEBUG(ezImageFormat::GetType(m_format) == ezImageFormatType::LINEAR, "Row pitch can only be retrieved for linear formats.");
  return GetSubImage(uiMipLevel, 0, 0).m_uiRowPitch;
}

ezUInt32 ezImage::GetDepthPitch(ezUInt32 uiMipLevel) const
{
  return GetSubImage(uiMipLevel, 0, 0).m_uiDepthPitch;
}

ezUInt32 ezImage::GetDataOffSet(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const
{
  return GetSubImage(uiMipLevel, uiFace, uiArrayIndex).m_uiDataOffset;
}

