#pragma once

#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <Foundation/Basics.h>

class ezQtSearchWidget;
class ezQtDocumentTreeView;
class ezSceneDocument;
class ezScene2Document;
class QStackedWidget;
struct ezScene2LayerEvent;

class ezQtScenegraphPanel : public ezQtDocumentPanel
{
  Q_OBJECT

public:
  ezQtScenegraphPanel(ads::CDockManager* pDockManager, QWidget* pParent, ezSceneDocument* pDocument);
  ezQtScenegraphPanel(ads::CDockManager* pDockManager, QWidget* pParent, ezScene2Document* pDocument);
  ~ezQtScenegraphPanel();

private:
  void LayerEventHandler(const ezScene2LayerEvent& e);
  void LayerLoaded(const ezUuid& layerGuid);
  void LayerUnloaded(const ezUuid& layerGuid);
  void ActiveLayerChanged(const ezUuid& layerGuid);

private:
  ezSceneDocument* m_pSceneDocument;
  QStackedWidget* m_pStack = nullptr;
  ezQtGameObjectWidget* m_pMainGameObjectWidget = nullptr;
  ezEvent<const ezScene2LayerEvent&>::Unsubscriber m_LayerEventUnsubscriber;
  ezMap<ezUuid, ezQtGameObjectWidget*> m_LayerWidgets;
};
