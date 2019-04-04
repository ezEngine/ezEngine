#include <TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

ezImageView::ezImageView()
{
  Clear();
}

ezImageView::ezImageView(const ezImageHeader& header, ezBlobPtr<const void> imageData)
{
  ResetAndViewExternalStorage(header, imageData);
}

void ezImageView::Clear()
{
  ezImageHeader::Clear();
  m_subImageOffsets.Clear();
  m_dataPtr.Clear();
}

bool ezImageView::IsValid() const
{
  return !m_dataPtr.IsEmpty();
}

void ezImageView::ResetAndViewExternalStorage(const ezImageHeader& header, ezBlobPtr<const void> imageData)
{
  static_cast<ezImageHeader&>(*this) = header;

  ezUInt64 dataSize = ComputeLayout();

  EZ_ASSERT_DEV(imageData.GetCount() == dataSize, "Provided image storage ({} bytes) doesn't match required data size ({} bytes)",
                imageData.GetCount(), dataSize);

  // Const cast is safe here as we will only perform non-const access if this is an ezImage which owns mutable access to the storage
  m_dataPtr = ezBlobPtr<ezUInt8>(const_cast<ezUInt8*>(static_cast<const ezUInt8*>(imageData.GetPtr())), imageData.GetCount());
}

ezResult ezImageView::SaveTo(const char* szFileName, ezLogInterface* pLog) const
{
  EZ_LOG_BLOCK(pLog, "Writing Image", szFileName);

  if (m_format == ezImageFormat::UNKNOWN)
  {
    ezLog::Error(pLog, "Cannot write image '{0}' - image data is invalid or empty", szFileName);
    return EZ_FAILURE;
  }

  ezFileWriter writer;
  if (writer.Open(szFileName) == EZ_FAILURE)
  {
    ezLog::Error(pLog, "Failed to open image file '{0}'", szFileName);
    return EZ_FAILURE;
  }

  ezStringView it = ezPathUtils::GetFileExtension(szFileName);

  if (ezImageFileFormat* pFormat = ezImageFileFormat::GetWriterFormat(it.GetData()))
  {
    if (pFormat->WriteImage(writer, *this, pLog, it.GetData()) != EZ_SUCCESS)
    {
      ezLog::Error(pLog, "Failed to write image file '{0}'", szFileName);
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  ezLog::Error(pLog, "No known image file format for extension '{0}'", it);
  return EZ_FAILURE;
}

const ezImageHeader& ezImageView::GetHeader() const
{
  return *this;
}

ezImageView ezImageView::GetRowView(ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/, ezUInt32 y /*= 0*/,
                                    ezUInt32 z /*= 0*/) const
{
  ezImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);
  header.SetWidth(GetWidth(uiMipLevel));
  header.SetHeight(ezImageFormat::GetBlockHeight(m_format));
  header.SetDepth(1);
  header.SetImageFormat(m_format);

  ezUInt64 offset = 0;

  offset += GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex);
  offset += z * GetDepthPitch(uiMipLevel);
  offset += y * GetRowPitch(uiMipLevel);

  ezBlobPtr<const ezUInt8> dataSlice = m_dataPtr.GetSubArray(offset, GetRowPitch(uiMipLevel));
  return ezImageView(header, ezBlobPtr<const void>(dataSlice.GetPtr(), dataSlice.GetCount()));
}

void ezImageView::ReinterpretAs(ezImageFormat::Enum format)
{
  EZ_ASSERT_DEBUG(ezImageFormat::IsCompressed(format) == ezImageFormat::IsCompressed(GetImageFormat()), "Cannot reinterpret compressed and non-compressed formats");

  EZ_ASSERT_DEBUG(ezImageFormat::GetBitsPerPixel(GetImageFormat()) == ezImageFormat::GetBitsPerPixel(format), "Cannot reinterpret between formats of different sizes");

  SetImageFormat(format);
}

ezUInt64 ezImageView::ComputeLayout()
{
  m_subImageOffsets.Clear();
  m_subImageOffsets.Reserve(m_uiNumMipLevels * m_uiNumFaces * m_uiNumArrayIndices);

  ezUInt64 uiDataSize = 0;

  bool bCompressed = ezImageFormat::GetType(m_format) == ezImageFormatType::BLOCK_COMPRESSED;
  ezUInt32 uiBitsPerPixel = ezImageFormat::GetBitsPerPixel(m_format);

  for (ezUInt32 uiArrayIndex = 0; uiArrayIndex < m_uiNumArrayIndices; uiArrayIndex++)
  {
    for (ezUInt32 uiFace = 0; uiFace < m_uiNumFaces; uiFace++)
    {
      for (ezUInt32 uiMipLevel = 0; uiMipLevel < m_uiNumMipLevels; uiMipLevel++)
      {
        m_subImageOffsets.PushBack(uiDataSize);

        uiDataSize += GetDepthPitch(uiMipLevel) * GetDepth(uiMipLevel);
      }
    }
  }

  // Push back total size as a marker
  m_subImageOffsets.PushBack(uiDataSize);

  return uiDataSize;
}

