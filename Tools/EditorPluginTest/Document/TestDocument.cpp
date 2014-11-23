#include <PCH.h>
#include <EditorPluginTest/Document/TestDocument.h>
#include <EditorPluginTest/Objects/TestObject.h>
#include <EditorPluginTest/Objects/TestObjectManager.h>
#include <EditorFramework/EditorApp.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestDocument, ezDocumentBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTestDocument::ezTestDocument(const char* szDocumentPath) : ezDocumentBase(szDocumentPath)
{
  //m_pCommandHistory;
  m_pObjectManager = new ezTestObjectManager(this);
  //m_pSelectionManager

  SetModified(true);

  ezEditorApp::GetInstance()->GetDocumentSettings(this, "TestPlugin").RegisterValueBool("HasSettings", true);
  ezEditorApp::GetInstance()->GetDocumentSettings(this, "TestPlugin").RegisterValueBool("HasUserSettings", true, ezSettingsFlags::User);

  ezEditorApp::GetInstance()->GetEditorSettings("TestPlugin").RegisterValueBool("HasEditorSettings", true);
  ezEditorApp::GetInstance()->GetProjectSettings("TestPlugin").RegisterValueBool("HasProjectSettings", true);
}

ezTestDocument::~ezTestDocument()
{
  delete m_pObjectManager;
}

ezStatus ezTestDocument::InternalSaveDocument()
{
  return ezDocumentBase::InternalSaveDocument();
}

