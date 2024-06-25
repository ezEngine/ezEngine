#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

ezImageView::ezImageView()
{
  Clear();
}

ezImageView::ezImageView(const ezImageHeader& header, ezConstByteBlobPtr imageData)
{
  ResetAndViewExternalStorage(header, imageData);
}

void ezImageView::Clear()
{
  ezImageHeader::Clear();
  m_SubImageOffsets.Clear();
  m_DataPtr.Clear();
}

bool ezImageView::IsValid() const
{
  return !m_DataPtr.IsEmpty();
}

void ezImageView::ResetAndViewExternalStorage(const ezImageHeader& header, ezConstByteBlobPtr imageData)
{
  static_cast<ezImageHeader&>(*this) = header;

  ezUInt64 dataSize = ComputeLayout();

  EZ_IGNORE_UNUSED(dataSize);
  EZ_ASSERT_DEV(imageData.GetCount() == dataSize, "Provided image storage ({} bytes) doesn't match required data size ({} bytes)",
    imageData.GetCount(), dataSize);

  // Const cast is safe here as we will only perform non-const access if this is an ezImage which owns mutable access to the storage
  m_DataPtr = ezBlobPtr<ezUInt8>(const_cast<ezUInt8*>(static_cast<const ezUInt8*>(imageData.GetPtr())), imageData.GetCount());
}

ezResult ezImageView::SaveTo(ezStringView sFileName) const
{
  EZ_LOG_BLOCK("Writing Image", sFileName);

  if (m_Format == ezImageFormat::UNKNOWN)
  {
    ezLog::Error("Cannot write image '{0}' - image data is invalid or empty", sFileName);
    return EZ_FAILURE;
  }

  ezFileWriter writer;
  if (writer.Open(sFileName) == EZ_FAILURE)
  {
    ezLog::Error("Failed to open image file '{0}'", sFileName);
    return EZ_FAILURE;
  }

  ezStringView it = ezPathUtils::GetFileExtension(sFileName);

  if (ezImageFileFormat* pFormat = ezImageFileFormat::GetWriterFormat(it.GetStartPointer()))
  {
    if (pFormat->WriteImage(writer, *this, it.GetStartPointer()) != EZ_SUCCESS)
    {
      ezLog::Error("Failed to write image file '{0}'", sFileName);
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  ezLog::Error("No known image file format for extension '{0}'", it);
  return EZ_FAILURE;
}

const ezImageHeader& ezImageView::GetHeader() const
{
  return *this;
}

ezImageView ezImageView::GetRowView(
  ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/, ezUInt32 y /*= 0*/, ezUInt32 z /*= 0*/, ezUInt32 uiPlaneIndex /*= 0*/) const
{
  ezImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);

  // Scale dimensions relative to the block size of the subformat
  ezImageFormat::Enum subFormat = ezImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex);
  header.SetWidth(GetWidth(uiMipLevel) * ezImageFormat::GetBlockWidth(subFormat) / ezImageFormat::GetBlockWidth(m_Format, uiPlaneIndex));
  header.SetHeight(ezImageFormat::GetBlockHeight(m_Format, 0) * ezImageFormat::GetBlockHeight(subFormat) / ezImageFormat::GetBlockHeight(m_Format, uiPlaneIndex));
  header.SetDepth(ezImageFormat::GetBlockDepth(subFormat) / ezImageFormat::GetBlockDepth(m_Format, uiPlaneIndex));
  header.SetImageFormat(ezImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex));

  ezUInt64 offset = 0;

  offset += GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  offset += z * GetDepthPitch(uiMipLevel, uiPlaneIndex);
  offset += y * GetRowPitch(uiMipLevel, uiPlaneIndex);

  ezBlobPtr<const ezUInt8> dataSlice = m_DataPtr.GetSubArray(offset, GetRowPitch(uiMipLevel, uiPlaneIndex));
  return ezImageView(header, ezConstByteBlobPtr(dataSlice.GetPtr(), dataSlice.GetCount()));
}

