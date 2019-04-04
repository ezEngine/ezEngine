#pragma once

template<typename T> struct ezImageSizeofHelper
{
  static constexpr size_t Size = sizeof (T);
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
  ValidateDataTypeAccessor<T>();
  return ezBlobPtr<const T>(reinterpret_cast<T*>(static_cast<ezUInt8*>(m_dataPtr.GetPtr())), m_dataPtr.GetCount() / ezImageSizeofHelper<T>::Size);
}

template <typename T>
ezBlobPtr<T> ezImage::GetBlobPtr()
{
  ezBlobPtr<const T> constPtr = ezImageView::GetBlobPtr<T>();
  
  return ezBlobPtr<T>(const_cast<T*>(static_cast<const T*>(constPtr.GetPtr())), constPtr.GetCount());
}

template <typename T>
const T* ezImageView::GetPixelPointer(ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/, ezUInt32 x /*= 0*/,
                                ezUInt32 y /*= 0*/, ezUInt32 z /*= 0*/) const
{
  ValidateDataTypeAccessor<T>();
  EZ_ASSERT_DEV(x < GetNumBlocksX(uiMipLevel), "Invalid x coordinate");
  EZ_ASSERT_DEV(y < GetNumBlocksY(uiMipLevel), "Invalid y coordinate");
  EZ_ASSERT_DEV(z < GetNumBlocksZ(uiMipLevel), "Invalid z coordinate");

  ezUInt32 offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex) + z * GetDepthPitch(uiMipLevel) + y * GetRowPitch(uiMipLevel) + x * ezImageFormat::GetBitsPerBlock(m_format) / 8;
  return reinterpret_cast<const T*>(&m_dataPtr[offset]);
}

template <typename T>
 T* ezImage::GetPixelPointer(ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/,
                                      ezUInt32 x /*= 0*/, ezUInt32 y /*= 0*/, ezUInt32 z /*= 0*/)
{
  return const_cast<T*>(ezImageView::GetPixelPointer<T>(uiMipLevel, uiFace, uiArrayIndex, x, y, z));
}


template <typename T>
void ezImageView::ValidateDataTypeAccessor() const
{
  ezUInt32 bytesPerBlock = ezImageFormat::GetBitsPerBlock(GetImageFormat()) / 8;
  EZ_ASSERT_DEV(bytesPerBlock % ezImageSizeofHelper<T>::Size == 0, "Accessor type is not suitable for interpreting contained data");
}
