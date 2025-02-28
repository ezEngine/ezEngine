#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ads/DockWidget.h>

class ezQtContainerWindow;
namespace ads
{
  class CDockManager;
}

/// \brief Base class for all panels that are supposed to be application wide (not tied to some document).
class EZ_GUIFOUNDATION_DLL ezQtApplicationPanel : public ads::CDockWidget
{
public:
  Q_OBJECT

public:
  ezQtApplicationPanel(ads::CDockManager* pDockManager, const char* szPanelName);
  ~ezQtApplicationPanel();

  void EnsureVisible();

  static const ezDynamicArray<ezQtApplicationPanel*>& GetAllApplicationPanels() { return s_AllApplicationPanels; }

protected:
  virtual void ToolsProjectEventHandler(const ezToolsProjectEvent& e);
  virtual bool event(QEvent* event) override;

private:
  friend class ezQtContainerWindow;

  static ezDynamicArray<ezQtApplicationPanel*> s_AllApplicationPanels;

  ezQtContainerWindow* m_pContainerWindow = nullptr;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GUIFOUNDATION_DLL, ezQtApplicationPanel);
