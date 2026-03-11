#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezMeshVertexColorConversion, 1)
  EZ_ENUM_CONSTANT(ezMeshVertexColorConversion::None),
  EZ_ENUM_CONSTANT(ezMeshVertexColorConversion::LinearToSrgb),
  EZ_ENUM_CONSTANT(ezMeshVertexColorConversion::SrgbToLinear),
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
ezResult ezMeshBufferUtils::EncodeFromFloat(const float fSource, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat)
{
  EZ_ASSERT_DEBUG(dest.GetCount() >= ezGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case ezGALResourceFormat::RFloat:
      *reinterpret_cast<float*>(dest.GetPtr()) = fSource;
      return EZ_SUCCESS;
    case ezGALResourceFormat::RHalf:
      *reinterpret_cast<ezFloat16*>(dest.GetPtr()) = fSource;
      return EZ_SUCCESS;
    default:
      return EZ_FAILURE;
  }
}

// static
ezResult ezMeshBufferUtils::EncodeFromVec2(const ezVec2& vSource, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat)
{
  EZ_ASSERT_DEBUG(dest.GetCount() >= ezGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case ezGALResourceFormat::RGFloat:
      *reinterpret_cast<ezVec2*>(dest.GetPtr()) = vSource;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGHalf:
      *reinterpret_cast<ezFloat16Vec2*>(dest.GetPtr()) = vSource;
      return EZ_SUCCESS;

    default:
      return EZ_FAILURE;
  }
}

// static
ezResult ezMeshBufferUtils::EncodeFromVec3(const ezVec3& vSource, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat)
{
  EZ_ASSERT_DEBUG(dest.GetCount() >= ezGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case ezGALResourceFormat::RGBFloat:
      *reinterpret_cast<ezVec3*>(dest.GetPtr()) = vSource;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUShortNormalized:
      reinterpret_cast<ezUInt16*>(dest.GetPtr())[0] = ezMath::ColorFloatToShort(vSource.x);
      reinterpret_cast<ezUInt16*>(dest.GetPtr())[1] = ezMath::ColorFloatToShort(vSource.y);
      reinterpret_cast<ezUInt16*>(dest.GetPtr())[2] = ezMath::ColorFloatToShort(vSource.z);
      reinterpret_cast<ezUInt16*>(dest.GetPtr())[3] = 0;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAShortNormalized:
      reinterpret_cast<ezInt16*>(dest.GetPtr())[0] = ezMath::ColorFloatToSignedShort(vSource.x);
      reinterpret_cast<ezInt16*>(dest.GetPtr())[1] = ezMath::ColorFloatToSignedShort(vSource.y);
      reinterpret_cast<ezInt16*>(dest.GetPtr())[2] = ezMath::ColorFloatToSignedShort(vSource.z);
      reinterpret_cast<ezInt16*>(dest.GetPtr())[3] = 0;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGB10A2UIntNormalized:
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) = ezMath::ColorFloatToUnsignedInt<10>(vSource.x);
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) |= ezMath::ColorFloatToUnsignedInt<10>(vSource.y) << 10;
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) |= ezMath::ColorFloatToUnsignedInt<10>(vSource.z) << 20;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUByteNormalized:
      dest.GetPtr()[0] = ezMath::ColorFloatToByte(vSource.x);
      dest.GetPtr()[1] = ezMath::ColorFloatToByte(vSource.y);
      dest.GetPtr()[2] = ezMath::ColorFloatToByte(vSource.z);
      dest.GetPtr()[3] = 0;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAByteNormalized:
      dest.GetPtr()[0] = ezMath::ColorFloatToSignedByte(vSource.x);
      dest.GetPtr()[1] = ezMath::ColorFloatToSignedByte(vSource.y);
      dest.GetPtr()[2] = ezMath::ColorFloatToSignedByte(vSource.z);
      dest.GetPtr()[3] = 0;
      return EZ_SUCCESS;
    default:
      return EZ_FAILURE;
  }
}

