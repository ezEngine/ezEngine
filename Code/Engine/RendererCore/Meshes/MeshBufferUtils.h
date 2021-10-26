
#pragma once

#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

struct ezMeshBufferResourceDescriptor;

struct ezMeshNormalPrecision
{
  typedef ezUInt8 StorageType;

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
  typedef ezUInt8 StorageType;

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

struct EZ_RENDERERCORE_DLL ezMeshBufferUtils
{
  static ezResult EncodeNormal(const ezVec3& normal, ezArrayPtr<ezUInt8> dest, ezMeshNormalPrecision::Enum normalPrecision);
  static ezResult EncodeTangent(const ezVec3& tangent, float biTangentSign, ezArrayPtr<ezUInt8> dest, ezMeshNormalPrecision::Enum tangentPrecision);
  static ezResult EncodeTexCoord(const ezVec2& texCoord, ezArrayPtr<ezUInt8> dest, ezMeshTexCoordPrecision::Enum texCoordPrecision);

  static ezResult EncodeNormal(const ezVec3& normal, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeTangent(const ezVec3& tangent, float biTangentSign, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeTexCoord(const ezVec2& texCoord, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);

  static ezResult DecodeNormal(ezArrayPtr<const ezUInt8> source, ezVec3& destNormal, ezMeshNormalPrecision::Enum normalPrecision);
  static ezResult DecodeTangent(
    ezArrayPtr<const ezUInt8> source, ezVec3& destTangent, float& destBiTangentSign, ezMeshNormalPrecision::Enum tangentPrecision);
  static ezResult DecodeTexCoord(ezArrayPtr<const ezUInt8> source, ezVec2& destTexCoord, ezMeshTexCoordPrecision::Enum texCoordPrecision);

  static ezResult DecodeNormal(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec3& destNormal);
  static ezResult DecodeTangent(
    ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec3& destTangent, float& destBiTangentSign);
  static ezResult DecodeTexCoord(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec2& destTexCoord);

  // low level conversion functions
  static ezResult EncodeFromFloat(const float source, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeFromVec2(const ezVec2& source, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeFromVec3(const ezVec3& source, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);
  static ezResult EncodeFromVec4(const ezVec4& source, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);

  static ezResult DecodeToFloat(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, float& dest);
  static ezResult DecodeToVec2(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec2& dest);
  static ezResult DecodeToVec3(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec3& dest);
  static ezResult DecodeToVec4(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec4& dest);

  /// \brief Helper function to get the position stream from the given mesh buffer descriptor
  static ezResult GetPositionStream(const ezMeshBufferResourceDescriptor& meshBufferDesc, const ezVec3*& out_pPositions, ezUInt32& out_uiElementStride);

  /// \brief Helper function to get the position and normal stream from the given mesh buffer descriptor
  static ezResult GetPositionAndNormalStream(const ezMeshBufferResourceDescriptor& meshBufferDesc, const ezVec3*& out_pPositions, const ezUInt8*& out_pNormals, ezGALResourceFormat::Enum& out_NormalFormat, ezUInt32& out_uiElementStride);
};

#include <RendererCore/Meshes/Implementation/MeshBufferUtils_inl.h>
