#include <EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Dialogs/AssetProfilesDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GameEngine/Configuration/PlatformProfile.h>
#include <QInputDialog>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

class ezAssetProfilesObjectManager : public ezDocumentObjectManager
{
public:
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override
  {
    Types.PushBack(ezGetStaticRTTI<ezPlatformProfile>());
  }
};

class ezAssetProfilesDocument : public ezDocument
{
public:
  ezAssetProfilesDocument(const char* szDocumentPath)
    : ezDocument(szDocumentPath, EZ_DEFAULT_NEW(ezAssetProfilesObjectManager))
  {
  }

  virtual const char* GetDocumentTypeDisplayString() const override { return "Asset Profile"; }

public:
  virtual ezDocumentInfo* CreateDocumentInfo() override { return EZ_DEFAULT_NEW(ezDocumentInfo); }
};

class ezQtAssetConfigAdapter : public ezQtNameableAdapter
{
public:
  ezQtAssetConfigAdapter(const ezQtAssetProfilesDlg* pDialog, const ezDocumentObjectManager* pTree, const ezRTTI* pType)
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
          case ezProfileTargetPlatform::PC:
            return ezQtUiServices::GetSingleton()->GetCachedIconResource(":EditorFramework/Icons/PlatformWindows16.png");

          case ezProfileTargetPlatform::UWP:
            return ezQtUiServices::GetSingleton()->GetCachedIconResource(":EditorFramework/Icons/PlatformWindows16.png"); // TODO: icon

          case ezProfileTargetPlatform::Android:
            return ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/PlatformAndroid16.png");
        }
      }

      if (role == Qt::DisplayRole)
      {
        QString name = ezQtNameableAdapter::data(pObject, row, column, role).toString();

        if (row == ezAssetCurator::GetSingleton()->GetActiveAssetProfileIndex())
        {
          name += " (active)";
        }
        else if (row == m_pDialog->m_uiActiveConfig)
        {
          name += " (switch to)";
        }

        return name;
      }
    }

    return ezQtNameableAdapter::data(pObject, row, column, role);
  }

private:
  const ezQtAssetProfilesDlg* m_pDialog = nullptr;
};

ezQtAssetProfilesDlg::ezQtAssetProfilesDlg(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  // do not allow to delete or rename the first item
  DeleteButton->setEnabled(false);
  RenameButton->setEnabled(false);

  m_pDocument = EZ_DEFAULT_NEW(ezAssetProfilesDocument, "<none>");
  m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtAssetProfilesDlg::SelectionEventHandler, this));

  std::unique_ptr<ezQtDocumentTreeModel> pModel(new ezQtDocumentTreeModel(m_pDocument->GetObjectManager()));
  pModel->AddAdapter(new ezQtDummyAdapter(m_pDocument->GetObjectManager(), ezGetStaticRTTI<ezDocumentRoot>(), "Children"));
  pModel->AddAdapter(new ezQtAssetConfigAdapter(this, m_pDocument->GetObjectManager(), ezPlatformProfile::GetStaticRTTI()));

  Tree->Initialize(m_pDocument, std::move(pModel));
  Tree->SetAllowDragDrop(false);
  Tree->SetAllowDeleteObjects(false);
  Tree->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  Tree->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

  connect(Tree, &QTreeView::doubleClicked, this, &ezQtAssetProfilesDlg::OnItemDoubleClicked);

  AllAssetProfilesToObject();

  Properties->SetDocument(m_pDocument);

  auto& rootChildArray = m_pDocument->GetObjectManager()->GetRootObject()->GetChildren();

  if (!rootChildArray.IsEmpty())
  {
    m_pDocument->GetSelectionManager()->SetSelection(rootChildArray[ezAssetCurator::GetSingleton()->GetActiveAssetProfileIndex()]);
  }
}

ezQtAssetProfilesDlg::~ezQtAssetProfilesDlg()
{
  delete Tree;
  Tree = nullptr;

  delete Properties;
  Properties = nullptr;

  EZ_DEFAULT_DELETE(m_pDocument);
}

