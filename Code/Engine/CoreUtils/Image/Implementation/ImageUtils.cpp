#include <CoreUtils/PCH.h>
#include <CoreUtils/Image/ImageUtils.h>

template<typename TYPE>
static void SetDiff(const ezImage& ImageA, const ezImage& ImageB, ezImage& out_Difference, ezUInt32 w, ezUInt32 h, ezUInt32 d, ezUInt32 comp)
{
  const TYPE* pA = ImageA.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  const TYPE* pB = ImageB.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  TYPE* pR = out_Difference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  for (ezUInt32 i = 0; i < comp; ++i)
    pR[i] = pB[i] ^ pA[i];
}

void ezImageUtils::ComputeImageDifference(const ezImage& ImageA, const ezImage& ImageB, ezImage& out_Difference)
{
  EZ_ASSERT(ImageA.GetWidth()       == ImageB.GetWidth(),       "Dimensions do not match");
  EZ_ASSERT(ImageA.GetHeight()      == ImageB.GetHeight(),      "Dimensions do not match");
  EZ_ASSERT(ImageA.GetDepth()       == ImageB.GetDepth(),       "Dimensions do not match");
  EZ_ASSERT(ImageA.GetImageFormat() == ImageB.GetImageFormat(), "Format does not match");

  out_Difference.SetWidth(ImageA.GetWidth());
  out_Difference.SetHeight(ImageA.GetHeight());
  out_Difference.SetDepth(ImageA.GetDepth());
  out_Difference.SetImageFormat(ImageA.GetImageFormat());
  out_Difference.AllocateImageData();

  ezUInt32 uiSize2D = ImageA.GetHeight() * ImageA.GetWidth();

  for (ezUInt32 d = 0; d < ImageA.GetDepth(); ++d)
  {
    //for (ezUInt32 h = 0; h < ImageA.GetHeight(); ++h)
    {
      //for (ezUInt32 w = 0; w < ImageA.GetWidth(); ++w)
      {
        switch (ImageA.GetImageFormat())
        {
        case ezImageFormat::R8G8B8A8_UNORM:
        case ezImageFormat::R8G8B8A8_TYPELESS:
        case ezImageFormat::R8G8B8A8_UNORM_SRGB:
        case ezImageFormat::R8G8B8A8_UINT:
        case ezImageFormat::R8G8B8A8_SNORM:
        case ezImageFormat::R8G8B8A8_SINT:
        case ezImageFormat::B8G8R8A8_UNORM:
        case ezImageFormat::B8G8R8X8_UNORM:
        case ezImageFormat::B8G8R8A8_TYPELESS:
        case ezImageFormat::B8G8R8A8_UNORM_SRGB:
        case ezImageFormat::B8G8R8X8_TYPELESS:
        case ezImageFormat::B8G8R8X8_UNORM_SRGB:
          SetDiff<ezUInt32>(ImageA, ImageB, out_Difference, 0, 0, d, uiSize2D);
          break;

        case ezImageFormat::B8G8R8_UNORM:
          {
            SetDiff<ezUInt8>(ImageA, ImageB, out_Difference, 0, 0, d, 3 * uiSize2D);
          }
          break;

        default:
          EZ_REPORT_FAILURE("The ezImageFormat %u is not implemented", (ezUInt32) ImageA.GetImageFormat());
          return;
        }
      }
    }
  }

}

