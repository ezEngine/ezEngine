#include <PCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Image/Formats/ImageFileFormat.h>
#include <Foundation/Image/Image.h>
#include <Foundation/Image/ImageConversion.h>

ezImageView::ezImageView()
{
  Reset();
}

ezImageView::ezImageView(const ezImageHeader& header, ezArrayPtr<const void> imageData)
{
  Reset(header, imageData);
}

void ezImageView::Reset()
{
  ezImageHeader::Reset();
  m_subImageOffsets.Clear();
  m_dataPtr.Reset();
}

void ezImageView::Reset(const ezImageHeader& header, ezArrayPtr<const void> imageData)
{
  static_cast<ezImageHeader&>(*this) = header;

  ezUInt32 dataSize = ComputeLayout();

  EZ_ASSERT_DEV(imageData.GetCount() == dataSize, "Provided image storage (%i bytes) doesn't match required data size (%i bytes)",
                imageData.GetCount(), dataSize);

  // Const cast is safe here as we will only perform non-const access if this is an ezImage which owns mutable access to the storage
  m_dataPtr = ezArrayPtr<ezUInt8>(const_cast<ezUInt8*>(static_cast<const ezUInt8*>(imageData.GetPtr())), imageData.GetCount());
}

ezResult ezImageView::SaveTo(const char* szFileName, ezLogInterface* pLog) const
{
  EZ_LOG_BLOCK(pLog, "Writing Image", szFileName);

  ezFileWriter writer;
  if (writer.Open(szFileName) == EZ_FAILURE)
  {
    ezLog::Warning(pLog, "Failed to open image file '{0}'", szFileName);
    return EZ_FAILURE;
  }

  ezStringView it = ezPathUtils::GetFileExtension(szFileName);

  for (ezImageFileFormat* pFormat = ezImageFileFormat::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if (pFormat->CanWriteFileType(it.GetData()))
    {
      if (pFormat->WriteImage(writer, *this, pLog) != EZ_SUCCESS)
      {
        ezLog::Warning(pLog, "Failed to write image file '{0}'", szFileName);
        return EZ_FAILURE;
      }

      return EZ_SUCCESS;
    }
  }

  ezLog::Warning(pLog, "No known image file format for extension '{0}'", it);

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

  ezUInt32 offset = 0;

  offset += GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex);
  offset += z * GetDepthPitch(uiMipLevel);
  offset += y * GetRowPitch(uiMipLevel);

  ezArrayPtr<const ezUInt8> dataSlice = m_dataPtr.GetSubArray(offset, GetRowPitch(uiMipLevel));
  return ezImageView(header, ezArrayPtr<const void>(dataSlice.GetPtr(), dataSlice.GetCount()));
}

