#pragma once

#include <TexConvLib/Basics.h>
#include <TexConvLib/Configuration/TexConvDesc.h>

class ezTexConvProcessor
{
public:

  ezResult Process();


private:
  ezTexConvDesc m_Descriptor;
  ezEnum<ezImageFormat> m_OutputImageFormat;

  ezResult LoadInputImages();


  ezResult AdjustTargetFormat();
  ezResult ChooseOutputFormat();
};

