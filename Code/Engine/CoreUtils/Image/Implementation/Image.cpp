#include <CoreUtils/PCH.h>
#include <CoreUtils/Image/Image.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

#include <Foundation/Logging/Log.h>

#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringView.h>

#include <CoreUtils/Image/Formats/ImageFileFormat.h>

void ezImage::AllocateImageData()
{
  m_subImages.SetCount(m_uiNumMipLevels * m_uiNumFaces * m_uiNumArrayIndices);

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

  m_data.SetCount(uiDataSize + 16);
}

ezResult ezImage::LoadFrom(const char* szFileName, ezLogInterface* pLog)
{
  EZ_LOG_BLOCK(pLog, "Loading Image", szFileName);

  ezFileReader reader;
  if (reader.Open(szFileName) == EZ_FAILURE)
  {
    ezLog::Warning(pLog, "Failed to open image file '%s'", szFileName);
    return EZ_FAILURE;
  }

  ezStringView it = ezPathUtils::GetFileExtension(szFileName);
  
  for (ezImageFileFormatBase* pFormat = ezImageFileFormatBase::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if (pFormat->CanReadFileType(it.GetData()))
    {
      if (pFormat->ReadImage(reader, *this, pLog) != EZ_SUCCESS)
      {
        ezLog::Warning(pLog, "Failed to read image file '%s'", szFileName);
        return EZ_FAILURE;
      }

      return EZ_SUCCESS;
    }
  }

  ezLog::Warning(pLog, "No known image file format for extension '%s'", it.GetData());

  return EZ_FAILURE;
}

ezResult ezImage::SaveTo(const char* szFileName, ezLogInterface* pLog)
{
  EZ_LOG_BLOCK(pLog, "Writing Image", szFileName);

  ezFileWriter writer;
  if (writer.Open(szFileName) == EZ_FAILURE)
  {
    ezLog::Warning(pLog, "Failed to open image file '%s'", szFileName);
    return EZ_FAILURE;
  }

  ezStringView it = ezPathUtils::GetFileExtension(szFileName);

  for (ezImageFileFormatBase* pFormat = ezImageFileFormatBase::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if (pFormat->CanWriteFileType(it.GetData()))
    {
      if (pFormat->WriteImage(writer, *this, pLog) != EZ_SUCCESS)
      {
        ezLog::Warning(pLog, "Failed to write image file '%s'", szFileName);
        return EZ_FAILURE;
      }

      return EZ_SUCCESS;
    }
  }

  ezLog::Warning(pLog, "No known image file format for extension '%s'", it.GetData());

  return EZ_FAILURE;
}



EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Image_Implementation_Image);

