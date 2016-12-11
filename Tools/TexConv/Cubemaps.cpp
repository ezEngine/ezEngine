#include "Main.h"

ezResult ezTexConv::CreateTextureCube()
{
  if (m_InputImages.GetCount() == 1)
  {
    return CreateTextureCubeFromSingleFile();
  }

  if (m_InputImages.GetCount() == 6)
  {
    return CreateTextureCubeFrom6Files();
  }

  ezLog::Error("Invalid number of inputs ({0}) to create a cubemap", m_InputImages.GetCount());
  return EZ_FAILURE;
}


ezResult ezTexConv::CreateTextureCubeFromSingleFile()
{
  const ezImage& img = m_InputImages[0];

  if (img.GetNumFaces() != 6)
  {
    SetReturnCode(TexConvReturnCodes::BAD_SINGLE_CUBEMAP_FILE);
    ezLog::Error("The single input file is not a cubemap");
    return EZ_FAILURE;
  }

  if (m_uiOutputChannels == 4 && m_bCompress)
  {
    m_bAlphaIsMaskOnly = IsImageAlphaBinaryMask(img);
  }

  Image srcImg[6];

  // According to the DX spec
  // https://msdn.microsoft.com/en-us/library/windows/desktop/bb204881%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
  // the order should be:
  //
  // 0 = +X = Right
  // 1 = -X = Left
  // 2 = +Y = Top
  // 3 = -Y = Bottom
  // 4 = +Z = Front
  // 5 = -Z = Back

  for (int i = 0; i < 6; ++i)
  {
    srcImg[i].width = img.GetWidth();
    srcImg[i].height = img.GetHeight();
    srcImg[i].rowPitch = img.GetRowPitch();
    srcImg[i].slicePitch = img.GetDepthPitch();
    srcImg[i].format = (DXGI_FORMAT)ezImageFormatMappings::ToDxgiFormat(img.GetImageFormat());
    srcImg[i].pixels = const_cast<ezUInt8*>(img.GetPixelPointer<ezUInt8>(0, i));
  }

  m_pCurrentImage = make_shared<ScratchImage>();
  if (FAILED(m_pCurrentImage->InitializeCubeFromImages(srcImg, 6)))
  {
    SetReturnCode(TexConvReturnCodes::FAILED_INITIALIZE_CUBEMAP);
    ezLog::Error("Failed to create a cubemap from the given input file");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv::CreateTextureCubeFrom6Files()
{
  if (m_uiOutputChannels == 4 && m_bCompress)
  {
    m_bAlphaIsMaskOnly = true;

    for (ezUInt32 i = 0; i < 6; ++i)
    {
      if (!IsImageAlphaBinaryMask(m_InputImages[i]))
      {
        m_bAlphaIsMaskOnly = false;
        break;
      }
    }
  }

  Image srcImg[6];

  for (int i = 0; i < 6; ++i)
  {
    const ezImage& img = m_InputImages[i];

    srcImg[i].width = img.GetWidth();
    srcImg[i].height = img.GetHeight();
    srcImg[i].rowPitch = img.GetRowPitch();
    srcImg[i].slicePitch = img.GetDepthPitch();
    srcImg[i].format = (DXGI_FORMAT)ezImageFormatMappings::ToDxgiFormat(img.GetImageFormat());
    srcImg[i].pixels = const_cast<ezUInt8*>(img.GetPixelPointer<ezUInt8>(0, 0));
  }

  m_pCurrentImage = make_shared<ScratchImage>();
  if (FAILED(m_pCurrentImage->InitializeCubeFromImages(srcImg, 6)))
  {
    SetReturnCode(TexConvReturnCodes::FAILED_COMBINE_CUBEMAP);
    ezLog::Error("Failed to combine 6 input files into one cubemap");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

