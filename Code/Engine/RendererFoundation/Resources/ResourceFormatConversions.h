
#pragma once

#include <RendererFoundation/Resources/ResourceFormats.h>

struct EZ_RENDERERFOUNDATION_DLL ezGALResourceFormatConversions
{
  static ezResult FromFloat1(const float source, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);
  static ezResult FromFloat2(const ezVec2& source, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);
  static ezResult FromFloat3(const ezVec3& source, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);
  static ezResult FromFloat4(const ezVec4& source, ezArrayPtr<ezUInt8> dest, ezGALResourceFormat::Enum destFormat);

  static ezResult ToFloat1(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, float& dest);
  static ezResult ToFloat2(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec2& dest);
  static ezResult ToFloat3(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec3& dest);
  static ezResult ToFloat4(ezArrayPtr<const ezUInt8> source, ezGALResourceFormat::Enum sourceFormat, ezVec4& dest);
};
