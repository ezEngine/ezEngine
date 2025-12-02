#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

class ezRecentFilesList;

/// Helper class for managing Windows taskbar jump lists
class EZ_EDITORFRAMEWORK_DLL ezWindowsJumpList
{
public:
  /// Updates the Windows taskbar jump list with recent projects
  static void UpdateJumpList(const ezRecentFilesList& recentProjects);
};

#endif
