#include <Image.h>

#include <Foundation/Basics.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

#include <Foundation/Logging/Log.h>

#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringIterator.h>

#include <ImageFileFormat.h>

void ezImage::AllocateImageData()
{
  m_subImages.SetCount(m_uiNumMipLevels * m_uiNumFaces * m_uiNumArrayIndices);

  int uiDataSize = 0;

  bool bCompressed = ezImageFormat::GetType(m_format) == ezImageFormatType::BLOCK_COMPRESSED;
  ezUInt32 uiBitsPerPixel = ezImageFormat::GetBitsPerPixel(m_format);

  for(ezUInt32 uiArrayIndex = 0; uiArrayIndex < m_uiNumArrayIndices; uiArrayIndex++)
  {
    for(ezUInt32 uiFace = 0; uiFace < m_uiNumFaces; uiFace++)
    {
      for(ezUInt32 uiMipLevel = 0; uiMipLevel < m_uiNumMipLevels; uiMipLevel++)
      {
        SubImage& subImage = GetSubImage(uiMipLevel, uiFace, uiArrayIndex);

        subImage.m_uiDataOffset = uiDataSize;

        if(bCompressed)
        {
          ezUInt32 uiBlockSize = 4;
          subImage.m_uiRowPitch = 0;
          subImage.m_uiDepthPitch = GetNumBlocksX(uiMipLevel) * GetNumBlocksY(uiMipLevel) * uiBlockSize * uiBlockSize * uiBitsPerPixel / 8;
        }
        else
        {
          subImage.m_uiRowPitch = ((GetWidth(uiMipLevel) * uiBitsPerPixel / 8 - 1) / m_uiRowAlignment + 1) * m_uiRowAlignment;
          subImage.m_uiDepthPitch = ((GetWidth(uiMipLevel) * subImage.m_uiRowPitch - 1) / m_uiDepthAlignment + 1) * m_uiDepthAlignment;
        }

        uiDataSize += subImage.m_uiDepthPitch * GetDepth(uiMipLevel);
      }
    }
  }

  m_data.SetCount(uiDataSize + 16);
}

ezResult ezImage::LoadFrom(const char* szFileName)
{
  ezFileReader reader;
  if(reader.Open(szFileName) == EZ_FAILURE)
  {
    ezLog::Warning("Failed to open image file '%s'", szFileName);
    return EZ_FAILURE;
  }

  ezStringIterator it = ezPathUtils::GetFileExtension(szFileName);
  
  for(ezIImageFileFormat* pFormat = ezIImageFileFormat::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if(pFormat->IsKnownExtension(it.GetData()))
    {
      ezStringBuilder outError;
      if(pFormat->ReadImage(reader, *this, outError) != EZ_SUCCESS)
      {
        ezLog::Warning("Failed to read image file '%s': %s", szFileName, outError.GetData());
        return EZ_FAILURE;
      }

      return EZ_SUCCESS;
    }
  }

  ezLog::Warning("No known image file format for extension '%s'", it.GetData());

  return EZ_FAILURE;
}

ezResult ezImage::SaveTo(const char* szFileName)
{
  ezFileWriter writer;
  if(writer.Open(szFileName) == EZ_FAILURE)
  {
    ezLog::Warning("Failed to open image file '%s'", szFileName);
    return EZ_FAILURE;
  }

  ezStringIterator it = ezPathUtils::GetFileExtension(szFileName);

  for(ezIImageFileFormat* pFormat = ezIImageFileFormat::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if(pFormat->IsKnownExtension(it.GetData()))
    {
      ezStringBuilder outError;
      if(pFormat->WriteImage(writer, *this, outError) != EZ_SUCCESS)
      {
        ezLog::Warning("Failed to write image file '%s': %s", szFileName, outError.GetData());
        return EZ_FAILURE;
      }

      return EZ_SUCCESS;
    }
  }

  ezLog::Warning("No known image file format for extension '%s'", it.GetData());

  return EZ_FAILURE;
}

