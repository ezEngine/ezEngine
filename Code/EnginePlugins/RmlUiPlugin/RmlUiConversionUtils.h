#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Types/Variant.h>
#include <RmlUi/Include/RmlUi/Core.h>

namespace ezRmlUiConversionUtils
{
  EZ_RMLUIPLUGIN_DLL ezVariant ToVariant(const Rml::Variant& value, ezVariant::Type::Enum targetType = ezVariant::Type::Invalid);
  EZ_RMLUIPLUGIN_DLL Rml::Variant ToVariant(const ezVariant& value);

  EZ_ALWAYS_INLINE ezString ToString(const Rml::String& value)
  {
    return ezStringView(value.c_str(), static_cast<ezUInt32>(value.length()));
  }

  EZ_ALWAYS_INLINE ezString ToString(const Rml::StringView& value)
  {
    return ezStringView(value.begin(), static_cast<ezUInt32>(value.size()));
  }

  EZ_ALWAYS_INLINE Rml::String ToString(const ezString& sValue)
  {
    return Rml::String(sValue.GetData(), sValue.GetElementCount());
  }

  EZ_ALWAYS_INLINE Rml::String ToString(ezStringView sValue)
  {
    return Rml::String(sValue.GetStartPointer(), sValue.GetElementCount());
  }

  EZ_ALWAYS_INLINE ezStringView ToStringView(const Rml::StringView& value)
  {
    return ezStringView(value.begin(), static_cast<ezUInt32>(value.size()));
  }

  EZ_ALWAYS_INLINE Rml::StringView ToStringView(ezStringView sValue)
  {
    return Rml::StringView(sValue.GetStartPointer(), sValue.GetElementCount());
  }

} // namespace ezRmlUiConversionUtils
