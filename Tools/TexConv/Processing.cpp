#include "Main.h"

ezResult ezTexConv::PassImageThrough()
{
  WriteTexHeader();

  ezImage* pImg = &m_InputImages[0];

  ezDdsFileFormat writer;
  if (writer.WriteImage(m_FileOut, *pImg, ezLog::GetThreadLocalLogSystem()).Failed())
  {
    SetReturnCode(TexConvReturnCodes::FAILED_PASS_THROUGH);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv::GenerateMipmaps()
{
  if (m_bGeneratedMipmaps)
  {
    shared_ptr<ScratchImage> pNewScratch = make_shared<ScratchImage>();

    if (FAILED(GenerateMipMaps(m_pCurrentImage->GetImages(), m_pCurrentImage->GetImageCount(), m_pCurrentImage->GetMetadata(), TEX_FILTER_SEPARATE_ALPHA, 0, *pNewScratch.get())))
    {
      SetReturnCode(TexConvReturnCodes::FAILED_MIPMAP_GENERATION);
      ezLog::Error("Mipmap generation failed");
      return EZ_FAILURE;
    }

    m_pCurrentImage = pNewScratch;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv::ApplyPremultiplyAlpha()
{
  if (m_bPremultiplyAlpha)
  {
    shared_ptr<ScratchImage> pNewScratch = make_shared<ScratchImage>();

    if (FAILED(PremultiplyAlpha(m_pCurrentImage->GetImages(), m_pCurrentImage->GetImageCount(), m_pCurrentImage->GetMetadata(), m_bSRGBOutput ? TEX_PMALPHA_SRGB : TEX_PMALPHA_IGNORE_SRGB, *pNewScratch.get())))
    {
      SetReturnCode(TexConvReturnCodes::FAILED_PREMULTIPLY_ALPHA);
      ezLog::Error("Pre-multiplying alpha failed");
      return EZ_FAILURE;
    }

    m_pCurrentImage = pNewScratch;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv::ConvertToOutputFormat()
{
  const ezImageFormat::Enum outputFormat = ChooseOutputFormat(false /*m_bSRGBOutput*/, m_bAlphaIsMaskOnly); // we don't want the implicit sRGB conversion of MS TexConv, so just write to non-sRGB target
  const DXGI_FORMAT dxgi = (DXGI_FORMAT)ezImageFormatMappings::ToDxgiFormat(outputFormat);

  if (m_bCompress)
  {
    shared_ptr<ScratchImage> pNewScratch = make_shared<ScratchImage>();

    // Some compressions are unbearably slow on the CPU. Use the GPU version if available.
    bool bCompressionDone = false;
    if (m_pD3dDevice)
    {
      if (SUCCEEDED(Compress(m_pD3dDevice, m_pCurrentImage->GetImages(), m_pCurrentImage->GetImageCount(), m_pCurrentImage->GetMetadata(), dxgi, TEX_COMPRESS_DEFAULT, 1.0f, *pNewScratch.get())))
      {
        // Not all formats can be compressed on the GPU. Fall back to CPU in case GPU compression fails.
        bCompressionDone = true;
      }
    }

    if(!bCompressionDone)
    {
      if (FAILED(Compress(m_pCurrentImage->GetImages(), m_pCurrentImage->GetImageCount(), m_pCurrentImage->GetMetadata(), dxgi, TEX_COMPRESS_DEFAULT, 1.0f, *pNewScratch.get())))
      {
        SetReturnCode(TexConvReturnCodes::FAILED_BC_COMPRESSION);
        ezLog::Error("Block compression failed");
        return EZ_FAILURE;
      }
    }

    m_pCurrentImage = pNewScratch;
  }
  else
  {
    if (outputFormat != ezImageFormatMappings::FromDxgiFormat(m_pCurrentImage->GetMetadata().format))
    {
      shared_ptr<ScratchImage> pNewScratch = make_shared<ScratchImage>();

      if (FAILED(Convert(m_pCurrentImage->GetImages(), m_pCurrentImage->GetImageCount(), m_pCurrentImage->GetMetadata(), dxgi, TEX_FILTER_DEFAULT, 0.0f, *pNewScratch.get())))
      {
        SetReturnCode(TexConvReturnCodes::FAILED_CONVERT_TO_OUTPUT_FORMAT);
        ezLog::Error("Failed to convert uncompressed image to {0} channels", m_uiOutputChannels);
        return EZ_FAILURE;
      }

      m_pCurrentImage = pNewScratch;
    }
  }

  // enforce sRGB format
  if (m_bSRGBOutput)
  {
    // up until here we ignore the sRGB format, to prevent automatic conversions
    // here we just claim that the data is sRGB and get around any conversion
    const ezImageFormat::Enum finalOutputFormat = ChooseOutputFormat(m_bSRGBOutput, m_bAlphaIsMaskOnly);
    const DXGI_FORMAT finalFormatDXGI = (DXGI_FORMAT)ezImageFormatMappings::ToDxgiFormat(finalOutputFormat);

    m_pCurrentImage->OverrideFormat(finalFormatDXGI);
  }

  return EZ_SUCCESS;
}


ezResult ezTexConv::SaveResultToDDS()
{
  const ezImageFormat::Enum dxgiOutputFormat = ezImageFormatMappings::FromDxgiFormat((ezUInt32)m_pCurrentImage->GetMetadata().format);
  ezLog::Info("Output Format: {0}", ezImageFormat::GetName(dxgiOutputFormat));

  if (FAILED(SaveToDDSMemory(m_pCurrentImage->GetImages(), m_pCurrentImage->GetImageCount(), m_pCurrentImage->GetMetadata(), 0, m_outputBlob)))
  {
    SetReturnCode(TexConvReturnCodes::FAILED_SAVE_AS_DDS);
    ezLog::Error("Failed to write image to file '{0}'", m_sOutputFile.GetData());
    return EZ_FAILURE;
  }

  WriteTexHeader();

  m_FileOut.WriteBytes(m_outputBlob.GetBufferPointer(), m_outputBlob.GetBufferSize());

  return EZ_SUCCESS;
}

ezResult ezTexConv::CreateTexture2D(ezImage* pImage, bool bCheckAlphaIsMask)
{
  if (pImage == nullptr)
    return EZ_FAILURE;

  m_bAlphaIsMaskOnly = false;
  if (bCheckAlphaIsMask && m_uiOutputChannels == 4 && m_bCompress)
  {
    m_bAlphaIsMaskOnly = IsImageAlphaBinaryMask(*pImage);
  }

  Image srcImg;
  srcImg.width = pImage->GetWidth();
  srcImg.height = pImage->GetHeight();
  srcImg.rowPitch = pImage->GetRowPitch();
  srcImg.slicePitch = pImage->GetDepthPitch();
  srcImg.format = (DXGI_FORMAT)ezImageFormatMappings::ToDxgiFormat(pImage->GetImageFormat());
  srcImg.pixels = pImage->GetDataPointer<ezUInt8>();

  m_pCurrentImage = make_shared<ScratchImage>();
  m_pCurrentImage->InitializeFromImage(srcImg);

  return EZ_SUCCESS;
}