// static
ezResult ezMeshBufferUtils::EncodeFromVec4(const ezVec4& vSource, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat)
{
  EZ_ASSERT_DEBUG(dest.GetCount() >= ezGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case ezGALResourceFormat::RGBAFloat:
      *reinterpret_cast<ezVec4*>(dest.GetPtr()) = vSource;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAHalf:
      *reinterpret_cast<ezFloat16Vec4*>(dest.GetPtr()) = vSource;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUShortNormalized:
      reinterpret_cast<ezUInt16*>(dest.GetPtr())[0] = ezMath::ColorFloatToShort(vSource.x);
      reinterpret_cast<ezUInt16*>(dest.GetPtr())[1] = ezMath::ColorFloatToShort(vSource.y);
      reinterpret_cast<ezUInt16*>(dest.GetPtr())[2] = ezMath::ColorFloatToShort(vSource.z);
      reinterpret_cast<ezUInt16*>(dest.GetPtr())[3] = ezMath::ColorFloatToShort(vSource.w);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAShortNormalized:
      reinterpret_cast<ezInt16*>(dest.GetPtr())[0] = ezMath::ColorFloatToSignedShort(vSource.x);
      reinterpret_cast<ezInt16*>(dest.GetPtr())[1] = ezMath::ColorFloatToSignedShort(vSource.y);
      reinterpret_cast<ezInt16*>(dest.GetPtr())[2] = ezMath::ColorFloatToSignedShort(vSource.z);
      reinterpret_cast<ezInt16*>(dest.GetPtr())[3] = ezMath::ColorFloatToSignedShort(vSource.w);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGB10A2UIntNormalized:
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) = ezMath::ColorFloatToUnsignedInt<10>(vSource.x);
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) |= ezMath::ColorFloatToUnsignedInt<10>(vSource.y) << 10;
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) |= ezMath::ColorFloatToUnsignedInt<10>(vSource.z) << 20;
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) |= ezMath::ColorFloatToUnsignedInt<2>(vSource.w) << 30;
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUByteNormalized:
      dest.GetPtr()[0] = ezMath::ColorFloatToByte(vSource.x);
      dest.GetPtr()[1] = ezMath::ColorFloatToByte(vSource.y);
      dest.GetPtr()[2] = ezMath::ColorFloatToByte(vSource.z);
      dest.GetPtr()[3] = ezMath::ColorFloatToByte(vSource.w);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAByteNormalized:
      dest.GetPtr()[0] = ezMath::ColorFloatToSignedByte(vSource.x);
      dest.GetPtr()[1] = ezMath::ColorFloatToSignedByte(vSource.y);
      dest.GetPtr()[2] = ezMath::ColorFloatToSignedByte(vSource.z);
      dest.GetPtr()[3] = ezMath::ColorFloatToSignedByte(vSource.w);
      return EZ_SUCCESS;

    default:
      return EZ_FAILURE;
  }
}

// static
ezResult ezMeshBufferUtils::DecodeToFloat(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, float& out_fDest)
{
  EZ_ASSERT_DEBUG(source.GetCount() >= ezGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case ezGALResourceFormat::RFloat:
      out_fDest = *reinterpret_cast<const float*>(source.GetPtr());
      return EZ_SUCCESS;
    case ezGALResourceFormat::RHalf:
      out_fDest = *reinterpret_cast<const ezFloat16*>(source.GetPtr());
      return EZ_SUCCESS;
    default:
      return EZ_FAILURE;
  }
}

// static
ezResult ezMeshBufferUtils::DecodeToVec2(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec2& out_vDest)
{
  EZ_ASSERT_DEBUG(source.GetCount() >= ezGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case ezGALResourceFormat::RGFloat:
      out_vDest = *reinterpret_cast<const ezVec2*>(source.GetPtr());
      return EZ_SUCCESS;
    case ezGALResourceFormat::RGHalf:
      out_vDest = *reinterpret_cast<const ezFloat16Vec2*>(source.GetPtr());
      return EZ_SUCCESS;
    default:
      return EZ_FAILURE;
  }
}

