#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class QProgressDialog;
class QWinTaskbarProgress;
class QWinTaskbarButton;
class ezProgress;
struct ezProgressEvent;

#undef USE_WIN_EXTRAS
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  define USE_WIN_EXTRAS EZ_ON
#else
#  define USE_WIN_EXTRAS EZ_OFF
#endif

/// \brief A Qt implementation to display the state of an ezProgress instance.
///
/// Create a single instance of this at application startup and link it to an ezProgress instance.
/// Whenever the instance's progress state changes, this class will display a simple progress bar.
class EZ_GUIFOUNDATION_DLL ezQtProgressbar
{
public:
  ezQtProgressbar();
  ~ezQtProgressbar();

  /// \brief Sets the ezProgress instance that should be visualized.
  void SetProgressbar(ezProgress* pProgress);

private:
  void ProgressbarEventHandler(const ezProgressEvent& e);

  void EnsureCreated();
  void EnsureDestroyed();

  QProgressDialog* m_pDialog = nullptr;
  ezProgress* m_pProgress = nullptr;

#if EZ_ENABLED(USE_WIN_EXTRAS)
  QWinTaskbarButton* m_pWinTaskBarButton = nullptr;
  QWinTaskbarProgress* m_pWinTaskBarProgress = nullptr;
  QMetaObject::Connection m_OnButtonDestroyed;
  QMetaObject::Connection m_OnProgressDestroyed;
#endif
};
