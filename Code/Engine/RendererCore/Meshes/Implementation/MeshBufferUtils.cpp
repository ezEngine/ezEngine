#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

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
      return static_cast<ezUInt32>(ezMath::Saturate(value) * fMaxValue + 0.5f);
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

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezMeshNormalPrecision, 1)
  EZ_ENUM_CONSTANT(ezMeshNormalPrecision::_10Bit),
  EZ_ENUM_CONSTANT(ezMeshNormalPrecision::_16Bit),
  EZ_ENUM_CONSTANT(ezMeshNormalPrecision::_32Bit),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezMeshTexCoordPrecision, 1)
  EZ_ENUM_CONSTANT(ezMeshTexCoordPrecision::_16Bit),
  EZ_ENUM_CONSTANT(ezMeshTexCoordPrecision::_32Bit),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezMeshBoneWeigthPrecision, 1)
  EZ_ENUM_CONSTANT(ezMeshBoneWeigthPrecision::_8Bit),
  EZ_ENUM_CONSTANT(ezMeshBoneWeigthPrecision::_10Bit),
  EZ_ENUM_CONSTANT(ezMeshBoneWeigthPrecision::_16Bit),
  EZ_ENUM_CONSTANT(ezMeshBoneWeigthPrecision::_32Bit),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezMeshVertexColorConversion, 1)
  EZ_ENUM_CONSTANT(ezMeshVertexColorConversion::None),
  EZ_ENUM_CONSTANT(ezMeshVertexColorConversion::LinearToSrgb),
  EZ_ENUM_CONSTANT(ezMeshVertexColorConversion::SrgbToLinear),
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
ezResult ezMeshBufferUtils::EncodeFromFloat(const float fSource, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat)
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
ezResult ezMeshBufferUtils::EncodeFromVec2(const ezVec2& vSource, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat)
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
ezResult ezMeshBufferUtils::EncodeFromVec3(const ezVec3& vSource, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat)
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
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) = ColorFloatToUNorm<10>(vSource.x);
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(vSource.y) << 10;
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(vSource.z) << 20;
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
ezResult ezMeshBufferUtils::EncodeFromVec4(const ezVec4& vSource, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat)
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
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) = ColorFloatToUNorm<10>(vSource.x);
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(vSource.y) << 10;
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(vSource.z) << 20;
      *reinterpret_cast<ezUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<2>(vSource.w) << 30;
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
ezResult ezMeshBufferUtils::DecodeToFloat(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, float& ref_fDest)
{
  EZ_ASSERT_DEBUG(source.GetCount() >= ezGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case ezGALResourceFormat::RFloat:
      ref_fDest = *reinterpret_cast<const float*>(source.GetPtr());
      return EZ_SUCCESS;
    case ezGALResourceFormat::RHalf:
      ref_fDest = *reinterpret_cast<const ezFloat16*>(source.GetPtr());
      return EZ_SUCCESS;
    default:
      return EZ_FAILURE;
  }
}

// static
ezResult ezMeshBufferUtils::DecodeToVec2(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec2& ref_vDest)
{
  EZ_ASSERT_DEBUG(source.GetCount() >= ezGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case ezGALResourceFormat::RGFloat:
      ref_vDest = *reinterpret_cast<const ezVec2*>(source.GetPtr());
      return EZ_SUCCESS;
    case ezGALResourceFormat::RGHalf:
      ref_vDest = *reinterpret_cast<const ezFloat16Vec2*>(source.GetPtr());
      return EZ_SUCCESS;
    default:
      return EZ_FAILURE;
  }
}

// static
ezResult ezMeshBufferUtils::DecodeToVec3(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec3& ref_vDest)
{
  EZ_ASSERT_DEBUG(source.GetCount() >= ezGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case ezGALResourceFormat::RGBFloat:
      ref_vDest = *reinterpret_cast<const ezVec3*>(source.GetPtr());
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUShortNormalized:
      ref_vDest.x = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[0]);
      ref_vDest.y = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[1]);
      ref_vDest.z = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[2]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAShortNormalized:
      ref_vDest.x = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[0]);
      ref_vDest.y = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[1]);
      ref_vDest.z = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[2]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGB10A2UIntNormalized:
      ref_vDest.x = ColorUNormToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()));
      ref_vDest.y = ColorUNormToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()) >> 10);
      ref_vDest.z = ColorUNormToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()) >> 20);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUByteNormalized:
      ref_vDest.x = ezMath::ColorByteToFloat(source.GetPtr()[0]);
      ref_vDest.y = ezMath::ColorByteToFloat(source.GetPtr()[1]);
      ref_vDest.z = ezMath::ColorByteToFloat(source.GetPtr()[2]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAByteNormalized:
      ref_vDest.x = ezMath::ColorSignedByteToFloat(source.GetPtr()[0]);
      ref_vDest.y = ezMath::ColorSignedByteToFloat(source.GetPtr()[1]);
      ref_vDest.z = ezMath::ColorSignedByteToFloat(source.GetPtr()[2]);
      return EZ_SUCCESS;
    default:
      return EZ_FAILURE;
  }
}

