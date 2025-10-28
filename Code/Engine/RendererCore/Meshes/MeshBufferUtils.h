
#pragma once

#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

/// Color space conversion modes for vertex colors.
struct ezMeshVertexColorConversion
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None,         ///< No conversion applied.
    LinearToSrgb, ///< Convert from linear to sRGB color space.
    SrgbToLinear, ///< Convert from sRGB to linear color space.

    Default = None
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezMeshVertexColorConversion);

/// Utility functions for encoding and decoding mesh vertex attributes.
///
/// Provides conversion between high-level types (ezVec3, ezColor, etc.) and various
/// GPU buffer formats. Handles normals, tangents, texture coordinates, bone weights,
/// and vertex colors with optional color space conversion.
struct EZ_RENDERERCORE_DLL ezMeshBufferUtils
{
  /// Encodes a normal vector to the specified GPU buffer format.
  static ezResult EncodeNormal(const ezVec3& vNormal, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat);

  /// Encodes a tangent vector and bitangent sign to the specified GPU buffer format.
  ///
  /// The tangent sign is typically used to reconstruct the bitangent in the shader.
  static ezResult EncodeTangent(const ezVec3& vTangent, float fTangentSign, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat);

  /// Encodes texture coordinates to the specified GPU buffer format.
  static ezResult EncodeTexCoord(const ezVec2& vTexCoord, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat);

  /// Encodes bone weights for skinned meshes to the specified GPU buffer format.
  static ezResult EncodeBoneWeights(const ezVec4& vWeights, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat);

  /// Encodes a vertex color with optional color space conversion.
  static ezResult EncodeColor(const ezColor& color, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat, ezMeshVertexColorConversion::Enum conversion);


  /// Decodes a normal vector from the specified GPU buffer format.
  static ezResult DecodeNormal(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec3& out_vDestNormal);

  /// Decodes a tangent vector and bitangent sign from the specified GPU buffer format.
  static ezResult DecodeTangent(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec3& out_vDestTangent, float& out_fDestBiTangentSign);

  /// Decodes texture coordinates from the specified GPU buffer format.
  static ezResult DecodeTexCoord(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec2& out_vDestTexCoord);

  /// Decodes bone weights from the specified GPU buffer format.
  static ezResult DecodeBoneWeights(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec4& out_vDestWeights);

  /// Decodes a vertex color from the specified GPU buffer format.
  static ezResult DecodeColor(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezColor& out_destColor);


  /// Low-level function to encode a float value to the specified GPU buffer format.
  static ezResult EncodeFromFloat(const float fSource, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat);

  /// Low-level function to encode a 2D vector to the specified GPU buffer format.
  static ezResult EncodeFromVec2(const ezVec2& vSource, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat);

  /// Low-level function to encode a 3D vector to the specified GPU buffer format.
  static ezResult EncodeFromVec3(const ezVec3& vSource, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat);

  /// Low-level function to encode a 4D vector to the specified GPU buffer format.
  static ezResult EncodeFromVec4(const ezVec4& vSource, ezByteArrayPtr dest, ezGALResourceFormat::Enum destFormat);

  /// Low-level function to decode a float value from the specified GPU buffer format.
  static ezResult DecodeToFloat(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, float& out_fDest);


  /// Low-level function to decode a 2D vector from the specified GPU buffer format.
  static ezResult DecodeToVec2(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec2& out_vDest);

  /// Low-level function to decode a 3D vector from the specified GPU buffer format.
  static ezResult DecodeToVec3(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec3& out_vDest);

  /// Low-level function to decode a 4D vector from the specified GPU buffer format.
  static ezResult DecodeToVec4(ezConstByteArrayPtr source, ezGALResourceFormat::Enum sourceFormat, ezVec4& out_vDest);
};

#include <RendererCore/Meshes/Implementation/MeshBufferUtils_inl.h>
