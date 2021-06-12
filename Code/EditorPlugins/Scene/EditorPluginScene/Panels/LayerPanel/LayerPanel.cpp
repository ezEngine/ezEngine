#include <EditorPluginScenePCH.h>

#include <EditorPluginScene/Panels/LayerPanel/LayerPanel.moc.h>
#include <QVBoxLayout>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>

ezQtLayerPanel::ezQtLayerPanel(QWidget* pParent, ezScene2Document* pDocument)
  : ezQtDocumentPanel(pParent)
{
  setObjectName("LayerPanel");
  setWindowTitle("Layers");
  m_pSceneDocument = pDocument;

  std::unique_ptr<ezQtDocumentTreeModel> pModel(new ezQtDocumentTreeModel(pDocument->GetObjectManager(), pDocument->GetSettingsObject()->GetGuid()));
  pModel->AddAdapter(new ezQtDummyAdapter(pDocument->GetObjectManager(), ezGetStaticRTTI<ezSceneDocumentSettings>(), "Layers"));
  pModel->AddAdapter(new ezQtDummyAdapter(pDocument->GetObjectManager(), ezGetStaticRTTI<ezSceneLayer>(), ""));
  pModel->AddAdapter(new ezQtGameObjectAdapter(pDocument));

  m_pTreeWidget = new ezQtDocumentTreeView(this, pDocument, std::move(pModel), m_pSceneDocument->GetLayerSelectionManager());
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pTreeWidget->SetAllowDeleteObjects(true);

  setWidget(m_pTreeWidget);
}

ezQtLayerPanel::~ezQtLayerPanel() {}
