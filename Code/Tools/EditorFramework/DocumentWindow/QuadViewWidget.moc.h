#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Strings/String.h>
#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <QWidget>

class ezAssetDocument;
class ezQtEngineDocumentWindow;
class QGridLayout;
class ezQtViewWidgetContainer;
class ezQtEngineViewWidget;
struct ezEngineViewConfig;
struct ezEngineViewPreferences;

class EZ_EDITORFRAMEWORK_DLL ezQtQuadViewWidget : public QWidget
{
  Q_OBJECT
public:
  typedef ezDelegate<ezQtEngineViewWidget*(ezQtEngineDocumentWindow*, ezEngineViewConfig*)> ViewFactory;
  ezQtQuadViewWidget(ezAssetDocument* pDocument, ezQtEngineDocumentWindow* pWindow,
    ViewFactory viewFactory, const char* szViewToolBarMapping);
  ~ezQtQuadViewWidget();

public Q_SLOTS:
  void ToggleViews(QWidget* pView);

protected:
  void SaveViewConfig(const ezEngineViewConfig& cfg, ezEngineViewPreferences& pref) const;
  void LoadViewConfig(ezEngineViewConfig& cfg, ezEngineViewPreferences& pref);
  void SaveViewConfigs() const;
  void LoadViewConfigs();
  void CreateViews(bool bQuad);

private:
  ezAssetDocument* m_pDocument;
  ezQtEngineDocumentWindow* m_pWindow;
  ViewFactory m_ViewFactory;
  ezString m_sViewToolBarMapping;

  ezEngineViewConfig m_ViewConfigSingle;
  ezEngineViewConfig m_ViewConfigQuad[4];
  ezHybridArray<ezQtViewWidgetContainer*, 4> m_ActiveMainViews;
  QGridLayout* m_pViewLayout;
};