ezUuid ezQtAssetProfilesDlg::NativeToObject(ezPlatformProfile* pProfile)
{
  const ezRTTI* pType = pProfile->GetDynamicRTTI();
  // Write properties to graph.
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;
  ezRttiConverterWriter conv(&graph, &context, true, true);

  ezUuid guid;
  guid.CreateNewUuid();
  context.RegisterObject(guid, pType, pProfile);
  ezAbstractObjectNode* pNode = conv.AddObjectToGraph(pType, pProfile, "root");

  // Read from graph and write into matching document object.
  auto pRoot = m_pDocument->GetObjectManager()->GetRootObject();
  ezDocumentObject* pObject = m_pDocument->GetObjectManager()->CreateObject(pType);
  m_pDocument->GetObjectManager()->AddObject(pObject, pRoot, "Children", -1);

  ezDocumentObjectConverterReader objectConverter(
    &graph, m_pDocument->GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  objectConverter.ApplyPropertiesToObject(pNode, pObject);

  return pObject->GetGuid();
}

void ezQtAssetProfilesDlg::ObjectToNative(ezUuid objectGuid, ezPlatformProfile* pProfile)
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

  conv.ApplyPropertiesToObject(pNode, pType, pProfile);
}


void ezQtAssetProfilesDlg::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  const auto& selection = m_pDocument->GetSelectionManager()->GetSelection();

  const bool bAllowModification =
    !selection.IsEmpty() && (selection[0] != m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  DeleteButton->setEnabled(bAllowModification);
  RenameButton->setEnabled(bAllowModification);
}

void ezQtAssetProfilesDlg::on_ButtonOk_clicked()
{
  ApplyAllChanges();

  // SaveAssetProfileRuntimeConfig
  for (ezUInt32 i = 0; i < ezAssetCurator::GetSingleton()->GetNumAssetProfiles(); ++i)
  {
    ezStringBuilder sProfileRuntimeDataFile;

    ezPlatformProfile* pProfile = ezAssetCurator::GetSingleton()->GetAssetProfile(i);

    sProfileRuntimeDataFile.Set(":project/RuntimeConfigs/", pProfile->GetConfigName(), ".ezProfile");

    pProfile->SaveForRuntime(sProfileRuntimeDataFile);
  }

  accept();

  ezAssetCurator::GetSingleton()->SaveAssetProfiles();
}

void ezQtAssetProfilesDlg::on_ButtonCancel_clicked()
{
  m_uiActiveConfig = ezAssetCurator::GetSingleton()->GetActiveAssetProfileIndex();
  reject();
}

void ezQtAssetProfilesDlg::OnItemDoubleClicked(QModelIndex idx)
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

bool ezQtAssetProfilesDlg::CheckProfileNameUniqueness(const char* szName)
{
  if (ezStringUtils::IsNullOrEmpty(szName))
  {
    ezQtUiServices::GetSingleton()->MessageBoxInformation("Empty strings are not allowed as profile names.");
    return false;
  }

  if (!ezStringUtils::IsValidIdentifierName(szName))
  {
    ezQtUiServices::GetSingleton()->MessageBoxInformation("Profile names may only contain characters, digits and underscores.");
    return false;
  }

  const auto& objects = m_pDocument->GetObjectManager()->GetRootObject()->GetChildren();
  for (const ezDocumentObject* pObject : objects)
  {
    if (pObject->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>().IsEqual_NoCase(szName))
    {
      ezQtUiServices::GetSingleton()->MessageBoxInformation("A profile with this name already exists.");
      return false;
    }
  }

  return true;
}

bool ezQtAssetProfilesDlg::DetermineNewProfileName(QWidget* parent, ezString& result)
{
  while (true)
  {
    bool ok = false;
    result = QInputDialog::getText(parent, "Profile Name", "New Name:", QLineEdit::Normal, "", &ok).toUtf8().data();

    if (!ok)
      return false;

    if (CheckProfileNameUniqueness(result))
      return true;
  }
}