void ezImageView::ValidateSubImageIndices(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const
{
  EZ_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
  EZ_ASSERT_DEV(uiFace < m_uiNumFaces, "Invalid uiFace");
  EZ_ASSERT_DEV(uiArrayIndex < m_uiNumArrayIndices, "Invalid array slice");
}

const ezUInt64& ezImageView::GetSubImageOffset(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const
{
  ValidateSubImageIndices(uiMipLevel, uiFace, uiArrayIndex);
  return m_subImageOffsets[uiMipLevel + m_uiNumMipLevels * (uiFace + m_uiNumFaces * uiArrayIndex)];
}

ezImage::ezImage()
{
  Clear();
}

ezImage::ezImage(const ezImageHeader& header)
{
  ResetAndAlloc(header);
}

ezImage::ezImage(const ezImageHeader& header, ezBlobPtr<void> externalData)
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
  m_internalStorage.Clear();

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

  if (!UsesExternalStorage() || m_dataPtr.GetCount() < requiredSize)
  {
    m_internalStorage.SetCountUninitialized(requiredSize);
    m_dataPtr = m_internalStorage.GetBlobPtr<ezUInt8>();
  }

  ezImageView::ResetAndViewExternalStorage(header, ezBlobPtr<const void>(m_dataPtr.GetPtr(), m_dataPtr.GetCount()));
}

void ezImage::ResetAndUseExternalStorage(const ezImageHeader& header, ezBlobPtr<void> externalData)
{
  m_internalStorage.Clear();

  ezImageView::ResetAndViewExternalStorage(header, externalData);
}

void ezImage::ResetAndMove(ezImage&& other)
{
  static_cast<ezImageHeader&>(*this) = other.GetHeader();

  if (other.UsesExternalStorage())
  {
    m_internalStorage.Clear();
    m_subImageOffsets = std::move(other.m_subImageOffsets);
    m_dataPtr = other.m_dataPtr;
    other.Clear();
  }
  else
  {
    m_internalStorage = std::move(other.m_internalStorage);
    m_subImageOffsets = std::move(other.m_subImageOffsets);
    m_dataPtr = m_internalStorage.GetBlobPtr<ezUInt8>();
    other.Clear();
  }
}

void ezImage::ResetAndCopy(const ezImageView& other)
{
  ResetAndAlloc(other.GetHeader());

  memcpy(GetBlobPtr<void>().GetPtr(), other.GetBlobPtr<void>().GetPtr(), GetBlobPtr<void>().GetCount());
}

ezResult ezImage::LoadFrom(const char* szFileName, ezLogInterface* pLog)
{
  EZ_LOG_BLOCK(pLog, "Loading Image", szFileName);

  ezFileReader reader;
  if (reader.Open(szFileName) == EZ_FAILURE)
  {
    ezLog::Warning(pLog, "Failed to open image file '{0}'", szFileName);
    return EZ_FAILURE;
  }

  ezStringView it = ezPathUtils::GetFileExtension(szFileName);

  if (ezImageFileFormat* pFormat = ezImageFileFormat::GetReaderFormat(it.GetData()))
  {
      if (pFormat->ReadImage(reader, *this, pLog, it.GetData()) != EZ_SUCCESS)
      {
        ezLog::Warning(pLog, "Failed to read image file '{0}'", szFileName);
        return EZ_FAILURE;
      }

      return EZ_SUCCESS;
  }

  ezLog::Warning(pLog, "No known image file format for extension '{0}'", it);

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
  header.SetImageFormat(m_format);

  const ezUInt32& offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex);
  ezUInt32 size = *(&offset + 1) - offset;

  ezBlobPtr<const ezUInt8> subView = m_dataPtr.GetSubArray(offset, size);

  return ezImageView(header, ezBlobPtr<const void>(subView.GetPtr(), subView.GetCount()));
}

ezImage ezImage::GetSubImageView(ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/)
{
  ezImageView constView = ezImageView::GetSubImageView(uiMipLevel, uiFace, uiArrayIndex);

  // Create an ezImage attached to the view. Const cast is safe here since we own the storage.
  return ezImage(constView.GetHeader(), ezBlobPtr<void>(const_cast<ezUInt8*>(constView.GetBlobPtr<ezUInt8>().GetPtr()),
                                                         constView.GetBlobPtr<ezUInt8>().GetCount()));
}

ezImage ezImage::GetSliceView(ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/, ezUInt32 z /*= 0*/)
{
  ezImageView constView = ezImageView::GetSliceView(uiMipLevel, uiFace, uiArrayIndex, z);

  // Create an ezImage attached to the view. Const cast is safe here since we own the storage.
  return ezImage(constView.GetHeader(), ezBlobPtr<void>(const_cast<ezUInt8*>(constView.GetBlobPtr<ezUInt8>().GetPtr()),
                                                         constView.GetBlobPtr<ezUInt8>().GetCount()));
}

ezImageView ezImageView::GetSliceView(ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/,
                                      ezUInt32 z /*= 0*/) const
{
  ezImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);
  header.SetWidth(GetWidth(uiMipLevel));
  header.SetHeight(GetHeight(uiMipLevel));
  header.SetDepth(1);
  header.SetImageFormat(m_format);

  ezUInt32 offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex) + z * GetDepthPitch(uiMipLevel);
  ezUInt32 size = GetDepthPitch(uiMipLevel);

  ezBlobPtr<const ezUInt8> subView = m_dataPtr.GetSubArray(offset, size);

  return ezImageView(header, ezBlobPtr<const void>(subView.GetPtr(), subView.GetCount()));
}

bool ezImage::UsesExternalStorage() const
{
  return m_internalStorage.GetBlobPtr<ezUInt8>() != m_dataPtr;
}

EZ_STATICLINK_FILE(Texture, Texture_Image_Implementation_Image);

