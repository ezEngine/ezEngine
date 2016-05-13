#include <PCH.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPreferences, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezMap<const ezDocument*, ezMap<const ezRTTI*, ezPreferences*>> ezPreferences::s_Preferences;

ezPreferences::ezPreferences(Domain domain, Visibility visibility, const char* szUniqueName)
{
  m_Domain = domain;
  m_Visibility = visibility;
  m_sUniqueName = szUniqueName;
  m_pDocument = nullptr;
}

ezPreferences* ezPreferences::GetPreferences(const ezRTTI* pRtti, const ezDocument* pDocument)
{
  EZ_ASSERT_DEV(ezQtEditorApp::GetSingleton() != nullptr, "Editor app is not available in this process");

  auto it = s_Preferences[pDocument].Find(pRtti);

  if (it.IsValid())
    return it.Value();

  auto pAlloc = pRtti->GetAllocator();
  EZ_ASSERT_DEV(pAlloc != nullptr, "Invalid allocator for preferences type");

  if (!pAlloc->CanAllocate())
  {
    EZ_ASSERT_DEV(pAlloc->CanAllocate(), "Cannot create a preferences object that does not have a proper allocator");
    return nullptr;
  }

  ezPreferences* pPref = reinterpret_cast<ezPreferences*>(pAlloc->Allocate());
  pPref->m_pDocument = pDocument;
  s_Preferences[pDocument][pRtti] = pPref;

  if (pPref->m_Domain == Domain::Document)
  {
    EZ_ASSERT_DEV(pDocument != nullptr, "Preferences of this type can only be used per document");
  }
  else
  {
    EZ_ASSERT_DEV(pDocument == nullptr, "Preferences of this type can not be used with a document");
  }

  pPref->Load();
  return pPref;
}

ezString ezPreferences::GetFilePath() const
{
  ezStringBuilder path;

  if (m_Domain == Domain::Application)
  {
    path = ezQtEditorApp::GetSingleton()->GetEditorPreferencesFolder(m_Visibility == Visibility::User);
    path.AppendPath(m_sUniqueName);

    if (m_Visibility == Visibility::User)
      path.ChangeFileExtension("userpref");
    else
      path.ChangeFileExtension("pref");
  }

  if (m_Domain == Domain::Project)
  {
    path = ezQtEditorApp::GetSingleton()->GetProjectPreferencesFolder(m_Visibility == Visibility::User);
    path.AppendPath(m_sUniqueName);

    if (m_Visibility == Visibility::User)
      path.ChangeFileExtension("userpref");
    else
      path.ChangeFileExtension("pref");
  }

  if (m_Domain == Domain::Document)
  {
    path = ezQtEditorApp::GetSingleton()->GetDocumentPreferencesFolder(m_pDocument, m_Visibility == Visibility::User);
    path.AppendPath(m_sUniqueName);

    if (m_Visibility == Visibility::User)
      path.ChangeFileExtension("userpref");
    else
      path.ChangeFileExtension("pref");
  }

  return path;
}

void ezPreferences::Load()
{
  ezFileReader file;
  if (file.Open(GetFilePath()).Failed())
    return;

  ezReflectionSerializer::ReadObjectPropertiesFromJSON(file, *GetDynamicRTTI(), this);
}

void ezPreferences::Save() const
{
  ezFileWriter file;
  if (file.Open(GetFilePath()).Failed())
    return;

  ezReflectionSerializer::WriteObjectToJSON(file, GetDynamicRTTI(), this, ezJSONWriter::WhitespaceMode::All);
}


void ezPreferences::SaveDocumentPreferences(const ezDocument* pDocument)
{
  auto& docPrefs = s_Preferences[pDocument];

  // save all preferences for the given document
  for (auto it = docPrefs.GetIterator(); it.IsValid(); ++it)
  {
    auto pPref = it.Value();
    pPref->Save();

    pPref->GetDynamicRTTI()->GetAllocator()->Deallocate(pPref);
  }

  // clean them all up
  docPrefs.Clear();
}

void ezPreferences::SaveProjectAndEditorPreferences()
{
  // just save everything that is not tied to a document
  SaveDocumentPreferences(nullptr);
}

