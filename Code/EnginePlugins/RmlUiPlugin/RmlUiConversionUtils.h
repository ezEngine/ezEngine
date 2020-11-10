#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

namespace ezRmlUiConversionUtils
{
  EZ_RMLUIPLUGIN_DLL ezVariant ToVariant(const Rml::Variant& value, ezVariant::Type::Enum targetType = ezVariant::Type::Invalid);
  EZ_RMLUIPLUGIN_DLL Rml::Variant ToVariant(const ezVariant& value);
}
