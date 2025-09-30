
#pragma once

#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

struct ezMeshVertexColorConversion
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None,
    LinearToSrgb,
    SrgbToLinear,

    Default = None
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezMeshVertexColorConversion);

struct EZ_RENDERERCORE_DLL ezMeshBufferUtils
{
  static ezResult EncodeNormal(const ezVec3& vNormal, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeTangent(const ezVec3& vTangent, float fTangentSign, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeTexCoord(const ezVec2& vTexCoord, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeBoneWeights(const ezVec4& vWeights, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeColor(const ezColor& color, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat, ezMeshVertexColorConversion::Enum conversion);

  static ezResult DecodeNormal(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec3& out_vDestNormal);
  static ezResult DecodeTangent(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec3& out_vDestTangent, float& out_fDestBiTangentSign);
  static ezResult DecodeTexCoord(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec2& out_vDestTexCoord);
  static ezResult DecodeBoneWeights(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec4& out_vDestWeights);
  static ezResult DecodeColor(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezColor& out_destColor);

  // low level conversion functions
  static ezResult EncodeFromFloat(const float fSource, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeFromVec2(const ezVec2& vSource, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeFromVec3(const ezVec3& vSource, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeFromVec4(const ezVec4& vSource, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat);

  static ezResult DecodeToFloat(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, float& out_fDest);
  static ezResult DecodeToVec2(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec2& out_vDest);
  static ezResult DecodeToVec3(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec3& out_vDest);
  static ezResult DecodeToVec4(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec4& out_vDest);
};

#include <RendererCore/Meshes/Implementation/MeshBufferUtils_inl.h>
