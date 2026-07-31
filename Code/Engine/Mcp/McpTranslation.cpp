#include <Mcp/McpPCH.h>

#include <Mcp/McpJsonWriter.h>
#include <Mcp/McpTranslation.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/TranslationLookup.h>

namespace
{
  /// The echo guard: an unknown key comes back unchanged, which is indistinguishable from a translation
  /// that happens to equal its key - and treating the latter as missing loses nothing, since repeating
  /// the key tells the caller nothing it did not send.
  ///
  /// Missing-translation logging is off for the duration: these tools ask about reflection data wholesale,
  /// where most keys are expected to have no entry, and ezTranslatorLogMissing would fill the editor log
  /// with a warning per property. The property grid disables it for the same reason.
  ezStringView Lookup(ezStringView sKey, ezTranslationUsage usage)
  {
    if (sKey.IsEmpty())
      return {};

    const bool bLogMissing = ezTranslatorLogMissing::s_bActive;
    ezTranslatorLogMissing::s_bActive = false;

    const ezStringView sResult = ezTranslationLookup::Translate(sKey, ezHashingUtils::StringHash(sKey), usage);

    ezTranslatorLogMissing::s_bActive = bLogMissing;

    if (sResult == sKey)
      return {};

    return sResult;
  }

  /// True when the translation is what ezTranslatorMakeMoreReadable derives from the key itself: the
  /// part behind the last '::' with spaces inserted at CamelCase boundaries. That translator is
  /// registered in the editor, so a key with no entry anywhere still comes back as readable text - and
  /// 'Projection Axis' for 'ProjectionAxis' is a token per property that tells the reader nothing it
  /// could not have written down itself. Genuine overrides ('Base Color Texture' for 'BaseColor')
  /// survive the comparison.
  bool IsDerivableFromKey(ezStringView sTranslation, ezStringView sKey)
  {
    const char* szScope = sKey.FindLastSubString("::");

    if (szScope != nullptr)
    {
      sKey.SetStartPosition(szScope + 2);
    }

    ezStringBuilder sStripped = sTranslation;
    sStripped.ReplaceAll(" ", "");

    return sStripped.IsEqual_NoCase(sKey);
  }

  /// Walks up the type hierarchy because the entry sits on the type that declares the property, which for
  /// an inherited property is not the type the caller asked about.
  ezStringView LookupProperty(const ezRTTI* pType, ezStringView sPropertyName, ezTranslationUsage usage)
  {
    if (sPropertyName.IsEmpty())
      return {};

    ezStringBuilder sKey;

    for (const ezRTTI* pCurrent = pType; pCurrent != nullptr; pCurrent = pCurrent->GetParentType())
    {
      sKey.Set(pCurrent->GetTypeName(), "::", sPropertyName);

      const ezStringView sResult = Lookup(sKey, usage);

      // A readable-ified key is not an entry: it is produced for every key, so accepting it here would
      // stop the walk on the first type and never reach the base type that declares the property.
      if (!sResult.IsEmpty() && !IsDerivableFromKey(sResult, sKey))
        return sResult;
    }

    return {};
  }
} // namespace

ezStringView ezMcpTranslation::GetDisplayName(ezStringView sKey)
{
  const ezStringView sResult = Lookup(sKey, ezTranslationUsage::Default);

  if (IsDerivableFromKey(sResult, sKey))
    return {};

  return sResult;
}

ezStringView ezMcpTranslation::GetTooltip(ezStringView sKey)
{
  return Lookup(sKey, ezTranslationUsage::Tooltip);
}

ezStringView ezMcpTranslation::GetHelpURL(ezStringView sKey)
{
  return Lookup(sKey, ezTranslationUsage::HelpURL);
}

ezStringView ezMcpTranslation::GetPropertyDisplayName(const ezRTTI* pType, ezStringView sPropertyName)
{
  return LookupProperty(pType, sPropertyName, ezTranslationUsage::Default);
}

ezStringView ezMcpTranslation::GetPropertyTooltip(const ezRTTI* pType, ezStringView sPropertyName)
{
  return LookupProperty(pType, sPropertyName, ezTranslationUsage::Tooltip);
}

void ezMcpTranslation::AddOptionalString(ezMcpJsonWriter& ref_writer, ezStringView sFieldName, ezStringView sValue)
{
  if (sValue.IsEmpty())
    return;

  ref_writer.AddVariableString(sFieldName, sValue);
}
