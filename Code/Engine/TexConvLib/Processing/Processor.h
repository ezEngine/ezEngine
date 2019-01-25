#pragma once

#include <TexConvLib/Basics.h>
#include <TexConvLib/Configuration/TexConvDesc.h>

class EZ_TEXCONV_DLL ezTexConvProcessor
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezTexConvProcessor);
public:
  ezTexConvProcessor();

  ezTexConvDesc m_Descriptor;

  ezResult Process();

  ezImage m_Output;

private:
  ezEnum<ezImageFormat> m_OutputImageFormat;

  ezResult LoadInputImages();


  ezResult AdjustTargetFormat();
  ezResult ChooseOutputFormat();
};

