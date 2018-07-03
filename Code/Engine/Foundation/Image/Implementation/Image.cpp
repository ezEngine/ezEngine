#include <PCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Image/Formats/ImageFileFormat.h>
#include <Foundation/Image/Image.h>

void ezImage::AllocateImageData()
{
  m_subImages.SetCountUninitialized(m_uiNumMipLevels * m_uiNumFaces * m_uiNumArrayIndices);

  int uiDataSize = 0;

  bool bCompressed = ezImageFormat::GetType(m_format) == ezImageFormatType::BLOCK_COMPRESSED;
  ezUInt32 uiBitsPerPixel = ezImageFormat::GetBitsPerPixel(m_format);

  for (ezUInt32 uiArrayIndex = 0; uiArrayIndex < m_uiNumArrayIndices; uiArrayIndex++)
  {
    for (ezUInt32 uiFace = 0; uiFace < m_uiNumFaces; uiFace++)
    {
      for (ezUInt32 uiMipLevel = 0; uiMipLevel < m_uiNumMipLevels; uiMipLevel++)
      {
        SubImage& subImage = GetSubImage(uiMipLevel, uiFace, uiArrayIndex);

        subImage.m_uiDataOffset = uiDataSize;

        if (bCompressed)
        {
          ezUInt32 uiBlockSize = 4;
          subImage.m_uiRowPitch = 0;
          subImage.m_uiDepthPitch = GetNumBlocksX(uiMipLevel) * GetNumBlocksY(uiMipLevel) * uiBlockSize * uiBlockSize * uiBitsPerPixel / 8;
        }
        else
        {
          subImage.m_uiRowPitch = GetWidth(uiMipLevel) * uiBitsPerPixel / 8;
          subImage.m_uiDepthPitch = GetHeight(uiMipLevel) * subImage.m_uiRowPitch;
        }

        uiDataSize += subImage.m_uiDepthPitch * GetDepth(uiMipLevel);
      }
    }
  }

  m_data.SetCountUninitialized(uiDataSize + 16);
}


void ezImage::Swap(ezImage& other)
{
  ezMath::Swap(m_uiNumMipLevels, other.m_uiNumMipLevels);
  ezMath::Swap(m_uiNumFaces, other.m_uiNumFaces);
  ezMath::Swap(m_uiNumArrayIndices, other.m_uiNumArrayIndices);
  ezMath::Swap(m_uiWidth, other.m_uiWidth);
  ezMath::Swap(m_uiHeight, other.m_uiHeight);
  ezMath::Swap(m_uiDepth, other.m_uiDepth);
  ezMath::Swap(m_format, other.m_format);

  m_subImages.Swap(other.m_subImages);
  m_data.Swap(other.m_data);
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

ezResult ezImage::SaveTo(const char* szFileName, ezLogInterface* pLog)
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



EZ_STATICLINK_FILE(Foundation, Foundation_Image_Implementation_Image);
