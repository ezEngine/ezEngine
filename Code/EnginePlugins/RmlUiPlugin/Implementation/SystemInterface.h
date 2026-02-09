#pragma once

#include <RmlUi/Core/SystemInterface.h>

namespace ezRmlUiInternal
{
  class SystemInterface final : public Rml::SystemInterface
  {
  public:
    virtual double GetElapsedTime() override;

    virtual int TranslateString(Rml::String& out_sTranslated, const Rml::String& sInput) override;

    virtual void JoinPath(Rml::String& out_sTranslatedPath, const Rml::String& sDocumentPath, const Rml::String& sPath) override;

    virtual bool LogMessage(Rml::Log::Type type, const Rml::String& sMessage) override;
  };
} // namespace ezRmlUiInternal
