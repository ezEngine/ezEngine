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

class ezQtAssetConfigAdapter : public ezQtNameableAdapter
{
public:
  ezQtAssetConfigAdapter(const ezQtAssetConfigsDlg* pDialog, const ezDocumentObjectManager* pTree, const ezRTTI* pType)
      : ezQtNameableAdapter(pTree, pType, "", "Name")
  {
    m_pDialog = pDialog;
  }

  virtual QVariant data(const ezDocumentObject* pObject, int row, int column, int role) const override
  {
    if (column == 0)
    {
      if (role == Qt::DecorationRole)
      {
        const ezInt32 iPlatform = pObject->GetTypeAccessor().GetValue("Platform").ConvertTo<ezInt32>();

        switch (iPlatform)
        {
          case ezAssetTargetPlatform::PC:
            return ezQtUiServices::GetSingleton()->GetCachedIconResource(":EditorFramework/Icons/PlatformWindows16.png");

          case ezAssetTargetPlatform::Android:
            return ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/PlatformAndroid16.png");
        }
      }

      if (role == Qt::DisplayRole)
      {
        QString name = ezQtNameableAdapter::data(pObject, row, column, role).toString();

        if (row == m_pDialog->m_uiActiveConfig)
        {
          name += " (active)";
        }

        return name;
      }
    }

    return ezQtNameableAdapter::data(pObject, row, column, role);
  }

private:
  const ezQtAssetConfigsDlg* m_pDialog = nullptr;
};

ezQtAssetConfigsDlg::ezQtAssetConfigsDlg(QWidget* parent)
    : QDialog(parent)
{
  setupUi(this);

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  AddButton->setEnabled(false);
  DeleteButton->setEnabled(false);
  RenameButton->setEnabled(false);

  m_pDocument = EZ_DEFAULT_NEW(ezAssetConfigsDocument, "<none>");

  // if this is set, all properties are applied immediately
  // m_pDocument->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtAssetConfigsDlg::PropertyChangedEventHandler,
  // this));
  std::unique_ptr<ezQtDocumentTreeModel> pModel(new ezQtDocumentTreeModel(m_pDocument->GetObjectManager()));
  pModel->AddAdapter(new ezQtDummyAdapter(m_pDocument->GetObjectManager(), ezGetStaticRTTI<ezDocumentRoot>(), "Children"));
  pModel->AddAdapter(new ezQtAssetConfigAdapter(this, m_pDocument->GetObjectManager(), ezAssetPlatformConfig::GetStaticRTTI()));

  Tree->Initialize(m_pDocument, std::move(pModel));
  Tree->SetAllowDragDrop(false);
  Tree->SetAllowDeleteObjects(false);
  Tree->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  Tree->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

  connect(Tree, &QTreeView::doubleClicked, this, &ezQtAssetConfigsDlg::OnItemDoubleClicked);

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

void ezQtAssetConfigsDlg::OnItemDoubleClicked(QModelIndex idx)
{
  if (m_uiActiveConfig == idx.row())
    return;

  const QModelIndex oldIdx = Tree->model()->index(m_uiActiveConfig, 0);

  m_uiActiveConfig = idx.row();

  QVector<int> roles;
  roles.push_back(Qt::DisplayRole);
  Tree->model()->dataChanged(idx, idx, roles);
  Tree->model()->dataChanged(oldIdx, oldIdx, roles);
}

void ezQtAssetConfigsDlg::AllAssetConfigsToObject()
{
  m_uiActiveConfig = ezAssetCurator::GetSingleton()->GetActivePlatformIndex();

  m_ConfigBinding.Clear();

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
