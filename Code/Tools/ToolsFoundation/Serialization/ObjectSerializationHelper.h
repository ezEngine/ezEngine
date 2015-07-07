#pragma once

#include <ToolsFoundation/Basics.h>

class ezIReflectedTypeAccessor;
class ezPropertySerializationContext;

class EZ_TOOLSFOUNDATION_DLL ezObjectSerializationHelper
{
public:
  static void WriteTypeAccessorToContext(const ezIReflectedTypeAccessor& accessor, ezPropertySerializationContext& context);
};
