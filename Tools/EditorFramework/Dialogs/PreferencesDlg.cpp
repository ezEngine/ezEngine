#include <PCH.h>
#include <EditorFramework/Dialogs/PreferencesDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

class ezPreferencesObjectManager : public ezDocumentObjectManager
{
public:

  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override
  {
    for (auto pRtti : m_KnownTypes)
    {
      Types.PushBack(pRtti);
    }
  }

  ezHybridArray<const ezRTTI*, 16> m_KnownTypes;
};


class ezPreferencesDocument : public ezDocument
{
public:
  ezPreferencesDocument(const char* szDocumentPath) : ezDocument(szDocumentPath, EZ_DEFAULT_NEW(ezPreferencesObjectManager))
  {
  }

  ~ezPreferencesDocument()
  {
  }


  virtual const char* GetDocumentTypeDisplayString() const override
  {
    return "Preferences";
  }

//protected:
  virtual void InitializeAfterLoading() override
  {
    ezDocument::InitializeAfterLoading();
  }

  virtual ezStatus InternalLoadDocument() override
  {
    GetObjectManager()->DestroyAllObjects();

    return ezDocument::InternalLoadDocument();
  }

//private:
public:

  virtual ezDocumentInfo* CreateDocumentInfo() override
  {
    return EZ_DEFAULT_NEW(ezDocumentInfo);
  }
};



PreferencesDlg::PreferencesDlg(QWidget* parent) : QDialog(parent)
{
  setupUi(this);


  m_pDocument = EZ_DEFAULT_NEW(ezPreferencesDocument, "<none>");
  static_cast<ezPreferencesObjectManager*>(m_pDocument->GetObjectManager())->m_KnownTypes.PushBack(ezProjectPreferencesUser::GetStaticRTTI());

  m_pDocument->InitializeAfterLoading();

  {
    ezProjectPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();
    NativeToObject(pPreferences);
  }

  Properties->SetDocument(m_pDocument);
  m_pDocument->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&PreferencesDlg::PropertyChangedEventHandler, this));
  m_pDocument->GetSelectionManager()->SetSelection(m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  m_sSelectedSettingDomain = "<Application>";

  //EZ_VERIFY(connect(ComboSettingsDomain, SIGNAL(currentIndexChanged(int)), this, SLOT(SlotComboSettingsDomainIndexChanged(int))) != nullptr, "signal/slot connection failed");

  UpdateSettings();

  
}

PreferencesDlg::~PreferencesDlg()
{
  //Properties->SetDocument(nullptr);
  delete Properties;
  Properties = nullptr;

  EZ_DEFAULT_DELETE(m_pDocument);
}

