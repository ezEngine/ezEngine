#include <PCH.h>
#include <EditorFramework/Dialogs/PreferencesDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>

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

  virtual const char* GetDocumentTypeDisplayString() const override
  {
    return "Preferences";
  }

public:

  virtual ezDocumentInfo* CreateDocumentInfo() override
  {
    return EZ_DEFAULT_NEW(ezDocumentInfo);
  }
};



PreferencesDlg::PreferencesDlg(QWidget* parent) : QDialog(parent)
{
  setupUi(this);

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  m_pDocument = EZ_DEFAULT_NEW(ezPreferencesDocument, "<none>");

  // if this is set, all properties are applied immediatly
  //m_pDocument->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&PreferencesDlg::PropertyChangedEventHandler, this));

  Tree->Initialize(m_pDocument, ezPreferences::GetStaticRTTI(), "");
  Tree->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  Tree->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

  RegisterAllPreferenceTypes();
  AllPreferencesToObject();

  Properties->SetDocument(m_pDocument);

  m_pDocument->GetSelectionManager()->SetSelection(m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
}

PreferencesDlg::~PreferencesDlg()
{
  delete Tree;
  Tree = nullptr;

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
  m_pDocument->GetObjectManager()->AddObject(pObject, pRoot, "Children", -1);

  ezDocumentObjectConverterReader objectConverter(&graph, m_pDocument->GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  objectConverter.ApplyPropertiesToObject(pNode, pObject);

  return pObject->GetGuid();
}

void PreferencesDlg::ObjectToNative(ezUuid objectGuid, const ezDocument* pPrefDocument)
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

  ezPreferences* pPreferences = ezPreferences::QueryPreferences(pType, pPrefDocument);
  conv.ApplyPropertiesToObject(pNode, pType, pPreferences);
}


void PreferencesDlg::on_ButtonOk_clicked()
{
  ApplyAllChanges();
  accept();
}


void PreferencesDlg::RegisterAllPreferenceTypes()
{
  ezPreferencesObjectManager* pManager = static_cast<ezPreferencesObjectManager*>(m_pDocument->GetObjectManager());

  ezHybridArray<ezPreferences*, 16> AllPrefs;
  ezPreferences::GatherAllPreferences(AllPrefs);

  for (auto pref : AllPrefs)
  {
    pManager->m_KnownTypes.PushBack(pref->GetDynamicRTTI());
  }
}


void PreferencesDlg::AllPreferencesToObject()
{
  ezHybridArray<ezPreferences*, 16> AllPrefs;
  ezPreferences::GatherAllPreferences(AllPrefs);

  ezHybridArray<ezAbstractProperty*, 32> properties;

  ezMap<ezString, ezPreferences*> appPref;
  ezMap<ezString, ezPreferences*> projPref;
  ezMap<ezString, ezPreferences*> docPref;

  for (auto pref : AllPrefs)
  {
    bool noVisibleProperties = true;

    // ignore all objects that have no visible properties
    pref->GetDynamicRTTI()->GetAllProperties(properties);
    for (const ezAbstractProperty* prop : properties)
    {
      if (prop->GetAttributeByType<ezHiddenAttribute>() != nullptr)
        continue;

      noVisibleProperties = false;
      break;
    }

    if (noVisibleProperties)
      continue;

    switch (pref->GetDomain())
    {
    case ezPreferences::Domain::Application:
      appPref[pref->GetName()] = pref;
      break;
    case ezPreferences::Domain::Project:
      projPref[pref->GetName()] = pref;
      break;
    case ezPreferences::Domain::Document:
      docPref[pref->GetName()] = pref;
      break;
    }
  }

  // create the objects in a certain order

  for (auto it = appPref.GetIterator(); it.IsValid(); ++it)
  {
    m_DocumentBinding[NativeToObject(it.Value())] = it.Value()->GetDocumentAssociation();
  }

  for (auto it = projPref.GetIterator(); it.IsValid(); ++it)
  {
    m_DocumentBinding[NativeToObject(it.Value())] = it.Value()->GetDocumentAssociation();
  }

  for (auto it = docPref.GetIterator(); it.IsValid(); ++it)
  {
    m_DocumentBinding[NativeToObject(it.Value())] = it.Value()->GetDocumentAssociation();
  }
}

void PreferencesDlg::PropertyChangedEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  const ezUuid guid = e.m_pObject->GetGuid();
  EZ_ASSERT_DEV(m_DocumentBinding.Contains(guid), "Object GUID is not in the known list!");

  ObjectToNative(guid, m_DocumentBinding[guid]);
}

void PreferencesDlg::ApplyAllChanges()
{
  for (auto it = m_DocumentBinding.GetIterator(); it.IsValid(); ++it)
  {
    ObjectToNative(it.Key(), it.Value());
  }
}