// static
ezResult ezMeshBufferUtils::DecodeToVec4(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec4& ref_vDest)
{
  EZ_ASSERT_DEBUG(source.GetCount() >= ezGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case ezGALResourceFormat::RGBAFloat:
      ref_vDest = *reinterpret_cast<const ezVec4*>(source.GetPtr());
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAHalf:
      ref_vDest = *reinterpret_cast<const ezFloat16Vec4*>(source.GetPtr());
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUShortNormalized:
      ref_vDest.x = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[0]);
      ref_vDest.y = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[1]);
      ref_vDest.z = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[2]);
      ref_vDest.w = ezMath::ColorShortToFloat(reinterpret_cast<const ezUInt16*>(source.GetPtr())[3]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAShortNormalized:
      ref_vDest.x = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[0]);
      ref_vDest.y = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[1]);
      ref_vDest.z = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[2]);
      ref_vDest.w = ezMath::ColorSignedShortToFloat(reinterpret_cast<const ezInt16*>(source.GetPtr())[3]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGB10A2UIntNormalized:
      ref_vDest.x = ColorUNormToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()));
      ref_vDest.y = ColorUNormToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()) >> 10);
      ref_vDest.z = ColorUNormToFloat<10>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()) >> 20);
      ref_vDest.w = ColorUNormToFloat<2>(*reinterpret_cast<const ezUInt32*>(source.GetPtr()) >> 30);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAUByteNormalized:
      ref_vDest.x = ezMath::ColorByteToFloat(source.GetPtr()[0]);
      ref_vDest.y = ezMath::ColorByteToFloat(source.GetPtr()[1]);
      ref_vDest.z = ezMath::ColorByteToFloat(source.GetPtr()[2]);
      ref_vDest.w = ezMath::ColorByteToFloat(source.GetPtr()[3]);
      return EZ_SUCCESS;

    case ezGALResourceFormat::RGBAByteNormalized:
      ref_vDest.x = ezMath::ColorSignedByteToFloat(source.GetPtr()[0]);
      ref_vDest.y = ezMath::ColorSignedByteToFloat(source.GetPtr()[1]);
      ref_vDest.z = ezMath::ColorSignedByteToFloat(source.GetPtr()[2]);
      ref_vDest.w = ezMath::ColorSignedByteToFloat(source.GetPtr()[3]);
      return EZ_SUCCESS;

    default:
      return EZ_FAILURE;
  }
}

// static
ezResult ezMeshBufferUtils::GetPositionStream(const ezMeshBufferResourceDescriptor& meshBufferDesc, const ezVec3*& out_pPositions, ezUInt32& out_uiElementStride)
{
  const ezVertexDeclarationInfo& vdi = meshBufferDesc.GetVertexDeclaration();
  const ezUInt8* pRawVertexData = meshBufferDesc.GetVertexBufferData().GetPtr();

  const ezVec3* pPositions = nullptr;

  for (ezUInt32 vs = 0; vs < vdi.m_VertexStreams.GetCount(); ++vs)
  {
    if (vdi.m_VertexStreams[vs].m_Semantic == ezGALVertexAttributeSemantic::Position)
    {
      if (vdi.m_VertexStreams[vs].m_Format != ezGALResourceFormat::RGBFloat)
      {
        ezLog::Error("Unsupported vertex position format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return EZ_FAILURE; // other position formats are not supported
      }

      pPositions = reinterpret_cast<const ezVec3*>(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
  }

  if (pPositions == nullptr)
  {
    ezLog::Error("No position stream found");
    return EZ_FAILURE;
  }

  out_pPositions = pPositions;
  out_uiElementStride = meshBufferDesc.GetVertexDataSize();
  return EZ_SUCCESS;
}

// static
ezResult ezMeshBufferUtils::GetPositionAndNormalStream(const ezMeshBufferResourceDescriptor& meshBufferDesc, const ezVec3*& out_pPositions, const ezUInt8*& out_pNormals, ezGALResourceFormat::Enum& out_normalFormat, ezUInt32& out_uiElementStride)
{
  const ezVertexDeclarationInfo& vdi = meshBufferDesc.GetVertexDeclaration();
  const ezUInt8* pRawVertexData = meshBufferDesc.GetVertexBufferData().GetPtr();

  const ezVec3* pPositions = nullptr;
  const ezUInt8* pNormals = nullptr;
  ezGALResourceFormat::Enum normalFormat = ezGALResourceFormat::Invalid;

  for (ezUInt32 vs = 0; vs < vdi.m_VertexStreams.GetCount(); ++vs)
  {
    if (vdi.m_VertexStreams[vs].m_Semantic == ezGALVertexAttributeSemantic::Position)
    {
      if (vdi.m_VertexStreams[vs].m_Format != ezGALResourceFormat::RGBFloat)
      {
        ezLog::Error("Unsupported vertex position format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return EZ_FAILURE; // other position formats are not supported
      }

      pPositions = reinterpret_cast<const ezVec3*>(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
    else if (vdi.m_VertexStreams[vs].m_Semantic == ezGALVertexAttributeSemantic::Normal)
    {
      pNormals = pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset;
      normalFormat = vdi.m_VertexStreams[vs].m_Format;
    }
  }

  if (pPositions == nullptr || pNormals == nullptr)
  {
    ezLog::Error("No position and normal stream found");
    return EZ_FAILURE;
  }

  ezUInt8 dummySource[16] = {};
  ezVec3 vNormal;
  if (DecodeNormal(ezMakeArrayPtr(dummySource), normalFormat, vNormal).Failed())
  {
    ezLog::Error("Unsupported vertex normal format {0}", normalFormat);
    return EZ_FAILURE;
  }

  out_pPositions = pPositions;
  out_pNormals = pNormals;
  out_normalFormat = normalFormat;
  out_uiElementStride = meshBufferDesc.GetVertexDataSize();
  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshBufferUtils);
