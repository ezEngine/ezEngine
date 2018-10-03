#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetPlatformConfig.h>
#include <EditorFramework/Dialogs/AssetConfigsDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

class ezAssetConfigsObjectManager : public ezDocumentObjectManager
{
public:
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override
  {
    Types.PushBack(ezGetStaticRTTI<ezAssetPlatformConfig>());
  }
};

class ezAssetConfigsDocument : public ezDocument
{
public:
  ezAssetConfigsDocument(const char* szDocumentPath)
      : ezDocument(szDocumentPath, EZ_DEFAULT_NEW(ezAssetConfigsObjectManager))
  {
  }

  virtual const char* GetDocumentTypeDisplayString() const override { return "Asset Configurations"; }

public:
  virtual ezDocumentInfo* CreateDocumentInfo() override { return EZ_DEFAULT_NEW(ezDocumentInfo); }
};



ezQtAssetConfigsDlg::ezQtAssetConfigsDlg(QWidget* parent)
    : QDialog(parent)
{
  setupUi(this);

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  m_pDocument = EZ_DEFAULT_NEW(ezAssetConfigsDocument, "<none>");

  // if this is set, all properties are applied immediately
  // m_pDocument->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtAssetConfigsDlg::PropertyChangedEventHandler,
  // this));
  std::unique_ptr<ezQtDocumentTreeModel> pModel(new ezQtDocumentTreeModel(m_pDocument->GetObjectManager()));
  pModel->AddAdapter(new ezQtDummyAdapter(m_pDocument->GetObjectManager(), ezGetStaticRTTI<ezDocumentRoot>(), "Children"));
  pModel->AddAdapter(new ezQtNamedAdapter(m_pDocument->GetObjectManager(), ezAssetPlatformConfig::GetStaticRTTI(), "", "Name"));

  Tree->Initialize(m_pDocument, std::move(pModel));
  Tree->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  Tree->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

  AllAssetConfigsToObject();

  Properties->SetDocument(m_pDocument);

  if (!m_pDocument->GetObjectManager()->GetRootObject()->GetChildren().IsEmpty())
  {
    m_pDocument->GetSelectionManager()->SetSelection(m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }
}

ezQtAssetConfigsDlg::~ezQtAssetConfigsDlg()
{
  delete Tree;
  Tree = nullptr;

  delete Properties;
  Properties = nullptr;

  EZ_DEFAULT_DELETE(m_pDocument);
}

ezUuid ezQtAssetConfigsDlg::NativeToObject(ezAssetPlatformConfig* pConfig)
{
  const ezRTTI* pType = pConfig->GetDynamicRTTI();
  // Write properties to graph.
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;
  ezRttiConverterWriter conv(&graph, &context, true, true);

  ezUuid guid;
  guid.CreateNewUuid();
  context.RegisterObject(guid, pType, pConfig);
  ezAbstractObjectNode* pNode = conv.AddObjectToGraph(pType, pConfig, "root");

  // Read from graph and write into matching document object.
  auto pRoot = m_pDocument->GetObjectManager()->GetRootObject();
  ezDocumentObject* pObject = m_pDocument->GetObjectManager()->CreateObject(pType);
  m_pDocument->GetObjectManager()->AddObject(pObject, pRoot, "Children", -1);

  ezDocumentObjectConverterReader objectConverter(&graph, m_pDocument->GetObjectManager(),
                                                  ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  objectConverter.ApplyPropertiesToObject(pNode, pObject);

  return pObject->GetGuid();
}

void ezQtAssetConfigsDlg::ObjectToNative(ezUuid objectGuid, ezAssetPlatformConfig* pConfig)
{
  ezDocumentObject* pObject = m_pDocument->GetObjectManager()->GetObject(objectGuid);
  const ezRTTI* pType = pObject->GetTypeAccessor().GetType();

  // Write object to graph.
  ezAbstractObjectGraph graph;
  auto filter = [](const ezAbstractProperty* pProp) -> bool {
    if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
      return false;
    return true;
  };
  ezDocumentObjectConverterWriter objectConverter(&graph, m_pDocument->GetObjectManager(), filter);
  ezAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(pObject, "root");

  // Read from graph and write to native object.
  ezRttiConverterContext context;
  ezRttiConverterReader conv(&graph, &context);

  conv.ApplyPropertiesToObject(pNode, pType, pConfig);

  // pPreferences->TriggerPreferencesChangedEvent();
}


void ezQtAssetConfigsDlg::on_ButtonOk_clicked()
{
  ApplyAllChanges();
  accept();

  ezAssetCurator::GetSingleton()->SaveAssetConfigs();
}

void ezQtAssetConfigsDlg::AllAssetConfigsToObject()
{
  for (auto* pCfg : ezAssetCurator::GetSingleton()->m_AssetPlatformConfigs)
  {
    m_ConfigBinding[NativeToObject(pCfg)] = pCfg;
  }
}

void ezQtAssetConfigsDlg::PropertyChangedEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  const ezUuid guid = e.m_pObject->GetGuid();
  EZ_ASSERT_DEV(m_ConfigBinding.Contains(guid), "Object GUID is not in the known list!");

  ObjectToNative(guid, m_ConfigBinding[guid]);
}

void ezQtAssetConfigsDlg::ApplyAllChanges()
{
  for (auto it = m_ConfigBinding.GetIterator(); it.IsValid(); ++it)
  {
    ObjectToNative(it.Key(), it.Value());
  }
}
