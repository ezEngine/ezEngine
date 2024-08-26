#include <Texture/TexturePCH.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX)

// BC.h poisons the preprocessor, making it impossible to include algorithm afterwards, so this has to be here.
#  include <algorithm>

#  include <Texture/DirectXTex/BC.h>

#  include <Texture/Image/ImageConversion.h>

#  include <Foundation/Threading/TaskSystem.h>

ezImageConversionEntry g_DXTexCpuConversions[] = {
  ezImageConversionEntry(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::BC6H_UF16, ezImageConversionFlags::Default),

  ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::BC1_UNORM, ezImageConversionFlags::Default, 100),
  ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::BC7_UNORM, ezImageConversionFlags::Default, 100),

  ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::BC1_UNORM_SRGB, ezImageConversionFlags::Default, 100),
  ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::BC7_UNORM_SRGB, ezImageConversionFlags::Default, 100),
};

class ezImageConversion_CompressDxTexCpu : public ezImageConversionStepCompressBlocks
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    return g_DXTexCpuConversions;
  }

  virtual ezResult CompressBlocks(ezConstByteBlobPtr source, ezByteBlobPtr target, ezUInt32 numBlocksX, ezUInt32 numBlocksY,
    ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    if (targetFormat == ezImageFormat::BC7_UNORM || targetFormat == ezImageFormat::BC7_UNORM_SRGB)
    {
      const ezUInt32 srcStride = numBlocksX * 4 * 4;
      const ezUInt32 targetStride = numBlocksX * 16;

      ezTaskSystem::ParallelForIndexed(0, numBlocksY, [srcStride, targetStride, source, target, numBlocksX](ezUInt32 startIndex, ezUInt32 endIndex)
        {
        const ezUInt8* srcIt = source.GetPtr() + srcStride * startIndex * 4;
        ezUInt8* targetIt = target.GetPtr() + targetStride * startIndex;
        for (ezUInt32 blockY = startIndex; blockY < endIndex; ++blockY)
        {
          for (ezUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
          {
            DirectX::XMVECTOR temp[16];
            for (ezUInt32 y = 0; y < 4; y++)
            {
              for (ezUInt32 x = 0; x < 4; x++)
              {
                const ezUInt8* pixel = srcIt + y * srcStride + x * 4;
                temp[y * 4 + x] = DirectX::XMVectorSet(pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, pixel[3] / 255.0f);
              }
            }
            DirectX::D3DXEncodeBC7(targetIt, temp, 0);

            srcIt += 4 * 4;
            targetIt += 16;
          }
          srcIt += 3 * srcStride;
        } });

      return EZ_SUCCESS;
    }
    else if (targetFormat == ezImageFormat::BC1_UNORM || targetFormat == ezImageFormat::BC1_UNORM_SRGB)
    {
      const ezUInt32 srcStride = numBlocksX * 4 * 4;
      const ezUInt32 targetStride = numBlocksX * 8;

      ezTaskSystem::ParallelForIndexed(0, numBlocksY, [srcStride, targetStride, source, target, numBlocksX](ezUInt32 startIndex, ezUInt32 endIndex)
        {
        const ezUInt8* srcIt = source.GetPtr() + srcStride * startIndex * 4;
        ezUInt8* targetIt = target.GetPtr() + targetStride * startIndex;
        for (ezUInt32 blockY = startIndex; blockY < endIndex; ++blockY)
        {
          for (ezUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
          {
            DirectX::XMVECTOR temp[16];
            for (ezUInt32 y = 0; y < 4; y++)
            {
              for (ezUInt32 x = 0; x < 4; x++)
              {
                const ezUInt8* pixel = srcIt + y * srcStride + x * 4;
                temp[y * 4 + x] = DirectX::XMVectorSet(pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, pixel[3] / 255.0f);
              }
            }
            DirectX::D3DXEncodeBC1(targetIt, temp, 1.0f, 0);

            srcIt += 4 * 4;
            targetIt += 8;
          }
          srcIt += 3 * srcStride;
        } });

      return EZ_SUCCESS;
    }
    else if (targetFormat == ezImageFormat::BC6H_UF16)
    {
      const ezUInt32 srcStride = numBlocksX * 4 * 4 * sizeof(float);
      const ezUInt32 targetStride = numBlocksX * 16;

      ezTaskSystem::ParallelForIndexed(0, numBlocksY, [srcStride, targetStride, source, target, numBlocksX](ezUInt32 startIndex, ezUInt32 endIndex)
        {
        const ezUInt8* srcIt = source.GetPtr() + srcStride * startIndex * 4;
        ezUInt8* targetIt = target.GetPtr() + targetStride * startIndex;
        for (ezUInt32 blockY = startIndex; blockY < endIndex; ++blockY)
        {
          for (ezUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
          {
            DirectX::XMVECTOR temp[16];
            for (ezUInt32 y = 0; y < 4; y++)
            {
              for (ezUInt32 x = 0; x < 4; x++)
              {
                const float* pixel = reinterpret_cast<const float*>(srcIt + y * srcStride + x * 4 * sizeof(float));
                temp[y * 4 + x] = DirectX::XMVectorSet(pixel[0], pixel[1], pixel[2], pixel[3]);
              }
            }
            DirectX::D3DXEncodeBC6HU(targetIt, temp, 0);

            srcIt += 4 * 4 * sizeof(float);
            targetIt += 16;
          }
          srcIt += 3 * srcStride;
        } });

      return EZ_SUCCESS;
    }

    return EZ_FAILURE;
  }
};

// EZ_STATICLINK_FORCE
static ezImageConversion_CompressDxTexCpu s_conversion_compressDxTexCpu;

#endif

EZ_STATICLINK_FILE(Texture, Texture_Image_Conversions_DXTexCpuConversions);