ezUInt32 ezImageView::ComputeLayout()
{
  m_subImageOffsets.Clear();
  m_subImageOffsets.Reserve(m_uiNumMipLevels * m_uiNumFaces * m_uiNumArrayIndices);

  ezUInt32 uiDataSize = 0;

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

const ezUInt32& ezImageView::GetSubImageOffset(ezUInt32 uiMipLevel, ezUInt32 uiFace, ezUInt32 uiArrayIndex) const
{
  ValidateSubImageIndices(uiMipLevel, uiFace, uiArrayIndex);
  return m_subImageOffsets[uiMipLevel + m_uiNumMipLevels * (uiFace + m_uiNumFaces * uiArrayIndex)];
}

ezImage::ezImage()
{
  Reset();
}

ezImage::ezImage(const ezImageHeader& header)
{
  Reset(header);
}

ezImage::ezImage(const ezImageHeader& header, ezArrayPtr<void> externalData)
{
  Reset(header, externalData);
}

ezImage::ezImage(ezImage&& other)
{
  Reset(std::move(other));
}

ezImage::ezImage(const ezImageView& other)
{
  Reset(other);
}

ezImage& ezImage::operator=(ezImage&& other)
{
  Reset(std::move(other));
  return *this;
}

ezImage& ezImage::operator=(const ezImageView& other)
{
  Reset(other);
  return *this;
}

void ezImage::Reset()
{
  m_internalStorage.Clear();

  ezImageView::Reset();
}

void ezImage::Reset(const ezImageHeader& header)
{
  if (!UsesExternalStorage())
  {
    ezUInt32 dataSize = header.ComputeDataSize();
    m_internalStorage.SetCountUninitialized(dataSize);
    m_dataPtr = m_internalStorage.GetArrayPtr();
  }

  ezImageView::Reset(header, ezArrayPtr<const void>(m_dataPtr.GetPtr(), m_dataPtr.GetCount()));
}

void ezImage::Reset(const ezImageHeader& header, ezArrayPtr<void> externalData)
{
  m_internalStorage.Clear();

  ezImageView::Reset(header, externalData);
}

void ezImage::Reset(ezImage&& other)
{
  static_cast<ezImageHeader&>(*this) = other.GetHeader();

  if (other.UsesExternalStorage())
  {
    m_internalStorage.Clear();
    m_subImageOffsets = std::move(other.m_subImageOffsets);
    m_dataPtr = other.m_dataPtr;
    other.ezImageView::Reset();
  }
  else
  {
    m_internalStorage = std::move(other.m_internalStorage);
    m_subImageOffsets = std::move(other.m_subImageOffsets);
    m_dataPtr = m_internalStorage.GetArrayPtr();
    other.ezImageView::Reset();
  }
}

void ezImage::Reset(const ezImageView& other)
{
  Reset(other.GetHeader());

  memcpy(GetArrayPtr<void>().GetPtr(), other.GetArrayPtr<void>().GetPtr(), GetArrayPtr<void>().GetCount());
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

  for (ezImageFileFormat* pFormat = ezImageFileFormat::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if (pFormat->CanReadFileType(it.GetData()))
    {
      if (pFormat->ReadImage(reader, *this, pLog) != EZ_SUCCESS)
      {
        ezLog::Warning(pLog, "Failed to read image file '{0}'", szFileName);
        return EZ_FAILURE;
      }

      return EZ_SUCCESS;
    }
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

  ezArrayPtr<const ezUInt8> subView = m_dataPtr.GetSubArray(offset, size);

  return ezImageView(header, ezArrayPtr<const void>(subView.GetPtr(), subView.GetCount()));
}

ezImage ezImage::GetSubImageView(ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/)
{
  ezImageView constView = ezImageView::GetSubImageView(uiMipLevel, uiFace, uiArrayIndex);

  // Create an ezImage attached to the view. Const cast is safe here since we own the storage.
  return ezImage(constView.GetHeader(), ezArrayPtr<void>(const_cast<ezUInt8*>(constView.GetArrayPtr<ezUInt8>().GetPtr()),
                                                         constView.GetArrayPtr<ezUInt8>().GetCount()));
}

ezImage ezImage::GetSliceView(ezUInt32 uiMipLevel /*= 0*/, ezUInt32 uiFace /*= 0*/, ezUInt32 uiArrayIndex /*= 0*/, ezUInt32 z /*= 0*/)
{
  ezImageView constView = ezImageView::GetSliceView(uiMipLevel, uiFace, uiArrayIndex, z);

  // Create an ezImage attached to the view. Const cast is safe here since we own the storage.
  return ezImage(constView.GetHeader(), ezArrayPtr<void>(const_cast<ezUInt8*>(constView.GetArrayPtr<ezUInt8>().GetPtr()),
                                                         constView.GetArrayPtr<ezUInt8>().GetCount()));
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

  ezArrayPtr<const ezUInt8> subView = m_dataPtr.GetSubArray(offset, size);

  return ezImageView(header, ezArrayPtr<const void>(subView.GetPtr(), subView.GetCount()));
}

bool ezImage::UsesExternalStorage() const
{
  return m_internalStorage.GetArrayPtr() != m_dataPtr;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Image_Implementation_Image);
