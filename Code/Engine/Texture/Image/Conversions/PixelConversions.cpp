#include <PCH.h>

#include <Texture/Image/Conversions/PixelConversions.h>
#include <Texture/Image/ImageConversion.h>
#include <Foundation/Math/Float16.h>

#if EZ_SSE_LEVEL >= EZ_SSE_20
#  include <emmintrin.h>
#endif

#if EZ_SSE_LEVEL >= EZ_SSE_30
#  include <tmmintrin.h>
#endif

namespace
{
  // 3D vector: 11/11/10 floating-point components
  // The 3D vector is packed into 32 bits as follows: a 5-bit biased exponent
  // and 6-bit mantissa for x component, a 5-bit biased exponent and
  // 6-bit mantissa for y component, a 5-bit biased exponent and a 5-bit
  // mantissa for z. The z component is stored in the most significant bits
  // and the x component in the least significant bits. No sign bits so
  // all partial-precision numbers are positive.
  // (Z10Y11X11): [32] ZZZZZzzz zzzYYYYY yyyyyyXX XXXxxxxx [0
  union R11G11B10 {
    struct Parts
    {
      ezUInt32 xm : 6; // x-mantissa
      ezUInt32 xe : 5; // x-exponent
      ezUInt32 ym : 6; // y-mantissa
      ezUInt32 ye : 5; // y-exponent
      ezUInt32 zm : 5; // z-mantissa
      ezUInt32 ze : 5; // z-exponent
    } p;
    ezUInt32 v;
  };
} // namespace

ezColorBaseUB ezDecompressA4B4G4R4(ezUInt16 uiColor)
{
  ezColorBaseUB result;
  result.r = ((uiColor & 0xF000u) * 17) >> 12;
  result.g = ((uiColor & 0x0F00u) * 17) >> 8;
  result.b = ((uiColor & 0x00F0u) * 17) >> 4;
  result.a = ((uiColor & 0x000Fu) * 17);
  return result;
}

ezUInt16 ezCompressA4B4G4R4(ezColorBaseUB color)
{
  ezUInt32 r = (color.r * 15 + 135) >> 8;
  ezUInt32 g = (color.g * 15 + 135) >> 8;
  ezUInt32 b = (color.b * 15 + 135) >> 8;
  ezUInt32 a = (color.a * 15 + 135) >> 8;
  return (r << 12) | (g << 8) | (b << 4) | a;
}

ezColorBaseUB ezDecompressB4G4R4A4(ezUInt16 uiColor)
{
  ezColorBaseUB result;
  result.r = ((uiColor & 0x0F00u) * 17) >> 8;
  result.g = ((uiColor & 0x00F0u) * 17) >> 4;
  result.b = ((uiColor & 0x000Fu) * 17);
  result.a = ((uiColor & 0xF000u) * 17) >> 12;
  return result;
}

ezUInt16 ezCompressB4G4R4A4(ezColorBaseUB color)
{
  ezUInt32 r = (color.r * 15 + 135) >> 8;
  ezUInt32 g = (color.g * 15 + 135) >> 8;
  ezUInt32 b = (color.b * 15 + 135) >> 8;
  ezUInt32 a = (color.a * 15 + 135) >> 8;
  return (a << 12) | (r << 8) | (g << 4) | b;
}

ezColorBaseUB ezDecompressB5G6R5(ezUInt16 uiColor)
{
  ezColorBaseUB result;
  result.r = ((uiColor & 0xF800u) * 527 + 47104) >> 17;
  result.g = ((uiColor & 0x07E0u) * 259 + 1056) >> 11;
  result.b = ((uiColor & 0x001Fu) * 527 + 23) >> 6;
  result.a = 0xFF;

  return result;
}

ezUInt16 ezCompressB5G6R5(ezColorBaseUB color)
{
  ezUInt32 r = (color.r * 249 + 1024) >> 11;
  ezUInt32 g = (color.g * 253 + 512) >> 10;
  ezUInt32 b = (color.b * 249 + 1024) >> 11;
  return (r << 11) | (g << 5) | b;
}

ezColorBaseUB ezDecompressB5G5R5X1(ezUInt16 uiColor)
{
  ezColorBaseUB result;
  result.r = ((uiColor & 0x7C00u) * 527 + 23552) >> 16;
  result.g = ((uiColor & 0x03E0u) * 527 + 736) >> 11;
  result.b = ((uiColor & 0x001Fu) * 527 + 23) >> 6;
  result.a = 0xFF;
  return result;
}

ezUInt16 ezCompressB5G5R5X1(ezColorBaseUB color)
{
  ezUInt32 r = (color.r * 249 + 1024) >> 11;
  ezUInt32 g = (color.g * 249 + 1024) >> 11;
  ezUInt32 b = (color.b * 249 + 1024) >> 11;
  return (1 << 15) | (r << 10) | (g << 5) | b;
}

ezColorBaseUB ezDecompressB5G5R5A1(ezUInt16 uiColor)
{
  ezColorBaseUB result;
  result.r = ((uiColor & 0x7C00u) * 527 + 23552) >> 16;
  result.g = ((uiColor & 0x03E0u) * 527 + 736) >> 11;
  result.b = ((uiColor & 0x001Fu) * 527 + 23) >> 6;
  result.a = ((uiColor & 0x8000u) * 255) >> 15;
  return result;
}

