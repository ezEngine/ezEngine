#pragma once

#include <TexConvLib/Basics.h>
#include <TexConvLib/Configuration/TexConvDesc.h>

/// \brief Runtime state for texture conversion
struct ezTexConvState
{
  ezTexConvDesc m_Descriptor;
  ezEnum<ezImageFormat> m_OutputImageFormat;

  void AdjustTargetFormat();
  void ChooseOutputFormat();
};
