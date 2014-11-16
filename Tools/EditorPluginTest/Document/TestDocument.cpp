#include <PCH.h>
#include <EditorPluginTest/Document/TestDocument.h>
#include <EditorPluginTest/Objects/TestObject.h>
#include <EditorPluginTest/Objects/TestObjectManager.h>
#include <EditorFramework/EditorFramework.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestDocument, ezDocumentBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTestDocument::ezTestDocument(const char* szDocumentPath) : ezDocumentBase(szDocumentPath)
{
  //m_pCommandHistory;
  m_pObjectManager = new ezTestObjectManager(this);
  //m_pSelectionManager

  SetModified(true);

  ezEditorFramework::GetDocumentSettings(this, "TestPlugin").RegisterValueBool("HasSettings", true);
  ezEditorFramework::GetDocumentSettings(this, "TestPlugin").RegisterValueBool("HasUserSettings", true, ezSettingsFlags::User);

  ezEditorFramework::GetEditorSettings("TestPlugin").RegisterValueBool("HasEditorSettings", true);
  ezEditorFramework::GetProjectSettings("TestPlugin").RegisterValueBool("HasProjectSettings", true);
}

ezTestDocument::~ezTestDocument()
{
  delete m_pObjectManager;
}