ezUInt16 ezCompressB5G5R5A1(ezColorBaseUB color)
{
  ezUInt32 r = (color.r * 249 + 1024) >> 11;
  ezUInt32 g = (color.g * 249 + 1024) >> 11;
  ezUInt32 b = (color.b * 249 + 1024) >> 11;
  ezUInt32 a = (color.a) >> 7;
  return (a << 15) | (r << 10) | (g << 5) | b;
}

ezColorBaseUB ezDecompressX1B5G5R5(ezUInt16 uiColor)
{
  ezColorBaseUB result;
  result.r = ((uiColor & 0xF800u) * 527 + 23552) >> 17;
  result.g = ((uiColor & 0x07C0u) * 527 + 736) >> 12;
  result.b = ((uiColor & 0x003Eu) * 527 + 23) >> 7;
  result.a = 0xFF;
  return result;
}

ezUInt16 ezCompressX1B5G5R5(ezColorBaseUB color)
{
  ezUInt32 r = (color.r * 249 + 1024) >> 11;
  ezUInt32 g = (color.g * 249 + 1024) >> 11;
  ezUInt32 b = (color.b * 249 + 1024) >> 11;
  return (r << 11) | (g << 6) | (b << 1) | 1;
}

ezColorBaseUB ezDecompressA1B5G5R5(ezUInt16 uiColor)
{
  ezColorBaseUB result;
  result.r = ((uiColor & 0xF800u) * 527 + 23552) >> 17;
  result.g = ((uiColor & 0x07C0u) * 527 + 736) >> 12;
  result.b = ((uiColor & 0x003Eu) * 527 + 23) >> 7;
  result.a = (uiColor & 0x0001u) * 255;
  return result;
}

ezUInt16 ezCompressA1B5G5R5(ezColorBaseUB color)
{
  ezUInt32 r = (color.r * 249 + 1024) >> 11;
  ezUInt32 g = (color.g * 249 + 1024) >> 11;
  ezUInt32 b = (color.b * 249 + 1024) >> 11;
  ezUInt32 a = color.a >> 7;
  return (r << 11) | (g << 6) | (b << 1) | a;
}

template <ezColorBaseUB (*decompressFunc)(ezUInt16), ezImageFormat::Enum templateSourceFormat>
class ezImageConversionStep_Decompress16bpp : ezImageConversionStepLinear
{
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    ezImageFormat::Enum sourceFormatSrgb = ezImageFormat::AsSrgb(templateSourceFormat);
    EZ_ASSERT_DEV(sourceFormatSrgb != templateSourceFormat, "Format '%s' should have a corresponding sRGB format",
                  ezImageFormat::GetName(templateSourceFormat));

    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(templateSourceFormat, ezImageFormat::R8G8B8A8_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(sourceFormatSrgb, ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageConversionFlags::Default),
    };

    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    ezUInt32 sourceStride = 2;
    ezUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      *reinterpret_cast<ezColorBaseUB*>(targetPointer) = decompressFunc(*reinterpret_cast<const ezUInt16*>(sourcePointer));

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

template <ezUInt16 (*compressFunc)(ezColorBaseUB), ezImageFormat::Enum templateTargetFormat>
class ezImageConversionStep_Compress16bpp : ezImageConversionStepLinear
{
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    ezImageFormat::Enum targetFormatSrgb = ezImageFormat::AsSrgb(templateTargetFormat);
    EZ_ASSERT_DEV(targetFormatSrgb != templateTargetFormat, "Format '%s' should have a corresponding sRGB format",
                  ezImageFormat::GetName(templateTargetFormat));

    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, templateTargetFormat, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, targetFormatSrgb, ezImageConversionFlags::Default),
    };

    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    ezUInt32 sourceStride = 4;
    ezUInt32 targetStride = 2;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      *reinterpret_cast<ezUInt16*>(targetPointer) = compressFunc(*reinterpret_cast<const ezColorBaseUB*>(sourcePointer));

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

static bool IsAligned(const void* pointer)
{
  return reinterpret_cast<size_t>(pointer) % 16 == 0;
}



