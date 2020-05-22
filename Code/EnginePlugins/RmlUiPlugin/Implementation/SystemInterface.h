#pragma once

#include <RmlUi/Core/SystemInterface.h>

namespace ezRmlUiInternal
{
  class SystemInterface : public Rml::Core::SystemInterface
  {
  public:
    virtual double GetElapsedTime() override;

    virtual void JoinPath(Rml::Core::String& translated_path, const Rml::Core::String& document_path, const Rml::Core::String& path) override;

    virtual bool LogMessage(Rml::Core::Log::Type type, const Rml::Core::String& message) override;
  };
} // namespace ezRmlUiInternal
