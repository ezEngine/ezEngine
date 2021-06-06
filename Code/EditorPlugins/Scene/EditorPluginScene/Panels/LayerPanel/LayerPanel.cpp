#include <EditorPluginScenePCH.h>

#include <EditorPluginScene/Panels/LayerPanel/LayerPanel.moc.h>

ezQtLayerPanel::ezQtLayerPanel(QWidget* pParent, ezScene2Document* pDocument)
  : ezQtDocumentPanel(pParent)
{
  setObjectName("LayerPanel");
  setWindowTitle("Layers");
  m_pSceneDocument = pDocument;


  QWidget* dummy = new QWidget(this);
  dummy->setContentsMargins(0, 0, 0, 0);
  setWidget(dummy);
}

ezQtLayerPanel::~ezQtLayerPanel() {}
