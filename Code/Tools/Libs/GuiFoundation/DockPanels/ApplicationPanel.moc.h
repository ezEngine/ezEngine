#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ads/DockWidget.h>
#include <Foundation/Reflection/Reflection.h>

class ezQtContainerWindow;

/// \brief Base class for all panels that are supposed to be application wide (not tied to some document).
class EZ_GUIFOUNDATION_DLL ezQtApplicationPanel : public ads::CDockWidget
{
public:
  Q_OBJECT

public:
  ezQtApplicationPanel(const char* szPanelName);
  ~ezQtApplicationPanel();

  void EnsureVisible();

  static const ezDynamicArray<ezQtApplicationPanel*>& GetAllApplicationPanels() { return s_AllApplicationPanels; }

protected:
  virtual void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

private:
  friend class ezQtContainerWindow;

  static ezDynamicArray<ezQtApplicationPanel*> s_AllApplicationPanels;

  ezQtContainerWindow* m_pContainerWindow;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_GUIFOUNDATION_DLL, ezQtApplicationPanel);
