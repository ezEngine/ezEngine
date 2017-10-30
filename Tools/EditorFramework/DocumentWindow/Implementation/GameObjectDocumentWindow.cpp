#include <PCH.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/Document/GameObjectDocument.h>

ezQtGameObjectDocumentWindow::ezQtGameObjectDocumentWindow(ezGameObjectDocument* pDocument)
  : ezQtEngineDocumentWindow(pDocument)
{
}

ezQtGameObjectDocumentWindow::~ezQtGameObjectDocumentWindow()
{
}

ezGameObjectDocument* ezQtGameObjectDocumentWindow::GetGameObjectDocument() const
{
  return static_cast<ezGameObjectDocument*>(GetDocument());
}
