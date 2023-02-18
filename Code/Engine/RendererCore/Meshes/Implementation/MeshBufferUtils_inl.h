
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
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeNormal(const ezVec3& vNormal, ezArrayPtr<ezUInt8> dest, ezMeshNormalPrecision::Enum normalPrecision)
{
  return EncodeNormal(vNormal, dest, ezMeshNormalPrecision::ToResourceFormatNormal(normalPrecision));
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeTangent(
  const ezVec3& vTangent, float fTangentSign, ezArrayPtr<ezUInt8> dest, ezMeshNormalPrecision::Enum tangentPrecision)
{
  return EncodeTangent(vTangent, fTangentSign, dest, ezMeshNormalPrecision::ToResourceFormatTangent(tangentPrecision));
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeTexCoord(
  const ezVec2& vTexCoord, ezArrayPtr<ezUInt8> dest, ezMeshTexCoordPrecision::Enum texCoordPrecision)
{
  return EncodeTexCoord(vTexCoord, dest, ezMeshTexCoordPrecision::ToResourceFormat(texCoordPrecision));
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeNormal(const ezVec3& vNormal, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat)
{
  // we store normals in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec3(vNormal * 0.5f + ezVec3(0.5f), dest, destFormat);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeTangent(
  const ezVec3& vTangent, float fTangentSign, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat)
{
  // make sure biTangentSign is either -1 or 1
  fTangentSign = (fTangentSign < 0.0f) ? -1.0f : 1.0f;

  // we store tangents in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec4(vTangent.GetAsVec4(fTangentSign) * 0.5f + ezVec4(0.5f), dest, destFormat);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeTexCoord(const ezVec2& vTexCoord, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat)
{
  return EncodeFromVec2(vTexCoord, dest, destFormat);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeNormal(
  ezArrayPtr<const ezUInt8> source, ezVec3& ref_vDestNormal, ezMeshNormalPrecision::Enum normalPrecision)
{
  return DecodeNormal(source, ezMeshNormalPrecision::ToResourceFormatNormal(normalPrecision), ref_vDestNormal);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeTangent(
  ezArrayPtr<const ezUInt8> source, ezVec3& ref_vDestTangent, float& ref_fDestBiTangentSign, ezMeshNormalPrecision::Enum tangentPrecision)
{
  return DecodeTangent(source, ezMeshNormalPrecision::ToResourceFormatTangent(tangentPrecision), ref_vDestTangent, ref_fDestBiTangentSign);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeTexCoord(
  ezArrayPtr<const ezUInt8> source, ezVec2& ref_vDestTexCoord, ezMeshTexCoordPrecision::Enum texCoordPrecision)
{
  return DecodeTexCoord(source, ezMeshTexCoordPrecision::ToResourceFormat(texCoordPrecision), ref_vDestTexCoord);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeNormal(
  ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec3& ref_vDestNormal)
{
  ezVec3 tempNormal;
  EZ_SUCCEED_OR_RETURN(DecodeToVec3(source, sourceFormat, tempNormal));
  ref_vDestNormal = tempNormal * 2.0f - ezVec3(1.0f);
  return EZ_SUCCESS;
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeTangent(
  ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec3& ref_vDestTangent, float& ref_fDestBiTangentSign)
{
  ezVec4 tempTangent;
  EZ_SUCCEED_OR_RETURN(DecodeToVec4(source, sourceFormat, tempTangent));
  ref_vDestTangent = tempTangent.GetAsVec3() * 2.0f - ezVec3(1.0f);
  ref_fDestBiTangentSign = tempTangent.w * 2.0f - 1.0f;
  return EZ_SUCCESS;
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeTexCoord(
  ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec2& ref_vDestTexCoord)
{
  return DecodeToVec2(source, sourceFormat, ref_vDestTexCoord);
}
