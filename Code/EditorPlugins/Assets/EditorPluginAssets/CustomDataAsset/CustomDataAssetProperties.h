#pragma once

#include <Core/Utils/CustomData.h>


class ezCustomDataAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCustomDataAssetProperties, ezReflectedClass);
public:
  ezCustomData* m_pType;
};

