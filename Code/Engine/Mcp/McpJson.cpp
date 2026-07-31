#include <Mcp/McpPCH.h>

#include <Mcp/McpJson.h>

#include <Foundation/Utilities/ConversionUtils.h>

ezStringView ezMcpJson::GetString(const ezVariantDictionary& dict, ezStringView sKey, ezStringView sFallback)
{
  const ezVariant* pValue = nullptr;

  if (dict.TryGetValue(sKey, pValue) && pValue->IsA<ezString>())
  {
    return pValue->Get<ezString>().GetView();
  }

  return sFallback;
}

ezInt64 ezMcpJson::GetInt(const ezVariantDictionary& dict, ezStringView sKey, ezInt64 iFallback)
{
  const ezVariant* pValue = nullptr;

  if (!dict.TryGetValue(sKey, pValue) || !pValue->IsValid())
    return iFallback;

  if (pValue->IsNumber())
    return static_cast<ezInt64>(pValue->ConvertTo<double>());

  if (pValue->IsA<ezString>())
  {
    ezInt64 iResult = 0;
    if (ezConversionUtils::StringToInt64(pValue->Get<ezString>(), iResult).Succeeded())
      return iResult;
  }

  return iFallback;
}

bool ezMcpJson::GetBool(const ezVariantDictionary& dict, ezStringView sKey, bool bFallback)
{
  const ezVariant* pValue = nullptr;

  if (!dict.TryGetValue(sKey, pValue) || !pValue->IsValid())
    return bFallback;

  if (pValue->IsA<bool>())
    return pValue->Get<bool>();

  if (pValue->IsNumber())
    return pValue->ConvertTo<double>() != 0.0;

  if (pValue->IsA<ezString>())
  {
    bool bResult = false;
    if (ezConversionUtils::StringToBool(pValue->Get<ezString>(), bResult) == EZ_SUCCESS)
      return bResult;
  }

  return bFallback;
}

bool ezMcpJson::GetStringArray(const ezVariantDictionary& dict, ezStringView sKey, ezDynamicArray<ezString>& out_values)
{
  const ezVariant* pValue = nullptr;

  if (!dict.TryGetValue(sKey, pValue) || !pValue->IsValid())
    return false;

  if (pValue->IsA<ezString>())
  {
    out_values.PushBack(pValue->Get<ezString>());
    return true;
  }

  if (!pValue->IsA<ezVariantArray>())
    return false;

  for (const ezVariant& element : pValue->Get<ezVariantArray>())
  {
    if (element.CanConvertTo<ezString>())
      out_values.PushBack(element.ConvertTo<ezString>());
  }

  return true;
}

const ezVariantDictionary* ezMcpJson::GetDict(const ezVariantDictionary& dict, ezStringView sKey)
{
  const ezVariant* pValue = nullptr;

  if (dict.TryGetValue(sKey, pValue) && pValue->IsA<ezVariantDictionary>())
  {
    return &pValue->Get<ezVariantDictionary>();
  }

  return nullptr;
}

const ezVariantArray* ezMcpJson::GetArray(const ezVariantDictionary& dict, ezStringView sKey)
{
  const ezVariant* pValue = nullptr;

  if (dict.TryGetValue(sKey, pValue) && pValue->IsA<ezVariantArray>())
  {
    return &pValue->Get<ezVariantArray>();
  }

  return nullptr;
}
