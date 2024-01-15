#pragma once

#include <Core/Utils/CustomData.h>

class CustomDataSample : public ezCustomData
{
  EZ_ADD_DYNAMIC_REFLECTION(CustomDataSample, ezCustomData);

public:

  ezString m_sText;
  ezInt32 m_iSize = 42;
  ezColor m_Color;
};

EZ_DECLARE_CUSTOM_DATA_RESOURCE(CustomDataSample);
