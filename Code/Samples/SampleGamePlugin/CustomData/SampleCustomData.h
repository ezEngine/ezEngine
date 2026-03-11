#pragma once

#include <Core/Utils/CustomData.h>

// BEGIN-DOCS-CODE-SNIPPET: customdata-decl
class SampleCustomData : public ezCustomData
{
  EZ_ADD_DYNAMIC_REFLECTION(SampleCustomData, ezCustomData);

public:
  ezString m_sText;
  ezInt32 m_iSize = 42;
  ezColor m_Color;
};

EZ_DECLARE_CUSTOM_DATA_RESOURCE(SampleCustomData);
// END-DOCS-CODE-SNIPPET
