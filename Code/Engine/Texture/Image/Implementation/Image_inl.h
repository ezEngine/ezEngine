#pragma once

template <typename T>
struct ezImageSizeofHelper
{
  static constexpr size_t Size = sizeof(T);
};

template <>
struct ezImageSizeofHelper<void>
{
  static constexpr size_t Size = 1;
};

template <>
struct ezImageSizeofHelper<const void>
{
  static constexpr size_t Size = 1;
};

template <typename T>
ezBlobPtr<const T> ezImageView::GetBlobPtr() const
{
  for (ezUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); ++uiPlaneIndex)
  {
    ValidateDataTypeAccessor<T>(uiPlaneIndex);
  }
  return ezBlobPtr<const T>(reinterpret_cast<T*>(static_cast<ezUInt8*>(m_DataPtr.GetPtr())), m_DataPtr.GetCount() / ezImageSizeofHelper<T>::Size);
}

inline ezConstByteBlobPtr ezImageView::GetByteBlobPtr() const
{
  for (ezUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); ++uiPlaneIndex)
  {
    ValidateDataTypeAccessor<ezUInt8>(uiPlaneIndex);
  }
  return ezConstByteBlobPtr(static_cast<ezUInt8*>(m_DataPtr.GetPtr()), m_DataPtr.GetCount());
}

template <typename T>
ezBlobPtr<T> ezImage::GetBlobPtr()
{
  ezBlobPtr<const T> constPtr = ezImageView::GetBlobPtr<T>();

  return ezBlobPtr<T>(const_cast<T*>(static_cast<const T*>(constPtr.GetPtr())), constPtr.GetCount());
}

inline ezByteBlobPtr ezImage::GetByteBlobPtr()
{
  ezConstByteBlobPtr constPtr = ezImageView::GetByteBlobPtr();

  return ezByteBlobPtr(const_cast<ezUInt8*>(constPtr.GetPtr()), constPtr.GetCount());
}

template <typename T>
const T* ezImageView::GetPixelPointer(ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/, ezUInt32 x /*= 0*/,
  ezUInt32 y /*= 0*/, ezUInt32 z /*= 0*/, ezUInt32 uiPlaneIndex /*= 0*/) const
{
  ValidateDataTypeAccessor<T>(uiPlaneIndex);
  EZ_ASSERT_DEV(x < GetNumBlocksX(uiMipLevel, uiPlaneIndex), "Invalid x coordinate");
  EZ_ASSERT_DEV(y < GetNumBlocksY(uiMipLevel, uiPlaneIndex), "Invalid y coordinate");
  EZ_ASSERT_DEV(z < GetNumBlocksZ(uiMipLevel, uiPlaneIndex), "Invalid z coordinate");

  ezUInt64 offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex) +
                    z * GetDepthPitch(uiMipLevel, uiPlaneIndex) +
                    y * GetRowPitch(uiMipLevel, uiPlaneIndex) +
                    x * ezImageFormat::GetBitsPerBlock(m_Format, uiPlaneIndex) / 8;
  return reinterpret_cast<const T*>(&m_DataPtr[offset]);
}

template <typename T>
T* ezImage::GetPixelPointer(
  ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/, ezUInt32 x /*= 0*/, ezUInt32 y /*= 0*/, ezUInt32 z /*= 0*/, ezUInt32 uiPlaneIndex /*= 0*/)
{
  return const_cast<T*>(ezImageView::GetPixelPointer<T>(uiMipLevel, uiFace, uiArrayIndex, x, y, z, uiPlaneIndex));
}


template <typename T>
void ezImageView::ValidateDataTypeAccessor(ezUInt32 uiPlaneIndex) const
{
  ezUInt32 bytesPerBlock = ezImageFormat::GetBitsPerBlock(GetImageFormat(), uiPlaneIndex) / 8;
  EZ_IGNORE_UNUSED(bytesPerBlock);
  EZ_ASSERT_DEV(bytesPerBlock % ezImageSizeofHelper<T>::Size == 0, "Accessor type is not suitable for interpreting contained data");
}