struct ezImageSwizzleConversion32_2103 : public ezImageConversionStepLinear
{
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::B8G8R8A8_UNORM, ezImageFormat::R8G8B8A8_UNORM, ezImageConversionFlags::InPlace),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::B8G8R8A8_UNORM, ezImageConversionFlags::InPlace),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::B8G8R8X8_UNORM, ezImageConversionFlags::InPlace),
        ezImageConversionEntry(ezImageFormat::B8G8R8A8_UNORM_SRGB, ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageConversionFlags::InPlace),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::B8G8R8A8_UNORM_SRGB, ezImageConversionFlags::InPlace),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::B8G8R8X8_UNORM_SRGB, ezImageConversionFlags::InPlace),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    ezUInt32 sourceStride = 4;
    ezUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    if (IsAligned(sourcePointer) && IsAligned(targetPointer))
    {
#if EZ_SSE_LEVEL >= EZ_SSE_30
      const ezUInt32 elementsPerBatch = 8;

      __m128i shuffleMask = _mm_set_epi8(15, 12, 13, 14, 11, 8, 9, 10, 7, 4, 5, 6, 3, 0, 1, 2);

      // Intel optimization manual, Color Pixel Format Conversion Using SSE3
      while (numElements >= elementsPerBatch)
      {
        __m128i in0 = reinterpret_cast<const __m128i*>(sourcePointer)[0];
        __m128i in1 = reinterpret_cast<const __m128i*>(sourcePointer)[1];

        reinterpret_cast<__m128i*>(targetPointer)[0] = _mm_shuffle_epi8(in0, shuffleMask);
        reinterpret_cast<__m128i*>(targetPointer)[1] = _mm_shuffle_epi8(in1, shuffleMask);

        sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        numElements -= elementsPerBatch;
      }
#else
      const ezUInt32 elementsPerBatch = 8;

      __m128i mask1 = _mm_set1_epi32(0xff00ff00);
      __m128i mask2 = _mm_set1_epi32(0x00ff00ff);

      // Intel optimization manual, Color Pixel Format Conversion Using SSE2
      while (numElements >= elementsPerBatch)
      {
        __m128i in0 = reinterpret_cast<const __m128i*>(sourcePointer)[0];
        __m128i in1 = reinterpret_cast<const __m128i*>(sourcePointer)[1];

        reinterpret_cast<__m128i*>(targetPointer)[0] =
            _mm_or_si128(_mm_and_si128(in0, mask1), _mm_and_si128(_mm_or_si128(_mm_slli_epi32(in0, 16), _mm_srli_epi32(in0, 16)), mask2));
        reinterpret_cast<__m128i*>(targetPointer)[1] =
            _mm_or_si128(_mm_and_si128(in1, mask1), _mm_and_si128(_mm_or_si128(_mm_slli_epi32(in1, 16), _mm_srli_epi32(in1, 16)), mask2));

        sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        numElements -= elementsPerBatch;
      }
#endif
    }

    while (numElements)
    {
      ezUInt8 a, b, c, d;
      a = reinterpret_cast<const ezUInt8*>(sourcePointer)[2];
      b = reinterpret_cast<const ezUInt8*>(sourcePointer)[1];
      c = reinterpret_cast<const ezUInt8*>(sourcePointer)[0];
      d = reinterpret_cast<const ezUInt8*>(sourcePointer)[3];
      reinterpret_cast<ezUInt8*>(targetPointer)[0] = a;
      reinterpret_cast<ezUInt8*>(targetPointer)[1] = b;
      reinterpret_cast<ezUInt8*>(targetPointer)[2] = c;
      reinterpret_cast<ezUInt8*>(targetPointer)[3] = d;

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

struct ezImageConversion_BGRX_BGRA : public ezImageConversionStepLinear
{
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        {ezImageFormat::B8G8R8X8_UNORM, ezImageFormat::B8G8R8A8_UNORM, ezImageConversionFlags::InPlace},
        {ezImageFormat::B8G8R8X8_UNORM_SRGB, ezImageFormat::B8G8R8A8_UNORM_SRGB, ezImageConversionFlags::InPlace},
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    ezUInt32 sourceStride = 4;
    ezUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

#if EZ_SSE_LEVEL >= EZ_SSE_20
    if (IsAligned(sourcePointer) && IsAligned(targetPointer))
    {
      const ezUInt32 elementsPerBatch = 4;

      __m128i mask = _mm_set1_epi32(0xFF000000);

      while (numElements >= elementsPerBatch)
      {
        const __m128i* pSource = reinterpret_cast<const __m128i*>(sourcePointer);
        __m128i* pTarget = reinterpret_cast<__m128i*>(targetPointer);

        pTarget[0] = _mm_or_si128(pSource[0], mask);

        sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        numElements -= elementsPerBatch;
      }
    }
#endif

    while (numElements)
    {
      ezUInt32 x = *(reinterpret_cast<const ezUInt32*>(sourcePointer));

#if EZ_ENABLED(EZ_PLATFORM_LITTLE_ENDIAN)
      x |= 0xFF000000;
#else
      x |= 0x000000FF;
#endif

      *(reinterpret_cast<ezUInt32*>(targetPointer)) = x;

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

class ezImageConversion_F32_U8 : public ezImageConversionStepLinear
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R32_FLOAT, ezImageFormat::R8_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32_FLOAT, ezImageFormat::R8G8_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32_FLOAT, ezImageFormat::R8G8B8_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::R8G8B8A8_UNORM, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    numElements *= ezImageFormat::GetBitsPerPixel(targetFormat) / 8;

    ezUInt32 sourceStride = 4;
    ezUInt32 targetStride = 1;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

#if EZ_SSE_LEVEL >= EZ_SSE_20
    {
      const ezUInt32 elementsPerBatch = 16;

      __m128 zero = _mm_setzero_ps();
      __m128 one = _mm_set1_ps(1.0f);
      __m128 scale = _mm_set1_ps(255.0f);
      __m128 half = _mm_set1_ps(0.5f);

      while (numElements >= elementsPerBatch)
      {
        __m128 float0 = _mm_loadu_ps(static_cast<const float*>(sourcePointer) + 0);
        __m128 float1 = _mm_loadu_ps(static_cast<const float*>(sourcePointer) + 4);
        __m128 float2 = _mm_loadu_ps(static_cast<const float*>(sourcePointer) + 8);
        __m128 float3 = _mm_loadu_ps(static_cast<const float*>(sourcePointer) + 12);

        // Clamp NaN to zero
        float0 = _mm_and_ps(_mm_cmpord_ps(float0, zero), float0);
        float1 = _mm_and_ps(_mm_cmpord_ps(float1, zero), float1);
        float2 = _mm_and_ps(_mm_cmpord_ps(float2, zero), float2);
        float3 = _mm_and_ps(_mm_cmpord_ps(float3, zero), float3);

        // Saturate
        float0 = _mm_max_ps(zero, _mm_min_ps(one, float0));
        float1 = _mm_max_ps(zero, _mm_min_ps(one, float1));
        float2 = _mm_max_ps(zero, _mm_min_ps(one, float2));
        float3 = _mm_max_ps(zero, _mm_min_ps(one, float3));

        float0 = _mm_mul_ps(float0, scale);
        float1 = _mm_mul_ps(float1, scale);
        float2 = _mm_mul_ps(float2, scale);
        float3 = _mm_mul_ps(float3, scale);

        // Add 0.5f and truncate for rounding as required by D3D spec
        float0 = _mm_add_ps(float0, half);
        float1 = _mm_add_ps(float1, half);
        float2 = _mm_add_ps(float2, half);
        float3 = _mm_add_ps(float3, half);

        __m128i int0 = _mm_cvttps_epi32(float0);
        __m128i int1 = _mm_cvttps_epi32(float1);
        __m128i int2 = _mm_cvttps_epi32(float2);
        __m128i int3 = _mm_cvttps_epi32(float3);

        __m128i short0 = _mm_packs_epi32(int0, int1);
        __m128i short1 = _mm_packs_epi32(int2, int3);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(targetPointer), _mm_packus_epi16(short0, short1));

        sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        numElements -= elementsPerBatch;
      }
    }
#endif

    while (numElements)
    {

      *reinterpret_cast<ezUInt8*>(targetPointer) = ezMath::ColorFloatToByte(*reinterpret_cast<const float*>(sourcePointer));

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

class ezImageConversion_F32_sRGB : public ezImageConversionStepLinear
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    ezUInt32 sourceStride = 16;
    ezUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      *reinterpret_cast<ezColorGammaUB*>(targetPointer) = *reinterpret_cast<const ezColor*>(sourcePointer);

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

class ezImageConversion_F32_U16 : public ezImageConversionStepLinear
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R32_FLOAT, ezImageFormat::R16_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32_FLOAT, ezImageFormat::R16G16_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32_FLOAT, ezImageFormat::R16G16B16_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::R16G16B16A16_UNORM, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    numElements *= ezImageFormat::GetBitsPerPixel(targetFormat) / 16;

    ezUInt32 sourceStride = 4;
    ezUInt32 targetStride = 2;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {

      *reinterpret_cast<ezUInt16*>(targetPointer) = ezMath::ColorFloatToShort(*reinterpret_cast<const float*>(sourcePointer));

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

class ezImageConversion_F32_F16 : public ezImageConversionStepLinear
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R32_FLOAT, ezImageFormat::R16_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32_FLOAT, ezImageFormat::R16G16_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::R16G16B16A16_FLOAT, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    numElements *= ezImageFormat::GetBitsPerPixel(targetFormat) / 16;

    ezUInt32 sourceStride = 4;
    ezUInt32 targetStride = 2;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {

      *reinterpret_cast<ezFloat16*>(targetPointer) = *reinterpret_cast<const float*>(sourcePointer);

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

class ezImageConversion_F32_S8 : public ezImageConversionStepLinear
{
public:
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R32_FLOAT, ezImageFormat::R8_SNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32_FLOAT, ezImageFormat::R8G8_SNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::R8G8B8A8_SNORM, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    numElements *= ezImageFormat::GetBitsPerPixel(targetFormat) / 8;

    ezUInt32 sourceStride = 4;
    ezUInt32 targetStride = 1;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {

      *reinterpret_cast<ezInt8*>(targetPointer) = ezMath::ColorFloatToSignedByte(*reinterpret_cast<const float*>(sourcePointer));

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

class ezImageConversion_U8_F32 : public ezImageConversionStepLinear
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R8_UNORM, ezImageFormat::R32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8_UNORM, ezImageFormat::R32G32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8_UNORM, ezImageFormat::R32G32B32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::R32G32B32A32_FLOAT, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    numElements *= ezImageFormat::GetBitsPerPixel(targetFormat) / 32;

    ezUInt32 sourceStride = 1;
    ezUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      *reinterpret_cast<float*>(targetPointer) = ezMath::ColorByteToFloat(*reinterpret_cast<const ezUInt8*>(sourcePointer));

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

class ezImageConversion_sRGB_F32 : public ezImageConversionStepLinear
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::R32G32B32A32_FLOAT, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    ezUInt32 sourceStride = 4;
    ezUInt32 targetStride = 16;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      *reinterpret_cast<ezColor*>(targetPointer) = *reinterpret_cast<const ezColorGammaUB*>(sourcePointer);

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

class ezImageConversion_U16_F32 : public ezImageConversionStepLinear
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R16_UNORM, ezImageFormat::R32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16_UNORM, ezImageFormat::R32G32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16B16_UNORM, ezImageFormat::R32G32B32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16B16A16_UNORM, ezImageFormat::R32G32B32A32_FLOAT, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    numElements *= ezImageFormat::GetBitsPerPixel(targetFormat) / 32;

    ezUInt32 sourceStride = 2;
    ezUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      *reinterpret_cast<float*>(targetPointer) = ezMath::ColorShortToFloat(*reinterpret_cast<const ezUInt16*>(sourcePointer));

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

class ezImageConversion_F16_F32 : public ezImageConversionStepLinear
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R16_FLOAT, ezImageFormat::R32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16_FLOAT, ezImageFormat::R32G32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16B16A16_FLOAT, ezImageFormat::R32G32B32A32_FLOAT, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    numElements *= ezImageFormat::GetBitsPerPixel(targetFormat) / 32;

    ezUInt32 sourceStride = 2;
    ezUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      *reinterpret_cast<float*>(targetPointer) = *reinterpret_cast<const ezFloat16*>(sourcePointer);

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

class ezImageConversion_S8_F32 : public ezImageConversionStepLinear
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R8_SNORM, ezImageFormat::R32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8_SNORM, ezImageFormat::R32G32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_SNORM, ezImageFormat::R32G32B32A32_FLOAT, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    numElements *= ezImageFormat::GetBitsPerPixel(targetFormat) / 32;

    ezUInt32 sourceStride = 1;
    ezUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      *reinterpret_cast<float*>(targetPointer) = ezMath::ColorSignedByteToFloat(*reinterpret_cast<const ezInt8*>(sourcePointer));

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

struct ezImageConversion_Pad_To_RGBA_U8 : public ezImageConversionStepLinear
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R8_UNORM, ezImageFormat::R8G8B8A8_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8_UNORM, ezImageFormat::R8G8B8A8_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8_UNORM, ezImageFormat::R8G8B8A8_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8_UNORM_SRGB, ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::B8G8R8_UNORM, ezImageFormat::B8G8R8A8_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::B8G8R8_UNORM_SRGB, ezImageFormat::B8G8R8A8_UNORM_SRGB, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    ezUInt32 sourceStride = ezImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    ezUInt32 targetStride = ezImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const ezUInt8* sourcePointer = static_cast<const ezUInt8*>(source.GetPtr());
    ezUInt8* targetPointer = static_cast<ezUInt8*>(target.GetPtr());

    const ezUInt32 numChannels = sourceStride / sizeof(ezUInt8);

#if EZ_ENABLED(EZ_PLATFORM_LITTLE_ENDIAN)
    if (numChannels == 3)
    {
      // Fast path for RGB -> RGBA
      const ezUInt32 elementsPerBatch = 4;

      while (numElements >= elementsPerBatch)
      {
        ezUInt32 source0 = reinterpret_cast<const ezUInt32*>(sourcePointer)[0];
        ezUInt32 source1 = reinterpret_cast<const ezUInt32*>(sourcePointer)[1];
        ezUInt32 source2 = reinterpret_cast<const ezUInt32*>(sourcePointer)[2];

        ezUInt32 target0 = source0 | 0xFF000000;
        ezUInt32 target1 = (source0 >> 24) | (source1 << 8) | 0xFF000000;
        ezUInt32 target2 = (source1 >> 16) | (source2 << 16) | 0xFF000000;
        ezUInt32 target3 = (source2 >> 8) | 0xFF000000;

        reinterpret_cast<ezUInt32*>(targetPointer)[0] = target0;
        reinterpret_cast<ezUInt32*>(targetPointer)[1] = target1;
        reinterpret_cast<ezUInt32*>(targetPointer)[2] = target2;
        reinterpret_cast<ezUInt32*>(targetPointer)[3] = target3;

        sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        numElements -= elementsPerBatch;
      }
    }
#endif


    while (numElements)
    {
      // Copy existing channels
      memcpy(targetPointer, sourcePointer, numChannels);

      // Fill others with zero
      memset(targetPointer + numChannels, 0, 3 * sizeof(ezUInt8) - numChannels);

      // Set alpha to 1
      targetPointer[3] = 0xFF;

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

struct ezImageConversion_Pad_To_RGBA_F32 : public ezImageConversionStepLinear
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R32_FLOAT, ezImageFormat::R32G32B32A32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32_FLOAT, ezImageFormat::R32G32B32A32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32_FLOAT, ezImageFormat::R32G32B32A32_FLOAT, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    ezUInt32 sourceStride = ezImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    ezUInt32 targetStride = ezImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const float* sourcePointer = static_cast<const float*>(static_cast<const void*>(source.GetPtr()));
    float* targetPointer = static_cast<float*>(static_cast<void*>(target.GetPtr()));

    const ezUInt32 numChannels = sourceStride / sizeof(float);

    while (numElements)
    {
      // Copy existing channels
      memcpy(targetPointer, sourcePointer, numChannels * sizeof(float));

      // Fill others with zero
      memset(targetPointer + numChannels, 0, sizeof(float) * (3 - numChannels));

      // Set alpha to 1
      targetPointer[3] = 1.0f;

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

struct ezImageConversion_DiscardChannels : public ezImageConversionStepLinear
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::R32G32B32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::R32G32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::R32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32A32_UINT, ezImageFormat::R32G32B32_UINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32A32_UINT, ezImageFormat::R32G32_UINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32A32_UINT, ezImageFormat::R32_UINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32A32_SINT, ezImageFormat::R32G32B32_SINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32A32_SINT, ezImageFormat::R32G32_SINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32A32_SINT, ezImageFormat::R32_SINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32_FLOAT, ezImageFormat::R32G32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32_FLOAT, ezImageFormat::R32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32_UINT, ezImageFormat::R32G32_UINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32_UINT, ezImageFormat::R32_UINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32_SINT, ezImageFormat::R32G32_SINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32_SINT, ezImageFormat::R32_SINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16B16A16_FLOAT, ezImageFormat::R16G16_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16B16A16_FLOAT, ezImageFormat::R16_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16B16A16_UNORM, ezImageFormat::R16G16B16_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16B16A16_UNORM, ezImageFormat::R16G16_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16B16A16_UNORM, ezImageFormat::R16_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16B16A16_UINT, ezImageFormat::R16G16_UINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16B16A16_UINT, ezImageFormat::R16_UINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16B16A16_SNORM, ezImageFormat::R16G16_SNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16B16A16_SNORM, ezImageFormat::R16_SNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16B16A16_SINT, ezImageFormat::R16G16_SINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16B16A16_SINT, ezImageFormat::R16_SINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16B16_UNORM, ezImageFormat::R16G16_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16B16_UNORM, ezImageFormat::R16_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32_FLOAT, ezImageFormat::R32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32_UINT, ezImageFormat::R32_UINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32_SINT, ezImageFormat::R32_SINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::D32_FLOAT_S8X24_UINT, ezImageFormat::D32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::R8G8B8_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::R8G8_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::R8_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::R8G8B8_UNORM_SRGB, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_UINT, ezImageFormat::R8G8_UINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_UINT, ezImageFormat::R8_UINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_SNORM, ezImageFormat::R8G8_SNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_SNORM, ezImageFormat::R8_SNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_SINT, ezImageFormat::R8G8_SINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8A8_SINT, ezImageFormat::R8_SINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::B8G8R8A8_UNORM, ezImageFormat::B8G8R8_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::B8G8R8A8_UNORM_SRGB, ezImageFormat::B8G8R8_UNORM_SRGB, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::B8G8R8X8_UNORM, ezImageFormat::B8G8R8_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::B8G8R8X8_UNORM_SRGB, ezImageFormat::B8G8R8_UNORM_SRGB, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16_FLOAT, ezImageFormat::R16_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16_UNORM, ezImageFormat::R16_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16_UINT, ezImageFormat::R16_UINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16_SNORM, ezImageFormat::R16_SNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R16G16_SINT, ezImageFormat::R16_SINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8_UNORM, ezImageFormat::R8G8_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8B8_UNORM, ezImageFormat::R8_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8_UNORM, ezImageFormat::R8_UNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8_UINT, ezImageFormat::R8_UINT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8_SNORM, ezImageFormat::R8_SNORM, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R8G8_SINT, ezImageFormat::R8_SINT, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    ezUInt32 sourceStride = ezImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    ezUInt32 targetStride = ezImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    if (ezImageFormat::GetBitsPerPixel(sourceFormat) == 32 && ezImageFormat::GetBitsPerPixel(targetFormat) == 24)
    {
      // Fast path for RGBA -> RGB
      while (numElements)
      {
        const ezUInt8* src = static_cast<const ezUInt8*>(sourcePointer);
        ezUInt8* dst = static_cast<ezUInt8*>(targetPointer);

        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];

        sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
        targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
        numElements--;
      }
    }

    while (numElements)
    {
      memcpy(targetPointer, sourcePointer, targetStride);

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

class ezImageConversion_FLOAT_to_R11G11B10 : public ezImageConversionStepLinear
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::R11G11B10_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R32G32B32_FLOAT, ezImageFormat::R11G11B10_FLOAT, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    ezUInt32 sourceStride = ezImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    ezUInt32 targetStride = ezImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      // Adapted from DirectXMath's XMStoreFloat3PK
      ezUInt32 IValue[3];
      memcpy(IValue, sourcePointer, 12);

      ezUInt32 Result[3];

      // X & Y Channels (5-bit exponent, 6-bit mantissa)
      for (ezUInt32 j = 0; j < 2; ++j)
      {
        ezUInt32 Sign = IValue[j] & 0x80000000;
        ezUInt32 I = IValue[j] & 0x7FFFFFFF;

        if ((I & 0x7F800000) == 0x7F800000)
        {
          // INF or NAN
          Result[j] = 0x7c0;
          if ((I & 0x7FFFFF) != 0)
          {
            Result[j] = 0x7c0 | (((I >> 17) | (I >> 11) | (I >> 6) | (I)) & 0x3f);
          }
          else if (Sign)
          {
            // -INF is clamped to 0 since 3PK is positive only
            Result[j] = 0;
          }
        }
        else if (Sign)
        {
          // 3PK is positive only, so clamp to zero
          Result[j] = 0;
        }
        else if (I > 0x477E0000U)
        {
          // The number is too large to be represented as a float11, set to max
          Result[j] = 0x7BF;
        }
        else
        {
          if (I < 0x38800000U)
          {
            // The number is too small to be represented as a normalized float11
            // Convert it to a denormalized value.
            ezUInt32 Shift = 113U - (I >> 23U);
            I = (0x800000U | (I & 0x7FFFFFU)) >> Shift;
          }
          else
          {
            // Rebias the exponent to represent the value as a normalized float11
            I += 0xC8000000U;
          }

          Result[j] = ((I + 0xFFFFU + ((I >> 17U) & 1U)) >> 17U) & 0x7ffU;
        }
      }

      // Z Channel (5-bit exponent, 5-bit mantissa)
      ezUInt32 Sign = IValue[2] & 0x80000000;
      ezUInt32 I = IValue[2] & 0x7FFFFFFF;

      if ((I & 0x7F800000) == 0x7F800000)
      {
        // INF or NAN
        Result[2] = 0x3e0;
        if (I & 0x7FFFFF)
        {
          Result[2] = 0x3e0 | (((I >> 18) | (I >> 13) | (I >> 3) | (I)) & 0x1f);
        }
        else if (Sign)
        {
          // -INF is clamped to 0 since 3PK is positive only
          Result[2] = 0;
        }
      }
      else if (Sign)
      {
        // 3PK is positive only, so clamp to zero
        Result[2] = 0;
      }
      else if (I > 0x477C0000U)
      {
        // The number is too large to be represented as a float10, set to max
        Result[2] = 0x3df;
      }
      else
      {
        if (I < 0x38800000U)
        {
          // The number is too small to be represented as a normalized float10
          // Convert it to a denormalized value.
          ezUInt32 Shift = 113U - (I >> 23U);
          I = (0x800000U | (I & 0x7FFFFFU)) >> Shift;
        }
        else
        {
          // Rebias the exponent to represent the value as a normalized float10
          I += 0xC8000000U;
        }

        Result[2] = ((I + 0x1FFFFU + ((I >> 18U) & 1U)) >> 18U) & 0x3ffU;
      }

      // Pack Result into memory
      *reinterpret_cast<ezUInt32*>(targetPointer) = (Result[0] & 0x7ff) | ((Result[1] & 0x7ff) << 11) | ((Result[2] & 0x3ff) << 22);

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

class ezImageConversion_R11G11B10_to_FLOAT : public ezImageConversionStepLinear
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R11G11B10_FLOAT, ezImageFormat::R32G32B32A32_FLOAT, ezImageConversionFlags::Default),
        ezImageConversionEntry(ezImageFormat::R11G11B10_FLOAT, ezImageFormat::R32G32B32_FLOAT, ezImageConversionFlags::Default),
    };

    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    ezUInt32 sourceStride = ezImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    ezUInt32 targetStride = ezImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      const R11G11B10* pSource = reinterpret_cast<const R11G11B10*>(sourcePointer);
      ezUInt32* targetUi = reinterpret_cast<ezUInt32*>(targetPointer);

      // Adapted from XMLoadFloat3PK
      ezUInt32 Mantissa;
      ezUInt32 Exponent;

      // X Channel (6-bit mantissa)
      Mantissa = pSource->p.xm;

      if (pSource->p.xe == 0x1f) // INF or NAN
      {
        targetUi[0] = 0x7f800000 | (pSource->p.xm << 17);
      }
      else
      {
        if (pSource->p.xe != 0) // The value is normalized
        {
          Exponent = pSource->p.xe;
        }
        else if (Mantissa != 0) // The value is denormalized
        {
          // Normalize the value in the resulting float
          Exponent = 1;

          do
          {
            Exponent--;
            Mantissa <<= 1;
          } while ((Mantissa & 0x40) == 0);

          Mantissa &= 0x3F;
        }
        else // The value is zero
        {
          Exponent = (ezUInt32)-112;
        }

        targetUi[0] = ((Exponent + 112) << 23) | (Mantissa << 17);
      }

      // Y Channel (6-bit mantissa)
      Mantissa = pSource->p.ym;

      if (pSource->p.ye == 0x1f) // INF or NAN
      {
        targetUi[1] = 0x7f800000 | (pSource->p.ym << 17);
      }
      else
      {
        if (pSource->p.ye != 0) // The value is normalized
        {
          Exponent = pSource->p.ye;
        }
        else if (Mantissa != 0) // The value is denormalized
        {
          // Normalize the value in the resulting float
          Exponent = 1;

          do
          {
            Exponent--;
            Mantissa <<= 1;
          } while ((Mantissa & 0x40) == 0);

          Mantissa &= 0x3F;
        }
        else // The value is zero
        {
          Exponent = (ezUInt32)-112;
        }

        targetUi[1] = ((Exponent + 112) << 23) | (Mantissa << 17);
      }

      // Z Channel (5-bit mantissa)
      Mantissa = pSource->p.zm;

      if (pSource->p.ze == 0x1f) // INF or NAN
      {
        targetUi[2] = 0x7f800000 | (pSource->p.zm << 17);
      }
      else
      {
        if (pSource->p.ze != 0) // The value is normalized
        {
          Exponent = pSource->p.ze;
        }
        else if (Mantissa != 0) // The value is denormalized
        {
          // Normalize the value in the resulting float
          Exponent = 1;

          do
          {
            Exponent--;
            Mantissa <<= 1;
          } while ((Mantissa & 0x20) == 0);

          Mantissa &= 0x1F;
        }
        else // The value is zero
        {
          Exponent = (ezUInt32)-112;
        }

        targetUi[2] = ((Exponent + 112) << 23) | (Mantissa << 18);
      }

      if (targetStride > sizeof(float) * 3)
      {
        reinterpret_cast<float*>(targetPointer)[3] = 1.0f; // Write alpha channel
      }
      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};

class ezImageConversion_R11G11B10_to_HALF : public ezImageConversionStepLinear
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
        ezImageConversionEntry(ezImageFormat::R11G11B10_FLOAT, ezImageFormat::R16G16B16A16_FLOAT, ezImageConversionFlags::Default)};
    return supportedConversions;
  }

  virtual ezResult ConvertPixels(ezArrayPtr<const void> source, ezArrayPtr<void> target, ezUInt32 numElements,
                                 ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    ezUInt32 sourceStride = ezImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    ezUInt32 targetStride = ezImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      ezUInt16* result = reinterpret_cast<ezUInt16*>(targetPointer);
      const R11G11B10* r11g11b10 = reinterpret_cast<const R11G11B10*>(sourcePointer);

      // We can do a straight forward conversion here because R11G11B10 uses the same number of bits for the exponent as a half
      // This means that all special values, e.g. denormals, inf, nan map exactly.
      result[0] = static_cast<ezUInt16>((r11g11b10->p.xe << 10) | (r11g11b10->p.xm << 4));
      result[1] = static_cast<ezUInt16>((r11g11b10->p.ye << 10) | (r11g11b10->p.ym << 4));
      result[2] = static_cast<ezUInt16>((r11g11b10->p.ze << 10) | (r11g11b10->p.zm << 5));
      result[3] = 0x3C00; // hex value of 1.0f as half

      sourcePointer = ezMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = ezMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return EZ_SUCCESS;
  }
};



#define ADD_16BPP_CONVERSION(format)                                                                                                       \
  static ezImageConversionStep_Decompress16bpp<ezDecompress##format, ezImageFormat::format##_UNORM> s_conversion_ezDecompress##format;     \
  static ezImageConversionStep_Compress16bpp<ezCompress##format, ezImageFormat::format##_UNORM> s_conversion_ezCompress##format

ADD_16BPP_CONVERSION(A4B4G4R4);
ADD_16BPP_CONVERSION(B4G4R4A4);
ADD_16BPP_CONVERSION(B5G6R5);
ADD_16BPP_CONVERSION(B5G5R5X1);
ADD_16BPP_CONVERSION(B5G5R5A1);
ADD_16BPP_CONVERSION(X1B5G5R5);
ADD_16BPP_CONVERSION(A1B5G5R5);

static ezImageSwizzleConversion32_2103 s_conversion_swizzle2103;
static ezImageConversion_BGRX_BGRA s_conversion_BGRX_BGRA;
static ezImageConversion_F32_U8 s_conversion_F32_U8;
static ezImageConversion_F32_sRGB s_conversion_F32_sRGB;
static ezImageConversion_F32_U16 s_conversion_F32_U16;
static ezImageConversion_F32_F16 s_conversion_F32_F16;
static ezImageConversion_F32_S8 s_conversion_F32_S8;
static ezImageConversion_U8_F32 s_conversion_U8_F32;
static ezImageConversion_sRGB_F32 s_conversion_sRGB_F32;
static ezImageConversion_U16_F32 s_conversion_U16_F32;
static ezImageConversion_F16_F32 s_conversion_F16_F32;
static ezImageConversion_S8_F32 s_conversion_S8_F32;
static ezImageConversion_Pad_To_RGBA_U8 s_conversion_Pad_To_RGBA_U8;
static ezImageConversion_Pad_To_RGBA_F32 s_conversion_Pad_To_RGBA_F32;
static ezImageConversion_DiscardChannels s_conversion_DiscardChannels;

static ezImageConversion_R11G11B10_to_FLOAT s_conversion_R11G11B10_to_FLOAT;
static ezImageConversion_R11G11B10_to_HALF s_conversion_R11G11B10_to_HALF;
static ezImageConversion_FLOAT_to_R11G11B10 s_conversion_FLOAT_to_R11G11B10;

EZ_STATICLINK_FILE(Texture, Texture_Image_Conversions_PixelConversions);

