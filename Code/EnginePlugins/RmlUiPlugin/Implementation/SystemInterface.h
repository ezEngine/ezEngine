#pragma once

#include <RmlUi/Core/SystemInterface.h>

namespace ezRmlUiInternal
{
  class SystemInterface final : public Rml::SystemInterface
  {
  public:
    virtual double GetElapsedTime() override;

    virtual void JoinPath(Rml::String& translated_path, const Rml::String& document_path, const Rml::String& path) override;

    virtual bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;
  };
} // namespace ezRmlUiInternal
