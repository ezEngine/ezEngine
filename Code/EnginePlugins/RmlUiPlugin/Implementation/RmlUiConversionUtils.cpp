#pragma once

#include <RmlUiPlugin/RmlUiConversionUtils.h>

namespace ezRmlUiConversionUtils
{
  ezVariant ToVariant(const Rml::Variant& value, ezVariant::Type::Enum targetType /*= ezVariant::Type::Invalid*/)
  {
    ezVariant result;

    switch (value.GetType())
    {
      case Rml::Variant::BOOL:
        result = value.Get<bool>();
        break;

      case Rml::Variant::CHAR:
        result = value.Get<char>();
        break;

      case Rml::Variant::BYTE:
        result = value.Get<Rml::byte>();
        break;

      case Rml::Variant::INT:
        result = value.Get<int>();
        break;

      case Rml::Variant::INT64:
        result = value.Get<ezInt64>();
        break;

      case Rml::Variant::FLOAT:
        result = value.Get<float>();
        break;

      case Rml::Variant::DOUBLE:
        result = value.Get<double>();
        break;

      case Rml::Variant::STRING:
        result = value.Get<Rml::String>().c_str();
        break;
    }

    if (targetType != ezVariant::Type::Invalid && result.IsValid())
    {
      ezResult conversionResult = EZ_SUCCESS;
      result = result.ConvertTo(targetType, &conversionResult);

      if (conversionResult.Failed())
      {
        ezLog::Warning("Failed to convert rml variant to target type '{}'", targetType);
      }
    }

    return result;
  }

  Rml::Variant ToVariant(const ezVariant& value)
  {
    switch (value.GetType())
    {
      case ezVariant::Type::Bool:
        return Rml::Variant(value.Get<bool>());

      case ezVariant::Type::Int8:
        return Rml::Variant(value.Get<ezInt8>());

      case ezVariant::Type::UInt8:
        return Rml::Variant(value.Get<ezUInt8>());

      case ezVariant::Type::Int16:
      case ezVariant::Type::UInt16:
      case ezVariant::Type::Int32:
        return Rml::Variant(value.ConvertTo<int>());

      case ezVariant::Type::UInt32:
      case ezVariant::Type::Int64:
        return Rml::Variant(value.ConvertTo<ezInt64>());

      case ezVariant::Type::Float:
        return Rml::Variant(value.Get<float>());

      case ezVariant::Type::Double:
        return Rml::Variant(value.Get<double>());

      case ezVariant::Type::String:
        return Rml::Variant(value.Get<ezString>());

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        return Rml::Variant();
    }
  }

} // namespace ezRmlUiConversionUtils
