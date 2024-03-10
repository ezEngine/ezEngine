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

static ezStringView BuildFormattedText(ezStringView sText, const ezVariantArray& params, ezStringBuilder& ref_sStorage)
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

  ezFormatString fs(sText);
  return fs.BuildFormattedText(ref_sStorage, stringViews.GetData(), stringViews.GetCount());
}

// static
void ezScriptExtensionClass_Log::Info(ezStringView sText, const ezVariantArray& params)
{
  ezStringBuilder sStorage;
  ezLog::Info(BuildFormattedText(sText, params, sStorage));
}

// static
void ezScriptExtensionClass_Log::Warning(ezStringView sText, const ezVariantArray& params)
{
  ezStringBuilder sStorage;
  ezLog::Warning(BuildFormattedText(sText, params, sStorage));
}

// static
void ezScriptExtensionClass_Log::Error(ezStringView sText, const ezVariantArray& params)
{
  ezStringBuilder sStorage;
  ezLog::Error(BuildFormattedText(sText, params, sStorage));
}


EZ_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptExtensionClass_Log);
