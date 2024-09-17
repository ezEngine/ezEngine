#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WEB)

#  include <Foundation/Platform/Web/EmscriptenUtils.h>

#  include <emscripten/emscripten.h>

void ezEmscriptenUtils::SetMainLoopFunction(ezEmscriptenMainLoopFunc func)
{
  emscripten_set_main_loop(func, 0, true);
}

#endif