// static
ezResult ezMeshBufferUtils::DecodeToVec3(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec3& out_vDest)
{
  EZ_ASSERT_DEBUG(source.GetCount() >= ezGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case ezGALResourceFormat::RGBFloat:
      out_vDest = *reinterpret_cast<const ezVec3*>(source.GetPtr());
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUShortNormalized:
      out_vDest.x = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[0]);
      out_vDest.y = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[1]);
      out_vDest.z = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[2]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAShortNormalized:
      out_vDest.x = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[0]);
      out_vDest.y = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[1]);
      out_vDest.z = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[2]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGB10A2UIntNormalized:
      out_vDest.x = ezMath::ColorUnsignedIntToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()));
      out_vDest.y = ezMath::ColorUnsignedIntToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()) >> 10);
      out_vDest.z = ezMath::ColorUnsignedIntToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()) >> 20);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUByteNormalized:
      out_vDest.x = ezMath::ColorByteToFloat(source.GetPtr()[0]);
      out_vDest.y = ezMath::ColorByteToFloat(source.GetPtr()[1]);
      out_vDest.z = ezMath::ColorByteToFloat(source.GetPtr()[2]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAByteNormalized:
      out_vDest.x = ezMath::ColorSignedByteToFloat(source.GetPtr()[0]);
      out_vDest.y = ezMath::ColorSignedByteToFloat(source.GetPtr()[1]);
      out_vDest.z = ezMath::ColorSignedByteToFloat(source.GetPtr()[2]);
      return EZ_SUCCESS;
    default:
      return EZ_FAILURE;
  }
}

// static
ezResult ezMeshBufferUtils::DecodeToVec4(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec4& out_vDest)
{
  EZ_ASSERT_DEBUG(source.GetCount() >= ezGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case ezGALResourceFormat::RGBAFloat:
      out_vDest = *reinterpret_cast<const ezVec4*>(source.GetPtr());
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAHalf:
      out_vDest = *reinterpret_cast<const ezFloat16Vec4*>(source.GetPtr());
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUShortNormalized:
      out_vDest.x = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[0]);
      out_vDest.y = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[1]);
      out_vDest.z = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[2]);
      out_vDest.w = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[3]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAShortNormalized:
      out_vDest.x = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[0]);
      out_vDest.y = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[1]);
      out_vDest.z = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[2]);
      out_vDest.w = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[3]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGB10A2UIntNormalized:
      out_vDest.x = ezMath::ColorUnsignedIntToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()));
      out_vDest.y = ezMath::ColorUnsignedIntToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()) >> 10);
      out_vDest.z = ezMath::ColorUnsignedIntToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()) >> 20);
      out_vDest.w = ezMath::ColorUnsignedIntToFloat<2>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()) >> 30);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUByteNormalized:
      out_vDest.x = ezMath::ColorByteToFloat(source.GetPtr()[0]);
      out_vDest.y = ezMath::ColorByteToFloat(source.GetPtr()[1]);
      out_vDest.z = ezMath::ColorByteToFloat(source.GetPtr()[2]);
      out_vDest.w = ezMath::ColorByteToFloat(source.GetPtr()[3]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAByteNormalized:
      out_vDest.x = ezMath::ColorSignedByteToFloat(source.GetPtr()[0]);
      out_vDest.y = ezMath::ColorSignedByteToFloat(source.GetPtr()[1]);
      out_vDest.z = ezMath::ColorSignedByteToFloat(source.GetPtr()[2]);
      out_vDest.w = ezMath::ColorSignedByteToFloat(source.GetPtr()[3]);
      return EZ_SUCCESS;

    default:
      return EZ_FAILURE;
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshBufferUtils);
