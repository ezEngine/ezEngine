#include <PCH.h>
#include <EditorFramework/Dialogs/PreferencesDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Serialization/BinarySerializer.h>

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
    m_ObjectMirror.Clear();
    m_ObjectMirror.DeInit();
  }


  virtual const char* GetDocumentTypeDisplayString() const override
  {
    return "Preferences";
  }

//protected:
  virtual void InitializeAfterLoading() override
  {
    ezDocument::InitializeAfterLoading();

    m_ObjectMirror.InitSender(GetObjectManager());
    m_ObjectMirror.InitReceiver(&m_Context);
    m_ObjectMirror.SendDocument();
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

  ezDocumentObjectMirror m_ObjectMirror;
  ezRttiConverterContext m_Context;
};



PreferencesDlg::PreferencesDlg(QWidget* parent) : QDialog(parent)
{
  setupUi(this);

  ezProjectPreferencesUser* pPreferences = ezPreferences::GetPreferences<ezProjectPreferencesUser>();

  m_pDocument = EZ_DEFAULT_NEW(ezPreferencesDocument, "<none>");
  static_cast<ezPreferencesObjectManager*>(m_pDocument->GetObjectManager())->m_KnownTypes.PushBack(ezProjectPreferencesUser::GetStaticRTTI());

  m_pDocument->InitializeAfterLoading();

  {
    auto pRoot = m_pDocument->GetObjectManager()->GetRootObject();

    //{
    //  ezDocumentObject* pObject = m_pDocument->GetObjectManager()->CreateObject(ezViewPreferencesUser::GetStaticRTTI());
    //  m_pDocument->GetObjectManager()->AddObject(pObject, pRoot, "Children", 0);

    //  ezViewPreferencesUser* pPref = static_cast<ezViewPreferencesUser*>(m_pDocument->m_ObjectMirror.GetNativeObjectPointer(m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]));

    //  //m_pDocument->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&PreferencesDlg::PropertyChangedEventHandler, this));

    //  ///// TODO: Sync values !
    //  //// this doesn't work
    //  //pPref->m_iSomeValue = -23;
    //  //pPref->m_sRenderPipelines = "pups";
    //}

      ezMemoryStreamStorage storage;
      ezMemoryStreamWriter writer(&storage);
      ezMemoryStreamReader reader(&storage);
      ezReflectionSerializer::WriteObjectToBinary(writer, pPreferences->GetDynamicRTTI(), pPreferences);

      {
        ezAbstractObjectGraph graph;
        ezAbstractGraphBinarySerializer::Read(reader, &graph);

        ezRttiConverterContext context;
        ezRttiConverterWriter conv(&graph, &context, false, false);

        ezDocumentObjectConverterReader objectConverter(&graph, m_pDocument->GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument);

        auto* pRootNode = graph.GetNodeByName("root");
        objectConverter.ApplyPropertiesToObject(pRootNode, m_pDocument->GetObjectManager()->GetRootObject());


        //ezRttiConverterContext context;
        //ezRttiConverterWriter conv(&graph, &context, false, true);

        //ezUuid guid;
        //guid.CreateNewUuid();

        //context.RegisterObject(guid, pRtti, const_cast<void*>(pObject));
        //ezAbstractObjectNode* pNode = conv.AddObjectToGraph(pRtti, const_cast<void*>(pObject), "root");
      }
  }

  Properties->SetDocument(m_pDocument);
  //m_pDocument->GetSelectionManager()->SetSelection(m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  m_pDocument->GetSelectionManager()->SetSelection(m_pDocument->GetObjectManager()->GetRootObject());

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
  ezProjectPreferencesUser* pPref = static_cast<ezProjectPreferencesUser*>(m_pDocument->m_ObjectMirror.GetNativeObjectPointer(m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]));

  int i = 0;
}
