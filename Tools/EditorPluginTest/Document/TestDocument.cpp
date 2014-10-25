#include <PCH.h>
#include <EditorPluginTest/Document/TestDocument.h>
#include <EditorPluginTest/Objects/TestObject.h>
#include <EditorPluginTest/Objects/TestObjectManager.h>

ezTestDocument::ezTestDocument()
{
  //m_pCommandHistory;
  m_pObjectManager = new ezTestObjectManager(this);
  //m_pSelectionManager
}

ezTestDocument::~ezTestDocument()
{
  delete m_pObjectManager;
}


