#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Implementation/SystemInterface.h>
#include <RmlUiPlugin/RmlUiConversionUtils.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Time/Clock.h>

namespace ezRmlUiInternal
{
  double SystemInterface::GetElapsedTime()
  {
    return ezClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds();
  }

  int SystemInterface::TranslateString(Rml::String& translated, const Rml::String& input)
  {
    ezStringView sInput = ezRmlUiConversionUtils::ToStringView(input);
    sInput.Trim(" \t\r\n");
    if (sInput.IsEmpty() == false)
    {
      translated = ezRmlUiConversionUtils::ToString(ezTranslate(sInput));
      return 1;
    }

    translated = input;
    return 0;
  }

  void SystemInterface::JoinPath(Rml::String& ref_sTranslated_path, const Rml::String& sDocument_path, const Rml::String& sPath)
  {
    if (ezFileSystem::ExistsFile(ezRmlUiConversionUtils::ToStringView(sPath)))
    {
      // path is already a valid path for ez file system so don't join with document path
      ref_sTranslated_path = sPath;
      return;
    }

    Rml::SystemInterface::JoinPath(ref_sTranslated_path, sDocument_path, sPath);
  }

  bool SystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& sMessage)
  {
    switch (type)
    {
      case Rml::Log::LT_ERROR:
        ezLog::Error("{}", ezRmlUiConversionUtils::ToStringView(sMessage));
        break;

      case Rml::Log::LT_ASSERT:
        EZ_REPORT_FAILURE(sMessage.c_str());
        break;

      case Rml::Log::LT_WARNING:
        ezLog::Warning("{}", ezRmlUiConversionUtils::ToStringView(sMessage));
        break;

      case Rml::Log::LT_ALWAYS:
      case Rml::Log::LT_INFO:
        ezLog::Info("{}", ezRmlUiConversionUtils::ToStringView(sMessage));
        break;

      case Rml::Log::LT_DEBUG:
        ezLog::Debug("{}", ezRmlUiConversionUtils::ToStringView(sMessage));
        break;
      default:
        break;
    }

    return true;
  }

} // namespace ezRmlUiInternal
