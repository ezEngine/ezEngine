#pragma once

#include <ToolsFoundation/Basics.h>

class ezIReflectedTypeAccessor;
class ezObjectSerializationContext;

class EZ_TOOLSFOUNDATION_DLL ezObjectSerializationHelper
{
public:
  static void WriteTypeAccessorToContext(const ezIReflectedTypeAccessor& accessor, ezObjectSerializationContext& context);
};
