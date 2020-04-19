#include <RendererFoundationPCH.h>

#include <Foundation/Math/Float16.h>
#include <RendererFoundation/Resources/ResourceFormatConversions.h>

namespace
{
  template <ezUInt32 Bits>
  EZ_ALWAYS_INLINE ezUInt32 ColorFloatToUNorm(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (ezMath::IsNaN(value))
    {
      return 0;
    }
    else
    {
      float fMaxValue = ((1 << Bits) - 1);
      return ezMath::Saturate(value) * fMaxValue + 0.5f;
    }
  }

  template <ezUInt32 Bits>
  constexpr inline float ColorUNormToFloat(ezUInt32 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    ezUInt32 uiMaxValue = ((1 << Bits) - 1);
    float fMaxValue = ((1 << Bits) - 1);
    return (value & uiMaxValue) * (1.0f / fMaxValue);
  }
} // namespace

// static
ezResult ezGALResourceFormatConversions::FromFloat1(const float source, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat)
{
  EZ_ASSERT_DEBUG(dest.GetCount() >= ezGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case ezGALResourceFormat::RFloat:
      *reinterpret_cast<float*>(dest.GetPtr()) = source;
      return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

// static
ezResult ezGALResourceFormatConversions::FromFloat2(const ezVec2& source, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat)
{
  EZ_ASSERT_DEBUG(dest.GetCount() >= ezGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case ezGALResourceFormat::RGFloat:
      *reinterpret_cast<ezVec2*>(dest.GetPtr()) = source;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGHalf:
      *reinterpret_cast<ezFloat16Vec2*>(dest.GetPtr()) = source;
      return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

// static
ezResult ezGALResourceFormatConversions::FromFloat3(const ezVec3& source, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat)
{
  EZ_ASSERT_DEBUG(dest.GetCount() >= ezGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case ezGALResourceFormat::RGBFloat:
      *reinterpret_cast<ezVec3*>(dest.GetPtr()) = source;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUShortNormalized:
      reinterpret_cast<ezUInt16*>(dest.GetPtr())[0] = ezMath::ColorFloatToShort(source.x);
      reinterpret_cast<ezUInt16*>(dest.GetPtr())[1] = ezMath::ColorFloatToShort(source.y);
      reinterpret_cast<ezUInt16*>(dest.GetPtr())[2] = ezMath::ColorFloatToShort(source.z);
      reinterpret_cast<ezUInt16*>(dest.GetPtr())[3] = 0;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAShortNormalized:
      reinterpret_cast<ezInt16*>(dest.GetPtr())[0] = ezMath::ColorFloatToSignedShort(source.x);
      reinterpret_cast<ezInt16*>(dest.GetPtr())[1] = ezMath::ColorFloatToSignedShort(source.y);
      reinterpret_cast<ezInt16*>(dest.GetPtr())[2] = ezMath::ColorFloatToSignedShort(source.z);
      reinterpret_cast<ezInt16*>(dest.GetPtr())[3] = 0;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGB10A2UIntNormalized:
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) = ColorFloatToUNorm<10>(source.x);
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(source.y) << 10;
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(source.z) << 20;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUByteNormalized:
      dest.GetPtr()[0] = ezMath::ColorFloatToByte(source.x);
      dest.GetPtr()[1] = ezMath::ColorFloatToByte(source.y);
      dest.GetPtr()[2] = ezMath::ColorFloatToByte(source.z);
      dest.GetPtr()[3] = 0;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAByteNormalized:
      dest.GetPtr()[0] = ezMath::ColorFloatToSignedByte(source.x);
      dest.GetPtr()[1] = ezMath::ColorFloatToSignedByte(source.y);
      dest.GetPtr()[2] = ezMath::ColorFloatToSignedByte(source.z);
      dest.GetPtr()[3] = 0;
      return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

// static
ezResult ezGALResourceFormatConversions::FromFloat4(const ezVec4& source, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat)
{
  EZ_ASSERT_DEBUG(dest.GetCount() >= ezGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case ezGALResourceFormat::RGBAFloat:
      *reinterpret_cast<ezVec4*>(dest.GetPtr()) = source;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAHalf:
      *reinterpret_cast<ezFloat16Vec4*>(dest.GetPtr()) = source;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUShortNormalized:
      reinterpret_cast<ezUInt16*>(dest.GetPtr())[0] = ezMath::ColorFloatToShort(source.x);
      reinterpret_cast<ezUInt16*>(dest.GetPtr())[1] = ezMath::ColorFloatToShort(source.y);
      reinterpret_cast<ezUInt16*>(dest.GetPtr())[2] = ezMath::ColorFloatToShort(source.z);
      reinterpret_cast<ezUInt16*>(dest.GetPtr())[3] = ezMath::ColorFloatToShort(source.w);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAShortNormalized:
      reinterpret_cast<ezInt16*>(dest.GetPtr())[0] = ezMath::ColorFloatToSignedShort(source.x);
      reinterpret_cast<ezInt16*>(dest.GetPtr())[1] = ezMath::ColorFloatToSignedShort(source.y);
      reinterpret_cast<ezInt16*>(dest.GetPtr())[2] = ezMath::ColorFloatToSignedShort(source.z);
      reinterpret_cast<ezInt16*>(dest.GetPtr())[3] = ezMath::ColorFloatToSignedShort(source.w);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGB10A2UIntNormalized:
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) = ColorFloatToUNorm<10>(source.x);
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(source.y) << 10;
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(source.z) << 20;
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<2>(source.w) << 30;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUByteNormalized:
      dest.GetPtr()[0] = ezMath::ColorFloatToByte(source.x);
      dest.GetPtr()[1] = ezMath::ColorFloatToByte(source.y);
      dest.GetPtr()[2] = ezMath::ColorFloatToByte(source.z);
      dest.GetPtr()[3] = ezMath::ColorFloatToByte(source.w);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAByteNormalized:
      dest.GetPtr()[0] = ezMath::ColorFloatToSignedByte(source.x);
      dest.GetPtr()[1] = ezMath::ColorFloatToSignedByte(source.y);
      dest.GetPtr()[2] = ezMath::ColorFloatToSignedByte(source.z);
      dest.GetPtr()[3] = ezMath::ColorFloatToSignedByte(source.w);
      return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

// static
ezResult ezGALResourceFormatConversions::ToFloat1(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, float& dest)
{
  EZ_ASSERT_DEBUG(source.GetCount() >= ezGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case ezGALResourceFormat::RFloat:
      dest = *reinterpret_cast<const float*>(source.GetPtr());
      return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

// static
ezResult ezGALResourceFormatConversions::ToFloat2(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec2& dest)
{
  EZ_ASSERT_DEBUG(source.GetCount() >= ezGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case ezGALResourceFormat::RGFloat:
      dest = *reinterpret_cast<const ezVec2*>(source.GetPtr());
      return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

// static
ezResult ezGALResourceFormatConversions::ToFloat3(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec3& dest)
{
  EZ_ASSERT_DEBUG(source.GetCount() >= ezGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case ezGALResourceFormat::RGBFloat:
      dest = *reinterpret_cast<const ezVec3*>(source.GetPtr());
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUShortNormalized:
      dest.x = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[0]);
      dest.y = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[1]);
      dest.z = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[2]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAShortNormalized:
      dest.x = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[0]);
      dest.y = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[1]);
      dest.z = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[2]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGB10A2UIntNormalized:
      dest.x = ColorUNormToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()));
      dest.y = ColorUNormToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()) >> 10);
      dest.z = ColorUNormToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()) >> 20);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUByteNormalized:
      dest.x = ezMath::ColorByteToFloat(source.GetPtr()[0]);
      dest.y = ezMath::ColorByteToFloat(source.GetPtr()[1]);
      dest.z = ezMath::ColorByteToFloat(source.GetPtr()[2]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAByteNormalized:
      dest.x = ezMath::ColorSignedByteToFloat(source.GetPtr()[0]);
      dest.y = ezMath::ColorSignedByteToFloat(source.GetPtr()[1]);
      dest.z = ezMath::ColorSignedByteToFloat(source.GetPtr()[2]);
      return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

// static
ezResult ezGALResourceFormatConversions::ToFloat4(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec4& dest)
{
  EZ_ASSERT_DEBUG(source.GetCount() >= ezGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case ezGALResourceFormat::RGBAFloat:
      dest = *reinterpret_cast<const ezVec4*>(source.GetPtr());
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAHalf:
      dest = *reinterpret_cast<const ezFloat16Vec4*>(source.GetPtr());
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUShortNormalized:
      dest.x = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[0]);
      dest.y = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[1]);
      dest.z = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[2]);
      dest.w = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[3]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAShortNormalized:
      dest.x = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[0]);
      dest.y = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[1]);
      dest.z = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[2]);
      dest.w = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[3]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGB10A2UIntNormalized:
      dest.x = ColorUNormToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()));
      dest.y = ColorUNormToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()) >> 10);
      dest.z = ColorUNormToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()) >> 20);
      dest.w = ColorUNormToFloat<2>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()) >> 30);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUByteNormalized:
      dest.x = ezMath::ColorByteToFloat(source.GetPtr()[0]);
      dest.y = ezMath::ColorByteToFloat(source.GetPtr()[1]);
      dest.z = ezMath::ColorByteToFloat(source.GetPtr()[2]);
      dest.w = ezMath::ColorByteToFloat(source.GetPtr()[3]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAByteNormalized:
      dest.x = ezMath::ColorSignedByteToFloat(source.GetPtr()[0]);
      dest.y = ezMath::ColorSignedByteToFloat(source.GetPtr()[1]);
      dest.z = ezMath::ColorSignedByteToFloat(source.GetPtr()[2]);
      dest.w = ezMath::ColorSignedByteToFloat(source.GetPtr()[3]);
      return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}
