#pragma once

#include <RmlUi/Core/SystemInterface.h>

namespace ezRmlUiInternal
{
  class SystemInterface final : public Rml::SystemInterface
  {
  public:
    virtual double GetElapsedTime() override;

    virtual void JoinPath(Rml::String& ref_sTranslated_path, const Rml::String& sDocument_path, const Rml::String& sPath) override;

    virtual bool LogMessage(Rml::Log::Type type, const Rml::String& sMessage) override;
  };
} // namespace ezRmlUiInternal
