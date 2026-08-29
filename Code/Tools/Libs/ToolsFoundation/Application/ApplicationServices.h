#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class ezDocument;

class EZ_TOOLSFOUNDATION_DLL ezApplicationServices
{
  EZ_DECLARE_SINGLETON(ezApplicationServices);

public:
  ezApplicationServices();

  /// A writable folder in which application specific user data may be stored
  ezString GetApplicationUserDataFolder() const;

  /// A read-only folder in which application specific data may be located
  ezString GetApplicationDataFolder() const;

  /// The writable location where the application should store preferences (user specific settings)
  ezString GetApplicationPreferencesFolder() const;

  /// The writable location where preferences for the current ezToolsProject should be stored (user specific settings)
  ezString GetProjectPreferencesFolder() const;

  ezString GetProjectPreferencesFolder(ezStringView sProjectFilePath) const;

  /// The writable location where preferences for the given ezDocument should be stored (user specific settings)
  ezString GetDocumentPreferencesFolder(const ezDocument* pDocument) const;

  /// The read-only folder where pre-compiled binaries for external tools can be found
  ezString GetPrecompiledToolsFolder(bool bUsePrecompiledTools) const;

  /// The folder under which the sample projects are stored
  ezString GetSampleProjectsFolder() const;
};
