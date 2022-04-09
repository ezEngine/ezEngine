#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Log.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezScriptExtensionClass_Log, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Info, In, "Text", In, "Params")->AddAttributes(new ezDynamicPinAttribute("Params")),
    EZ_SCRIPT_FUNCTION_PROPERTY(Warning, In, "Text", In, "Params")->AddAttributes(new ezDynamicPinAttribute("Params")),
    EZ_SCRIPT_FUNCTION_PROPERTY(Error, In, "Text", In, "Params")->AddAttributes(new ezDynamicPinAttribute("Params")),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezScriptExtensionAttribute("Log"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

const char* BuildFormattedText(const char* szText, const ezVariantArray& params, ezStringBuilder& ref_sStorage)
{
  ezHybridArray<ezString, 12> stringStorage;
  stringStorage.Reserve(params.GetCount());
  for (auto& param : params)
  {
    stringStorage.PushBack(param.ConvertTo<ezString>());
  }

  ezHybridArray<ezStringView, 12> stringViews;
  stringViews.Reserve(stringStorage.GetCount());
  for (auto& s : stringStorage)
  {
    stringViews.PushBack(s);
  }

  ezFormatString fs(szText);
  return fs.BuildFormattedText(ref_sStorage, stringViews.GetData(), stringViews.GetCount());
}

// static
void ezScriptExtensionClass_Log::Info(const char* szText, const ezVariantArray& params)
{
  ezStringBuilder sStorage;
  ezLog::Info(BuildFormattedText(szText, params, sStorage));
}

// static
void ezScriptExtensionClass_Log::Warning(const char* szText, const ezVariantArray& params)
{
  ezStringBuilder sStorage;
  ezLog::Warning(BuildFormattedText(szText, params, sStorage));
}

// static
void ezScriptExtensionClass_Log::Error(const char* szText, const ezVariantArray& params)
{
  ezStringBuilder sStorage;
  ezLog::Error(BuildFormattedText(szText, params, sStorage));
}
