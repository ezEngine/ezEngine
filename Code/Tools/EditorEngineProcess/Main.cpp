#include <EditorEngineProcessPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

  #include <EditorEngineProcess/EngineProcGameAppUWP.h>
  EZ_APPLICATION_ENTRY_POINT(ezEngineProcessGameApplicationUWP);

#else

  #include <EditorEngineProcess/EngineProcGameApp.h>
  EZ_APPLICATION_ENTRY_POINT(ezEngineProcessGameApplication);

#endif

