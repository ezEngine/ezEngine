#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Communication/Event.h>

class QProgressDialog;
class QWinTaskbarProgress;
class QWinTaskbarButton;
class ezProgress;
struct ezProgressEvent;

class EZ_GUIFOUNDATION_DLL ezQtProgressbar
{
public:
  ezQtProgressbar();
  ~ezQtProgressbar();

  void SetProgressbar(ezProgress* pProgress);

private:
  void ProgressbarEventHandler(const ezProgressEvent& e);

  void EnsureCreated();
  void EnsureDestroyed();

  QProgressDialog* m_pDialog;
  ezProgress* m_pProgress;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  QWinTaskbarButton* m_pWinTaskBarButton;
  QWinTaskbarProgress* m_pWinTaskBarProgress;
#endif

};



