#pragma once

#include <RmlUiPlugin/RmlUiConversionUtils.h>

namespace ezRmlUiConversionUtils
{
  ezVariant ToVariant(const Rml::Variant& value)
  {
    return ezVariant();
  }

  Rml::Variant ToVariant(const ezVariant& value)
  {
    switch (value.GetType())
    {
      case ezVariant::Type::Bool:
        return Rml::Variant(value.Get<bool>());

      case ezVariant::Type::String:
        return Rml::Variant(value.Get<ezString>());

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        return Rml::Variant();
    }
  }

} // namespace ezRmlUiConversionUtils
