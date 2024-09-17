#pragma once

#if EZ_ENABLED(EZ_PLATFORM_WEB)

namespace ezEmscriptenUtils
{
  using ezEmscriptenMainLoopFunc = void (*)();

  /// \brief Calls emscripten_set_main_loop under the hood to replace the main loop callback.
  void SetMainLoopFunction(ezEmscriptenMainLoopFunc func);

} // namespace ezEmscriptenUtils

#endif
