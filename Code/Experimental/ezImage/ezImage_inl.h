template<typename T>
T* ezImage::GetDataPointer()
{
  return reinterpret_cast<T*>(&m_data[0]);
}

template<typename T>
const T* ezImage::GetSubImagePointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const
{
  return reinterpret_cast<const T*>(&m_data[GetSubImage(uiMipLevel, uiFace, uiArrayIndex).m_uiDataOffset]);
}

template<typename T>
T* ezImage::GetSubImagePointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex)
{
  return const_cast<T*>(static_cast<const ezImage*>(this)->GetSubImagePointer(uiMipLevel, uiFace, uiArrayIndex));
}

template<typename T>
const T* ezImage::GetPixelPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 x, ezUInt32 y, ezUInt32 z) const
{
  EZ_ASSERT(ezImageFormat::GetType(m_format) == ezImageFormatType::LINEAR, "Pixel pointer can only be retrieved for linear formats.");

  const ezUInt8* pPointer = GetSubImagePointer<ezUInt8>(uiMipLevel, uiFace, uiArrayIndex);

  pPointer += z * GetDepthPitch(uiMipLevel, uiFace, uiArrayIndex);
  pPointer += y * GetRowPitch(uiMipLevel, uiFace, uiArrayIndex);
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
  EZ_ASSERT(ezImageFormat::GetType(m_format) == ezImageFormatType::BLOCK_COMPRESSED,
    "Block pointer can only be retrieved for block compressed formats.");

  const ezUInt8* basePointer = GetSubImagePointer<ezUInt8>(uiMipLevel, uiFace, uiArrayIndex);

  basePointer += z * GetDepthPitch(uiMipLevel, uiFace, uiArrayIndex);

  const ezUInt32 uiBlockSize = 4;

  const ezUInt32 uiNumBlocksX = GetWidth(uiMipLevel) / uiBlockSize;

  const ezUInt32 uiBlockIndex = uiBlockX + uiNumBlocksX * uiBlockY;

  basePointer += uiBlockIndex * uiBlockSize * uiBlockSize * ezImageFormat::GetBitsPerPixel(m_format) / 8;

  return reinterpret_cast<const T*>(basePointer);
}

template<typename T>
T* ezImage::GetBlockPointer(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 uiBlockX, ezUInt32 uiBlockY, ezUInt32 z)
{
  return const_cast<T*>(static_cast<const ezImage*>(this)->GetBlockPointer(uiMipLevel, uiFace, uiArrayIndex, uiBlockX, uiBlockY, z));
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
  EZ_ASSERT(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
  EZ_ASSERT(uiFace < m_uiNumFaces, "Invalid uiFace");
  EZ_ASSERT(uiArrayIndex < m_uiNumArrayIndices, "Invalid array slice");
}
