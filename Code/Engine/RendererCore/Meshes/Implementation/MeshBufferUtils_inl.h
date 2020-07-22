
// static
EZ_ALWAYS_INLINE ezGALResourceFormat::Enum ezMeshNormalPrecision::ToResourceFormatNormal(Enum value)
{
  return value == _10Bit ? ezGALResourceFormat::RGB10A2UIntNormalized
                         : (value == _16Bit ? ezGALResourceFormat::RGBAUShortNormalized : ezGALResourceFormat::XYZFloat);
}

// static
EZ_ALWAYS_INLINE ezGALResourceFormat::Enum ezMeshNormalPrecision::ToResourceFormatTangent(Enum value)
{
  return value == _10Bit ? ezGALResourceFormat::RGB10A2UIntNormalized
                         : (value == _16Bit ? ezGALResourceFormat::RGBAUShortNormalized : ezGALResourceFormat::XYZWFloat);
}

//////////////////////////////////////////////////////////////////////////

// static
EZ_ALWAYS_INLINE ezGALResourceFormat::Enum ezMeshTexCoordPrecision::ToResourceFormat(Enum value)
{
  return value == _16Bit ? ezGALResourceFormat::UVHalf : ezGALResourceFormat::UVFloat;
}

//////////////////////////////////////////////////////////////////////////

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeNormal(const ezVec3& normal, ezArrayPtr<ezUInt8> dest, ezMeshNormalPrecision::Enum normalPrecision)
{
  return EncodeNormal(normal, dest, ezMeshNormalPrecision::ToResourceFormatNormal(normalPrecision));
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeTangent(
  const ezVec3& tangent, float biTangentSign, ezArrayPtr<ezUInt8> dest, ezMeshNormalPrecision::Enum tangentPrecision)
{
  return EncodeTangent(tangent, biTangentSign, dest, ezMeshNormalPrecision::ToResourceFormatTangent(tangentPrecision));
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeTexCoord(
  const ezVec2& texCoord, ezArrayPtr<ezUInt8> dest, ezMeshTexCoordPrecision::Enum texCoordPrecision)
{
  return EncodeTexCoord(texCoord, dest, ezMeshTexCoordPrecision::ToResourceFormat(texCoordPrecision));
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeNormal(const ezVec3& normal, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat)
{
  // we store normals in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec3(normal * 0.5f + ezVec3(0.5f), dest, destFormat);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeTangent(
  const ezVec3& tangent, float biTangentSign, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat)
{
  // make sure biTangentSign is either -1 or 1
  biTangentSign = (biTangentSign < 0.0f) ? -1.0f : 1.0f;

  // we store tangents in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec4(tangent.GetAsVec4(biTangentSign) * 0.5f + ezVec4(0.5f), dest, destFormat);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeTexCoord(const ezVec2& texCoord, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat)
{
  return EncodeFromVec2(texCoord, dest, destFormat);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeNormal(
  ezArrayPtr<const ezUInt8> source, ezVec3& destNormal, ezMeshNormalPrecision::Enum normalPrecision)
{
  return DecodeNormal(source, ezMeshNormalPrecision::ToResourceFormatNormal(normalPrecision), destNormal);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeTangent(
  ezArrayPtr<const ezUInt8> source, ezVec3& destTangent, float& destBiTangentSign, ezMeshNormalPrecision::Enum tangentPrecision)
{
  return DecodeTangent(source, ezMeshNormalPrecision::ToResourceFormatTangent(tangentPrecision), destTangent, destBiTangentSign);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeTexCoord(
  ezArrayPtr<const ezUInt8> source, ezVec2& destTexCoord, ezMeshTexCoordPrecision::Enum texCoordPrecision)
{
  return DecodeTexCoord(source, ezMeshTexCoordPrecision::ToResourceFormat(texCoordPrecision), destTexCoord);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeNormal(
  ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec3& destNormal)
{
  ezVec3 tempNormal;
  EZ_SUCCEED_OR_RETURN(DecodeToVec3(source, sourceFormat, tempNormal));
  destNormal = tempNormal * 2.0f - ezVec3(1.0f);
  return EZ_SUCCESS;
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeTangent(
  ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec3& destTangent, float& destBiTangentSign)
{
  ezVec4 tempTangent;
  EZ_SUCCEED_OR_RETURN(DecodeToVec4(source, sourceFormat, tempTangent));
  destTangent = tempTangent.GetAsVec3() * 2.0f - ezVec3(1.0f);
  destBiTangentSign = tempTangent.w * 2.0f - 1.0f;
  return EZ_SUCCESS;
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeTexCoord(
  ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec2& destTexCoord)
{
  return DecodeToVec2(source, sourceFormat, destTexCoord);
}