void ezImageView::ReinterpretAs(ezImageFormat::Enum format)
{
  EZ_ASSERT_DEBUG(
    ezImageFormat::IsCompressed(format) == ezImageFormat::IsCompressed(GetImageFormat()), "Cannot reinterpret compressed and non-compressed formats");

  EZ_ASSERT_DEBUG(ezImageFormat::GetBitsPerPixel(GetImageFormat()) == ezImageFormat::GetBitsPerPixel(format),
    "Cannot reinterpret between formats of different sizes");

  SetImageFormat(format);
}

ezUInt64 ezImageView::ComputeLayout()
{
  m_SubImageOffsets.Clear();
  m_SubImageOffsets.Reserve(m_uiNumMipLevels * m_uiNumFaces * m_uiNumArrayIndices * GetPlaneCount());

  ezUInt64 uiDataSize = 0;

  for (ezUInt32 uiArrayIndex = 0; uiArrayIndex < m_uiNumArrayIndices; uiArrayIndex++)
  {
    for (ezUInt32 uiFace = 0; uiFace < m_uiNumFaces; uiFace++)
    {
      for (ezUInt32 uiMipLevel = 0; uiMipLevel < m_uiNumMipLevels; uiMipLevel++)
      {
        for (ezUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); uiPlaneIndex++)
        {
          m_SubImageOffsets.PushBack(uiDataSize);

          uiDataSize += GetDepthPitch(uiMipLevel, uiPlaneIndex) * GetDepth(uiMipLevel);
        }
      }
    }
  }

  // Push back total size as a marker
  m_SubImageOffsets.PushBack(uiDataSize);

  return uiDataSize;
}

void ezImageView::ValidateSubImageIndices(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 uiPlaneIndex) const
{
  EZ_IGNORE_UNUSED(uiMipLevel);
  EZ_IGNORE_UNUSED(uiFace);
  EZ_IGNORE_UNUSED(uiArrayIndex);
  EZ_IGNORE_UNUSED(uiPlaneIndex);

  EZ_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
  EZ_ASSERT_DEV(uiFace < m_uiNumFaces, "Invalid uiFace");
  EZ_ASSERT_DEV(uiArrayIndex < m_uiNumArrayIndices, "Invalid array slice");
  EZ_ASSERT_DEV(uiPlaneIndex < GetPlaneCount(), "Invalid plane index");
}

