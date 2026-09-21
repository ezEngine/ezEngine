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

    /// While this is true, RmlUi's 'could not locate data model' error is logged as a debug message instead
    /// of an error.
    ///
    /// A document declares the data models it wants to use through 'data-model' attributes, but those models
    /// only exist once a data binding has been added to the context. Since the document has to be loaded
    /// before bindings can be set up, it is expected that the first load refers to models that don't exist
    /// yet. In the editor, which does not run the game code that adds the bindings, that state is permanent.
    /// RmlUi logs an error for it every time and has no way of knowing that it is expected, so the message is
    /// filtered out on this side instead.
    static bool s_bAllowMissingDataModels;
  };
} // namespace ezRmlUiInternal
