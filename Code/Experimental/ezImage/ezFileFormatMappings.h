#pragma once

#include <Foundation/Basics.h>

#include <ezImageFormat.h>

class ezFileFormatMappings
{
  static ezUInt32 ToDxgiFormat(ezImageFormat::Enum format);
  static ezImageFormat::Enum FromDxgiFormat(ezUInt32 uiDxgiFormat);
};