const ezUInt64& ezImageView::GetSubImageOffset(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex, ezUInt32 uiPlaneIndex) const
{
  ValidateSubImageIndices(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  return m_SubImageOffsets[uiPlaneIndex + GetPlaneCount() * (uiMipLevel + m_uiNumMipLevels * (uiFace + m_uiNumFaces * uiArrayIndex))];
}

ezImage::ezImage()
{
  Clear();
}

ezImage::ezImage(const ezImageHeader& header)
{
  ResetAndAlloc(header);
}

ezImage::ezImage(const ezImageHeader& header, ezByteBlobPtr externalData)
{
  ResetAndUseExternalStorage(header, externalData);
}

ezImage::ezImage(ezImage&& other)
{
  ResetAndMove(std::move(other));
}

ezImage::ezImage(const ezImageView& other)
{
  ResetAndCopy(other);
}

void ezImage::operator=(ezImage&& rhs)
{
  ResetAndMove(std::move(rhs));
}

void ezImage::Clear()
{
  m_InternalStorage.Clear();

  ezImageView::Clear();
}

void ezImage::ResetAndAlloc(const ezImageHeader& header)
{
  const ezUInt64 requiredSize = header.ComputeDataSize();

  // it is debatable whether this function should reuse external storage, at all
  // however, it is especially dangerous to rely on the external storage being big enough, since many functions just take an ezImage as a
  // destination parameter and expect it to behave correctly when any of the Reset functions is called on it; it is not intuitive, that
  // Reset may fail due to how the image was previously reset

  // therefore, if external storage is insufficient, fall back to internal storage

  if (!UsesExternalStorage() || m_DataPtr.GetCount() < requiredSize)
  {
    m_InternalStorage.SetCountUninitialized(requiredSize);
    m_DataPtr = m_InternalStorage.GetBlobPtr<ezUInt8>();
  }

  ezImageView::ResetAndViewExternalStorage(header, ezConstByteBlobPtr(m_DataPtr.GetPtr(), m_DataPtr.GetCount()));
}

void ezImage::ResetAndUseExternalStorage(const ezImageHeader& header, ezByteBlobPtr externalData)
{
  m_InternalStorage.Clear();

  ezImageView::ResetAndViewExternalStorage(header, externalData);
}

void ezImage::ResetAndMove(ezImage&& other)
{
  static_cast<ezImageHeader&>(*this) = other.GetHeader();

  if (other.UsesExternalStorage())
  {
    m_InternalStorage.Clear();
    m_SubImageOffsets = std::move(other.m_SubImageOffsets);
    m_DataPtr = other.m_DataPtr;
    other.Clear();
  }
  else
  {
    m_InternalStorage = std::move(other.m_InternalStorage);
    m_SubImageOffsets = std::move(other.m_SubImageOffsets);
    m_DataPtr = m_InternalStorage.GetBlobPtr<ezUInt8>();
    other.Clear();
  }
}

void ezImage::ResetAndCopy(const ezImageView& other)
{
  ResetAndAlloc(other.GetHeader());

  memcpy(GetBlobPtr<ezUInt8>().GetPtr(), other.GetBlobPtr<ezUInt8>().GetPtr(), static_cast<size_t>(other.GetBlobPtr<ezUInt8>().GetCount()));
}

ezResult ezImage::LoadFrom(ezStringView sFileName)
{
  EZ_LOG_BLOCK("Loading Image", sFileName);

  EZ_PROFILE_SCOPE(ezPathUtils::GetFileNameAndExtension(sFileName).GetStartPointer());

  ezFileReader reader;
  if (reader.Open(sFileName) == EZ_FAILURE)
  {
    ezLog::Warning("Failed to open image file '{0}'", ezArgSensitive(sFileName, "File"));
    return EZ_FAILURE;
  }

  ezStringView it = ezPathUtils::GetFileExtension(sFileName);

  if (ezImageFileFormat* pFormat = ezImageFileFormat::GetReaderFormat(it.GetStartPointer()))
  {
    if (pFormat->ReadImage(reader, *this, it.GetStartPointer()) != EZ_SUCCESS)
    {
      ezLog::Warning("Failed to read image file '{0}'", ezArgSensitive(sFileName, "File"));
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  ezLog::Warning("No known image file format for extension '{0}'", it);

  return EZ_FAILURE;
}

ezResult ezImage::Convert(ezImageFormat::Enum targetFormat)
{
  return ezImageConversion::Convert(*this, *this, targetFormat);
}

ezImageView ezImageView::GetSubImageView(ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/) const
{
  ezImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);
  header.SetWidth(GetWidth(uiMipLevel));
  header.SetHeight(GetHeight(uiMipLevel));
  header.SetDepth(GetDepth(uiMipLevel));
  header.SetImageFormat(m_Format);

  const ezUInt64& offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, 0);
  ezUInt64 size = *(&offset + GetPlaneCount()) - offset;

  ezBlobPtr<const ezUInt8> subView = m_DataPtr.GetSubArray(offset, size);

  return ezImageView(header, ezConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

ezImage ezImage::GetSubImageView(ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/)
{
  ezImageView constView = ezImageView::GetSubImageView(uiMipLevel, uiFace, uiArrayIndex);

  // Create an ezImage attached to the view. Const cast is safe here since we own the storage.
  return ezImage(
    constView.GetHeader(), ezByteBlobPtr(const_cast<ezUInt8*>(constView.GetBlobPtr<ezUInt8>().GetPtr()), constView.GetBlobPtr<ezUInt8>().GetCount()));
}

ezImageView ezImageView::GetPlaneView(ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/, ezUInt32 uiPlaneIndex /*= 0*/) const
{
  ezImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);

  // Scale dimensions relative to the block size of the first plane which determines the "nominal" width, height and depth
  ezImageFormat::Enum subFormat = ezImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex);
  header.SetWidth(GetWidth(uiMipLevel) * ezImageFormat::GetBlockWidth(subFormat) / ezImageFormat::GetBlockWidth(m_Format, uiPlaneIndex));
  header.SetHeight(GetHeight(uiMipLevel) * ezImageFormat::GetBlockHeight(subFormat) / ezImageFormat::GetBlockHeight(m_Format, uiPlaneIndex));
  header.SetDepth(GetDepth(uiMipLevel) * ezImageFormat::GetBlockDepth(subFormat) / ezImageFormat::GetBlockDepth(m_Format, uiPlaneIndex));
  header.SetImageFormat(subFormat);

  const ezUInt64& offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  ezUInt64 size = *(&offset + 1) - offset;

  ezBlobPtr<const ezUInt8> subView = m_DataPtr.GetSubArray(offset, size);

  return ezImageView(header, ezConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

ezImage ezImage::GetPlaneView(ezUInt32 uiMipLevel /* = 0 */, ezUInt32 uiFace /* = 0 */, ezUInt32 uiArrayIndex /* = 0 */, ezUInt32 uiPlaneIndex /* = 0 */)
{
  ezImageView constView = ezImageView::GetPlaneView(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);

  // Create an ezImage attached to the view. Const cast is safe here since we own the storage.
  return ezImage(
    constView.GetHeader(), ezByteBlobPtr(const_cast<ezUInt8*>(constView.GetBlobPtr<ezUInt8>().GetPtr()), constView.GetBlobPtr<ezUInt8>().GetCount()));
}

ezImage ezImage::GetSliceView(ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/, ezUInt32 z /*= 0*/, ezUInt32 uiPlaneIndex /*= 0*/)
{
  ezImageView constView = ezImageView::GetSliceView(uiMipLevel, uiFace, uiArrayIndex, z, uiPlaneIndex);

  // Create an ezImage attached to the view. Const cast is safe here since we own the storage.
  return ezImage(
    constView.GetHeader(), ezByteBlobPtr(const_cast<ezUInt8*>(constView.GetBlobPtr<ezUInt8>().GetPtr()), constView.GetBlobPtr<ezUInt8>().GetCount()));
}

ezImageView ezImageView::GetSliceView(ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/, ezUInt32 z /*= 0*/, ezUInt32 uiPlaneIndex /*= 0*/) const
{
  ezImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);

  // Scale dimensions relative to the block size of the first plane which determines the "nominal" width, height and depth
  ezImageFormat::Enum subFormat = ezImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex);
  header.SetWidth(GetWidth(uiMipLevel) * ezImageFormat::GetBlockWidth(subFormat) / ezImageFormat::GetBlockWidth(m_Format, uiPlaneIndex));
  header.SetHeight(GetHeight(uiMipLevel) * ezImageFormat::GetBlockHeight(subFormat) / ezImageFormat::GetBlockHeight(m_Format, uiPlaneIndex));
  header.SetDepth(ezImageFormat::GetBlockDepth(subFormat) / ezImageFormat::GetBlockDepth(m_Format, uiPlaneIndex));
  header.SetImageFormat(subFormat);

  ezUInt64 offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex) + z * GetDepthPitch(uiMipLevel, uiPlaneIndex);
  ezUInt64 size = GetDepthPitch(uiMipLevel, uiPlaneIndex);

  ezBlobPtr<const ezUInt8> subView = m_DataPtr.GetSubArray(offset, size);

  return ezImageView(header, ezConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

bool ezImage::UsesExternalStorage() const
{
  return m_InternalStorage.GetBlobPtr<ezUInt8>() != m_DataPtr;
}
