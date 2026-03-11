

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeNormal(const ezVec3& vNormal, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat)
{
  // we store normals in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec3(vNormal * 0.5f + ezVec3(0.5f), dest, destFormat);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeTangent(const ezVec3& vTangent, float fTangentSign, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat)
{
  // make sure biTangentSign is either -1 or 1
  fTangentSign = (fTangentSign < 0.0f) ? -1.0f : 1.0f;

  // we store tangents in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec4(vTangent.GetAsVec4(fTangentSign) * 0.5f + ezVec4(0.5f), dest, destFormat);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeTexCoord(const ezVec2& vTexCoord, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat)
{
  return EncodeFromVec2(vTexCoord, dest, destFormat);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeBoneWeights(const ezVec4& vWeights, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat)
{
  return EncodeFromVec4(vWeights, dest, destFormat);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::EncodeColor(const ezColor& color, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat, ezMeshVertexColorConversion::Enum conversion)
{
  ezVec4 finalColor = color.GetAsVec4();
  if (conversion == ezMeshVertexColorConversion::LinearToSrgb)
  {
    finalColor = ezColor::LinearToGamma(finalColor.GetAsVec3()).GetAsVec4(finalColor.w);
  }
  else if (conversion == ezMeshVertexColorConversion::SrgbToLinear)
  {
    finalColor = ezColor::GammaToLinear(finalColor.GetAsVec3()).GetAsVec4(finalColor.w);
  }

  return EncodeFromVec4(finalColor, dest, destFormat);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeNormal(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec3& out_vDestNormal)
{
  ezVec3 tempNormal;
  EZ_SUCCEED_OR_RETURN(DecodeToVec3(source, sourceFormat, tempNormal));
  out_vDestNormal = tempNormal * 2.0f - ezVec3(1.0f);
  return EZ_SUCCESS;
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeTangent(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec3& out_vDestTangent, float& out_fDestBiTangentSign)
{
  ezVec4 tempTangent;
  EZ_SUCCEED_OR_RETURN(DecodeToVec4(source, sourceFormat, tempTangent));
  out_vDestTangent = tempTangent.GetAsVec3() * 2.0f - ezVec3(1.0f);
  out_fDestBiTangentSign = tempTangent.w * 2.0f - 1.0f;
  return EZ_SUCCESS;
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeTexCoord(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec2& out_vDestTexCoord)
{
  return DecodeToVec2(source, sourceFormat, out_vDestTexCoord);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeBoneWeights(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec4& out_vDestWeights)
{
  return DecodeToVec4(source, sourceFormat, out_vDestWeights);
}

// static
EZ_ALWAYS_INLINE ezResult ezMeshBufferUtils::DecodeColor(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezColor& out_destColor)
{
  ezVec4 res;
  EZ_SUCCEED_OR_RETURN(DecodeToVec4(source, sourceFormat, res));
  out_destColor = ezColor(res.x, res.y, res.z, res.w);
  return EZ_SUCCESS;
}
