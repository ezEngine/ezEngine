#include <RmlUiPluginDLL.h>

#include <Foundation/Time/Clock.h>
#include <RmlUiPlugin/Implementation/SystemInterface.h>

namespace ezRmlUiInternal
{
  double SystemInterface::GetElapsedTime()
  {
    return ezClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds();
  }

  bool SystemInterface::LogMessage(Rml::Core::Log::Type type, const Rml::Core::String& message)
  {
    switch (type)
    {
      case Rml::Core::Log::LT_ALWAYS:
      case Rml::Core::Log::LT_ERROR:
        ezLog::Error("{}", message.c_str());
        break;

      case Rml::Core::Log::LT_ASSERT:
        EZ_REPORT_FAILURE(message.c_str());
        break;

      case Rml::Core::Log::LT_WARNING:
        ezLog::Warning("{}", message.c_str());
        break;

      case Rml::Core::Log::LT_INFO:
        ezLog::Info("{}", message.c_str());
        break;

      case Rml::Core::Log::LT_DEBUG:
        ezLog::Debug("{}", message.c_str());
        break;
    }

    return true;
  }

} // namespace ezRmlUiInternal
