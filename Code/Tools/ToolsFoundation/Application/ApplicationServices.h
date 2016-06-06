#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Strings/String.h>

class ezDocument;

class EZ_TOOLSFOUNDATION_DLL ezApplicationServices
{
  EZ_DECLARE_SINGLETON(ezApplicationServices);

public:

  ezApplicationServices();

  void SetApplicationName(const char* szName);

  const char* GetApplicationName() const;

  /// \brief A writable folder in which application specific user data may be stored
  ezString GetApplicationUserDataFolder() const;

  /// \brief A read-only folder in which application specific data may be located
  ezString GetApplicationDataFolder() const;

  /// \brief The writable location where the application should store preferences (user specific settings)
  ezString GetApplicationPreferencesFolder() const;

  /// \brief The writable location where preferences for the current ezToolsProject should be stored (user specific settings)
  ezString GetProjectPreferencesFolder() const;

  /// \brief The writable location where preferences for the given ezDocument should be stored (user specific settings)
  ezString GetDocumentPreferencesFolder(const ezDocument* pDocument) const;

  /// \brief The read-only folder where pre-compiled binaries for external tools can be found
  ezString GetPrecompiledToolsFolder(bool bUsePrecompiledTools) const;



private:
  ezString m_sApplicationName;

};