void ezQtAssetProfilesDlg::on_AddButton_clicked()
{
  ezString sProfileName;
  if (!DetermineNewProfileName(this, sProfileName))
    return;

  ezPlatformProfile profile;
  profile.m_sName = sProfileName;
  profile.AddMissingConfigs();

  auto& binding = m_ProfileBindings[NativeToObject(&profile)];
  binding.m_pProfile = nullptr;
  binding.m_State = Binding::State::Added;

  // select the new profile
  m_pDocument->GetSelectionManager()->SetSelection(m_pDocument->GetObjectManager()->GetRootObject()->GetChildren().PeekBack());
}

void ezQtAssetProfilesDlg::on_DeleteButton_clicked()
{
  const auto& sel = m_pDocument->GetSelectionManager()->GetSelection();
  if (sel.IsEmpty())
    return;

  // do not allow to delete the first object
  if (sel[0] == m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0])
    return;

  if (ezQtUiServices::GetSingleton()->MessageBoxQuestion(
        ezFmt("Delete the selected profile?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
    return;

  m_ProfileBindings[sel[0]->GetGuid()].m_State = Binding::State::Deleted;

  m_pDocument->GetCommandHistory()->StartTransaction("Delete Profile");

  ezRemoveObjectCommand cmd;
  cmd.m_Object = sel[0]->GetGuid();

  m_pDocument->GetCommandHistory()->AddCommand(cmd);

  m_pDocument->GetCommandHistory()->FinishTransaction();
}

void ezQtAssetProfilesDlg::on_RenameButton_clicked()
{
  const auto& sel = m_pDocument->GetSelectionManager()->GetSelection();
  if (sel.IsEmpty())
    return;

  // do not allow to rename the first object
  if (sel[0] == m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0])
    return;

  ezString sProfileName;
  if (!DetermineNewProfileName(this, sProfileName))
    return;

  m_pDocument->GetCommandHistory()->StartTransaction("Rename Profile");

  ezSetObjectPropertyCommand cmd;
  cmd.m_Object = sel[0]->GetGuid();
  cmd.m_sProperty = "Name";
  cmd.m_NewValue = sProfileName;

  m_pDocument->GetCommandHistory()->AddCommand(cmd);

  m_pDocument->GetCommandHistory()->FinishTransaction();
}

void ezQtAssetProfilesDlg::on_SwitchToButton_clicked()
{
  const auto& sel = Tree->selectionModel()->selectedRows();
  if (sel.isEmpty())
    return;

  OnItemDoubleClicked(sel[0]);
}

void ezQtAssetProfilesDlg::AllAssetProfilesToObject()
{
  m_uiActiveConfig = ezAssetCurator::GetSingleton()->GetActiveAssetProfileIndex();

  m_ProfileBindings.Clear();

  for (ezUInt32 i = 0; i < ezAssetCurator::GetSingleton()->GetNumAssetProfiles(); ++i)
  {
    auto* pProfile = ezAssetCurator::GetSingleton()->GetAssetProfile(i);

    m_ProfileBindings[NativeToObject(pProfile)].m_pProfile = pProfile;
  }
}

void ezQtAssetProfilesDlg::PropertyChangedEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  const ezUuid guid = e.m_pObject->GetGuid();
  EZ_ASSERT_DEV(m_ProfileBindings.Contains(guid), "Object GUID is not in the known list!");

  ObjectToNative(guid, m_ProfileBindings[guid].m_pProfile);
}

void ezQtAssetProfilesDlg::ApplyAllChanges()
{
  for (auto it = m_ProfileBindings.GetIterator(); it.IsValid(); ++it)
  {
    const auto& binding = it.Value();

    ezPlatformProfile* pProfile = binding.m_pProfile;

    if (binding.m_State == Binding::State::Deleted)
    {
      ezAssetCurator::GetSingleton()->DeleteAssetProfile(pProfile);
      continue;
    }

    if (binding.m_State == Binding::State::Added)
    {
      // create a new profile object and synchronize the state directly into that
      pProfile = ezAssetCurator::GetSingleton()->CreateAssetProfile();
    }

    ObjectToNative(it.Key(), pProfile);
  }
}
