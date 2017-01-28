#include "Main.h"
#include <Foundation/Image/ImageUtils.h>

namespace
{
  void CopyFace(ezImage& dstImg, const ezImage& srcImg, ezUInt32 offsetX, ezUInt32 offsetY, ezUInt32 faceIndex)
  {
    ezUInt32 faceRowPitch = dstImg.GetRowPitch();
    ezUInt32 srcRowPitch = srcImg.GetRowPitch();
    EZ_ASSERT_DEV(faceRowPitch > 0 && srcRowPitch > 0, "Can not deal with compressed images. Image should already be decompressed at this point");

    ezUInt8* dstFace = dstImg.GetSubImagePointer<ezUInt8>(0, faceIndex);
    const ezUInt8* srcPtr = srcImg.GetPixelPointer<ezUInt8>(0, 0, 0, offsetX, offsetY);
    const ezUInt32 faceSize = dstImg.GetWidth();
    for (ezUInt32 y = 0; y < faceSize; y++)
    {
      ezMemoryUtils::Copy(dstFace, srcPtr, faceRowPitch);
      dstFace += faceRowPitch;
      srcPtr += srcRowPitch;
    }
  }
}

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
  ezImage& img = m_InputImages[0];

  if (img.GetNumFaces() != 6)
  {
    if (img.GetWidth() % 3 == 0 && img.GetHeight() % 4 == 0 && img.GetWidth() / 3 == img.GetHeight() / 4)
    {
      // Vertical cube map layout
      //     +---+
      //     | Y+|
      // +---+---+---+
      // | X-| Z+| X+|
      // +---+---+---+
      //     | Y-|
      //     +---+
      //     | Z-|
      //     +---+
      ezUInt32 faceSize = img.GetWidth() / 3;

      ezImage tmpImg;

      tmpImg.SetWidth(faceSize);
      tmpImg.SetHeight(faceSize);
      tmpImg.SetImageFormat(img.GetImageFormat());
      tmpImg.SetDepth(1);
      tmpImg.SetNumFaces(6);
      tmpImg.SetNumMipLevels(1);
      tmpImg.SetNumArrayIndices(1);

      tmpImg.AllocateImageData();

      // face order in dds files is: positive x, negative x, positive y, negative y, positive z, negative z

      // Positive X face
      CopyFace(tmpImg, img, faceSize * 2, faceSize, 0);

      // Negative X face
      CopyFace(tmpImg, img, 0, faceSize, 1);

      // Positive Y face
      CopyFace(tmpImg, img, faceSize, 0, 2);

      // Negative Y face
      CopyFace(tmpImg, img, faceSize, faceSize * 2, 3);

      // Positive Z face
      CopyFace(tmpImg, img, faceSize, faceSize, 4);

      // Negative Z face
      CopyFace(tmpImg, img, faceSize, faceSize * 3, 5);
      ezImageUtils::RotateSubImage180(tmpImg, 0, 5);

      img.Swap(tmpImg);
    }
    else if (img.GetWidth() % 4 == 0 && img.GetHeight() % 3 == 0 && img.GetWidth() / 4 == img.GetHeight() / 3)
    {
      // Horizontal cube map layout
      //     +---+
      //     | Y+|
      // +---+---+---+---+
      // | X-| Z+| X+| Z-|
      // +---+---+---+---+
      //     | Y-|
      //     +---+
      ezUInt32 faceSize = img.GetWidth() / 4;

      ezImage tmpImg;

      tmpImg.SetWidth(faceSize);
      tmpImg.SetHeight(faceSize);
      tmpImg.SetImageFormat(img.GetImageFormat());
      tmpImg.SetDepth(1);
      tmpImg.SetNumFaces(6);
      tmpImg.SetNumMipLevels(1);
      tmpImg.SetNumArrayIndices(1);

      tmpImg.AllocateImageData();

      // face order in dds files is: positive x, negative x, positive y, negative y, positive z, negative z

      // Positive X face
      CopyFace(tmpImg, img, faceSize * 2, faceSize, 0);

      // Negative X face
      CopyFace(tmpImg, img, 0, faceSize, 1);

      // Positive Y face
      CopyFace(tmpImg, img, faceSize, 0, 2);

      // Negative Y face
      CopyFace(tmpImg, img, faceSize, faceSize * 2, 3);

      // Positive Z face
      CopyFace(tmpImg, img, faceSize, faceSize, 4);

      // Negative Z face
      CopyFace(tmpImg, img, faceSize * 3, faceSize, 5);

      img.Swap(tmpImg);
    }
    else
    {
      // Spherical mapping
      if (img.GetWidth() % 4 != 0)
      {
        ezLog::Error("Width of the input image should be a multiple of 4");
        return EZ_FAILURE;
      }

      ezUInt32 faceSize = img.GetWidth() / 4;

      ezImage tmpImg;

      tmpImg.SetWidth(faceSize);
      tmpImg.SetHeight(faceSize);
      tmpImg.SetImageFormat(img.GetImageFormat());
      tmpImg.SetDepth(1);
      tmpImg.SetNumFaces(6);
      tmpImg.SetNumMipLevels(1);
      tmpImg.SetNumArrayIndices(1);

      tmpImg.AllocateImageData();

      // Corners of the UV space for the respective faces in model space
      ezVec3 faceCorners[] =
      {
        ezVec3( 0.5,  0.5,  0.5), // X+
        ezVec3(-0.5,  0.5, -0.5), // X-
        ezVec3(-0.5,  0.5, -0.5), // Y+
        ezVec3(-0.5, -0.5,  0.5), // Y-
        ezVec3(-0.5,  0.5,  0.5), // Z+
        ezVec3( 0.5,  0.5, -0.5)  // Z-
      };

      // UV Axis of the respective faces in model space
      ezVec3 faceAxis[] =
      {
        ezVec3( 0,  0, -1), ezVec3(0, -1,  0), // X+
        ezVec3( 0,  0,  1), ezVec3(0, -1,  0), // X-
        ezVec3( 1,  0,  0), ezVec3(0,  0,  1), // Y+
        ezVec3( 1,  0,  0), ezVec3(0,  0, -1), // Y-
        ezVec3( 1,  0,  0), ezVec3(0, -1,  0), // Z+
        ezVec3(-1,  0,  0), ezVec3(0, -1,  0)  // Z-
      };

      float fFaceSize = (float)faceSize;
      float fHalfPixel = 0.5f / fFaceSize;
      float fPixel = 1.0f / fFaceSize;

      float fHalfSrcWidth = img.GetWidth() / 2.0f;
      float fSrcHeight = (float)img.GetHeight();

      ezUInt32 srcWidthMinus1 = img.GetWidth() - 1;
      ezUInt32 srcHeightMinus1 = img.GetHeight() - 1;
      EZ_ASSERT_DEV(img.GetRowPitch() % sizeof(ezColor) == 0, "Row pitch should be a multiple of sizeof(ezColor)");
      const ezUInt32 srcRowPitch = img.GetRowPitch() / sizeof(ezColor);
      EZ_ASSERT_DEV(tmpImg.GetRowPitch() % sizeof(ezColor) == 0, "Row pitch should be a multiple of sizeof(ezColor)");
      const ezUInt32 faceRowPitch = tmpImg.GetRowPitch() / sizeof(ezColor);
      const ezColor* srcData = img.GetSubImagePointer<ezColor>();
      const float InvPi = 1.0f / ezMath::BasicType<float>::Pi();
      for (ezUInt32 faceIndex = 0; faceIndex < 6; faceIndex++)
      {
        ezColor* faceData = tmpImg.GetSubImagePointer<ezColor>(0, faceIndex);
        for (ezUInt32 y = 0; y < faceSize; y++)
        {
          float dstV = (float)y * fPixel + fHalfPixel;
          for (ezUInt32 x = 0; x < faceSize; x++)
          {
            float dstU = (float)x * fPixel + fHalfPixel;
            ezVec3 modelSpacePos = faceCorners[faceIndex] + dstU * faceAxis[faceIndex * 2] + dstV * faceAxis[faceIndex * 2 + 1];
            ezVec3 modelSpaceDir = modelSpacePos.GetNormalized();

            float phi = ezMath::ATan2(modelSpaceDir.x, modelSpaceDir.z).GetRadian() + ezMath::BasicType<float>::Pi();
            float r = ezMath::Sqrt(modelSpaceDir.x * modelSpaceDir.x + modelSpaceDir.z * modelSpaceDir.z);
            float theta = ezMath::ATan2(modelSpaceDir.y, r).GetRadian() + ezMath::BasicType<float>::Pi() * 0.5f;

            EZ_ASSERT_DEBUG(phi >= 0.0f && phi <= 2.0f * ezMath::BasicType<float>::Pi(), "");
            EZ_ASSERT_DEBUG(theta >= 0.0f && theta <= ezMath::BasicType<float>::Pi(), "");

            float srcU = phi * InvPi * fHalfSrcWidth;
            float srcV = (1.0f - theta * InvPi) * fSrcHeight;

            ezUInt32 x1 = (ezUInt32)ezMath::Floor(srcU);
            ezUInt32 x2 = x1 + 1;
            ezUInt32 y1 = (ezUInt32)ezMath::Floor(srcV);
            ezUInt32 y2 = y1 + 1;

            float fracX = srcU - x1;
            float fracY = srcV - y1;

            x1 = ezMath::Clamp(x1, 0u, srcWidthMinus1);
            x2 = ezMath::Clamp(x2, 0u, srcWidthMinus1);
            y1 = ezMath::Clamp(y1, 0u, srcHeightMinus1);
            y2 = ezMath::Clamp(y2, 0u, srcHeightMinus1);

            ezColor A = srcData[x1 + y1 * srcRowPitch];
            ezColor B = srcData[x2 + y1 * srcRowPitch];
            ezColor C = srcData[x1 + y2 * srcRowPitch];
            ezColor D = srcData[x2 + y2 * srcRowPitch];

            ezColor interpolated = A * (1 - fracX)*(1 - fracY) + B * (fracX)*(1 - fracY) + C * (1 - fracX)*fracY + D * fracX*fracY;
            faceData[x + y * faceRowPitch] = interpolated;
          }
        }
      }

      img.Swap(tmpImg);
    }
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