ezUuid PreferencesDlg::NativeToObject(ezPreferences* pPreferences)
{
  const ezRTTI* pType = pPreferences->GetDynamicRTTI();
  // Write properties to graph.
  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;
  ezRttiConverterWriter conv(&graph, &context, true, true);

  ezUuid guid;
  guid.CreateNewUuid();
  context.RegisterObject(guid, pType, pPreferences);
  ezAbstractObjectNode* pNode = conv.AddObjectToGraph(pType, pPreferences, "root");

  // Read from graph and write into matching document object.
  auto pRoot = m_pDocument->GetObjectManager()->GetRootObject();
  ezDocumentObject* pObject = m_pDocument->GetObjectManager()->CreateObject(pType);
  m_pDocument->GetObjectManager()->AddObject(pObject, pRoot, "Children", 0);

  ezDocumentObjectConverterReader objectConverter(&graph, m_pDocument->GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  objectConverter.ApplyPropertiesToObject(pNode, pObject);

  return guid;
}

void PreferencesDlg::ObjectToNative(ezUuid objectGuid)
{
  ezDocumentObject* pObject = m_pDocument->GetObjectManager()->GetObject(objectGuid);
  const ezRTTI* pType = pObject->GetTypeAccessor().GetType();

  // Write object to graph.
  ezAbstractObjectGraph graph;
  ezDocumentObjectConverterWriter objectConverter(&graph, m_pDocument->GetObjectManager(), false, true);
  ezAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(pObject, "root");

  // Read from graph and write to native object.
  ezRttiConverterContext context;
  ezRttiConverterReader conv(&graph, &context);

  ezPreferences* pPreferences = ezPreferences::QueryPreferences(pType /*TODO: which document?*/);
  conv.ApplyPropertiesToObject(pNode, pType, pPreferences);
}

void PreferencesDlg::SlotComboSettingsDomainIndexChanged(int iIndex)
{
  m_sSelectedSettingDomain = ComboSettingsDomain->itemData(iIndex, Qt::UserRole).toString().toUtf8().data();

  UpdateSettings();

  //if (iIndex == 0) // Application
  //{
  //}
  //else if (iIndex == 1) // Project
  //{
  //}
  //else
  //{
  //  ComboSettingsDomain->itemText(iIndex);
  //}
}


void PreferencesDlg::UpdateSettings()
{
  //ezStringBuilder sTemp;

  //ComboSettingsDomain->blockSignals(true);
  //ComboSettingsDomain->clear();

  //int iSelected = 0;
  //ComboSettingsDomain->addItem("<Application>", QString("<Application>"));

  //if (ezToolsProject::IsProjectOpen())
  //{
  //  ComboSettingsDomain->addItem("<Project>", QString("<Project>"));

  //  if ("<Project>" == m_sSelectedSettingDomain)
  //    iSelected = 1;

  //  ezInt32 iIndex = 2;
  //  for (auto dm : ezDocumentManager::GetAllDocumentManagers())
  //  {
  //    for (auto doc : dm->GetAllDocuments())
  //    {
  //      ezString sRel;
  //      if (!ezToolsProject::GetSingleton()->IsDocumentInAllowedRoot(doc->GetDocumentPath(), &sRel))
  //        continue;

  //      ComboSettingsDomain->addItem(sRel.GetData(), QString(doc->GetDocumentPath()));

  //      if (doc->GetDocumentPath() == m_sSelectedSettingDomain)
  //        iSelected = iIndex;

  //      ++iIndex;
  //    }
  //  }
  //}

  //ComboSettingsDomain->setCurrentIndex(iSelected);
  //m_sSelectedSettingDomain = ComboSettingsDomain->itemData(iSelected, Qt::UserRole).toString().toUtf8().data();
  //ComboSettingsDomain->blockSignals(false);

  //m_pSettingsGrid->BeginProperties();

  //for (const ezString& sName : ezQtEditorApp::GetSingleton()->GetRegisteredPluginNamesForSettings())
  //{
  //  ezSettings* s;

  //  if (m_sSelectedSettingDomain == "<Application>")
  //    s = &ezQtEditorApp::GetSingleton()->GetEditorSettings(sName);
  //  else
  //    if (m_sSelectedSettingDomain == "<Project>")
  //      s = &ezQtEditorApp::GetSingleton()->GetProjectSettings(sName);
  //    else
  //      s = &ezQtEditorApp::GetSingleton()->GetDocumentSettings(m_sSelectedSettingDomain, sName);

  //  bool bAddedGroupName = false;

  //  for (auto it = s->GetAllSettings().GetIterator(); it.IsValid(); ++it)
  //  {
  //    if (!it.Value().m_Flags.IsAnySet(ezSettingsFlags::Registered))
  //      continue;
  //    if (it.Value().m_Flags.IsAnySet(ezSettingsFlags::Hidden))
  //      continue;

  //    if (!bAddedGroupName)
  //    {
  //      m_pSettingsGrid->AddProperty("Data Group:", sName, nullptr, true);
  //      bAddedGroupName = true;
  //    }

  //    m_pSettingsGrid->AddProperty(it.Key(), it.Value().m_Value, &it.Value().m_Value, it.Value().m_Flags.IsAnySet(ezSettingsFlags::ReadOnly));
  //  }
  //}

  //m_pSettingsGrid->EndProperties();
}

void PreferencesDlg::PropertyChangedEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  ObjectToNative(e.m_pObject->GetGuid());
}
