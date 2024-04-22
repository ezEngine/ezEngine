
#pragma once

#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

struct ezMeshBufferResourceDescriptor;

struct ezMeshNormalPrecision
{
  using StorageType = ezUInt8;

  enum Enum
  {
    _10Bit,
    _16Bit,
    _32Bit,

    Default = _10Bit
  };

  /// \brief Convert mesh normal precision to actual resource format used for normals
  static ezGALResourceFormat::Enum ToResourceFormatNormal(Enum value);

  /// \brief Convert mesh normal precision to actual resource format used for tangents
  static ezGALResourceFormat::Enum ToResourceFormatTangent(Enum value);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezMeshNormalPrecision);

struct ezMeshTexCoordPrecision
{
  using StorageType = ezUInt8;

  enum Enum
  {
    _16Bit,
    _32Bit,

    Default = _16Bit
  };

  /// \brief Convert mesh texcoord precision to actual resource format
  static ezGALResourceFormat::Enum ToResourceFormat(Enum value);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezMeshTexCoordPrecision);

struct ezMeshBoneWeigthPrecision
{
  using StorageType = ezUInt8;

  enum Enum
  {
    _8Bit,
    _10Bit,
    _16Bit,
    _32Bit,

    Default = _8Bit
  };

  /// \brief Convert mesh texcoord precision to actual resource format
  static ezGALResourceFormat::Enum ToResourceFormat(Enum value);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezMeshBoneWeigthPrecision);

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
  static ezResult EncodeNormal(const ezVec3& vNormal, ezArrayPtr<ezUInt8> dest, ezMeshNormalPrecision::Enum normalPrecision);
  static ezResult EncodeTangent(const ezVec3& vTangent, float fTangentSign, ezArrayPtr<ezUInt8> dest, ezMeshNormalPrecision::Enum tangentPrecision);
  static ezResult EncodeTexCoord(const ezVec2& vTexCoord, ezArrayPtr<ezUInt8> dest, ezMeshTexCoordPrecision::Enum texCoordPrecision);
  static ezResult EncodeBoneWeights(const ezVec4& vWeights, ezArrayPtr<ezUInt8> dest, ezMeshBoneWeigthPrecision::Enum precision);
  static ezResult EncodeColor(const ezVec4& vColor, ezArrayPtr<ezUInt8> dest, ezMeshVertexColorConversion::Enum conversion);

  static ezResult EncodeNormal(const ezVec3& vNormal, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeTangent(const ezVec3& vTangent, float fTangentSign, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeTexCoord(const ezVec2& vTexCoord, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeBoneWeights(const ezVec4& vWeights, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);

  static ezResult DecodeNormal(ezArrayPtr<const ezUInt8> source, ezVec3& ref_vDestNormal, ezMeshNormalPrecision::Enum normalPrecision);
  static ezResult DecodeTangent(
    ezArrayPtr<const ezUInt8> source, ezVec3& ref_vDestTangent, float& ref_fDestBiTangentSign, ezMeshNormalPrecision::Enum tangentPrecision);
  static ezResult DecodeTexCoord(ezArrayPtr<const ezUInt8> source, ezVec2& ref_vDestTexCoord, ezMeshTexCoordPrecision::Enum texCoordPrecision);

  static ezResult DecodeNormal(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec3& ref_vDestNormal);
  static ezResult DecodeTangent(
    ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec3& ref_vDestTangent, float& ref_fDestBiTangentSign);
  static ezResult DecodeTexCoord(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec2& ref_vDestTexCoord);

  // low level conversion functions
  static ezResult EncodeFromFloat(const float fSource, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeFromVec2(const ezVec2& vSource, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeFromVec3(const ezVec3& vSource, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeFromVec4(const ezVec4& vSource, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);

  static ezResult DecodeToFloat(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, float& ref_fDest);
  static ezResult DecodeToVec2(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec2& ref_vDest);
  static ezResult DecodeToVec3(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec3& ref_vDest);
  static ezResult DecodeToVec4(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec4& ref_vDest);

  /// \brief Helper function to get the position stream from the given mesh buffer descriptor
  static ezResult GetPositionStream(const ezMeshBufferResourceDescriptor& meshBufferDesc, const ezVec3*& out_pPositions, ezUInt32& out_uiElementStride);

  /// \brief Helper function to get the position and normal stream from the given mesh buffer descriptor
  static ezResult GetPositionAndNormalStream(const ezMeshBufferResourceDescriptor& meshBufferDesc, const ezVec3*& out_pPositions, const ezUInt8*& out_pNormals, ezGALResourceFormat::Enum& out_normalFormat, ezUInt32& out_uiElementStride);
};

#include <RendererCore/Meshes/Implementation/MeshBufferUtils_inl.h>
