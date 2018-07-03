#include <PCH.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestDocument, 1, ezRTTINoAllocator)

EZ_END_DYNAMIC_REFLECTED_TYPE;


ezTestDocumentObjectManager::ezTestDocumentObjectManager()
{
}

ezTestDocumentObjectManager::~ezTestDocumentObjectManager()
{
}

ezTestDocument::ezTestDocument(const char* szDocumentPath, bool bUseIPCObjectMirror /*= false*/) : ezDocument(szDocumentPath, EZ_DEFAULT_NEW(ezTestDocumentObjectManager))
, m_bUseIPCObjectMirror(bUseIPCObjectMirror)
{

}

ezTestDocument::~ezTestDocument()
{
  if (m_bUseIPCObjectMirror)
  {
    m_ObjectMirror.Clear();
    m_ObjectMirror.DeInit();
  }
}

void ezTestDocument::InitializeAfterLoading()
{
  ezDocument::InitializeAfterLoading();

  if (m_bUseIPCObjectMirror)
  {
    m_ObjectMirror.InitSender(GetObjectManager());
    m_ObjectMirror.InitReceiver(&m_Context);
    m_ObjectMirror.SendDocument();
  }
}

void ezTestDocument::ApplyNativePropertyChangesToObjectManager(ezDocumentObject* pObject)
{
  // Create native object graph
  ezAbstractObjectGraph graph;
  ezAbstractObjectNode* pRootNode = nullptr;
  {
    ezRttiConverterWriter rttiConverter(&graph, &m_Context, true, true);
    pRootNode = rttiConverter.AddObjectToGraph(pObject->GetType(), m_ObjectMirror.GetNativeObjectPointer(pObject), "Object");
  }

  // Create object manager graph
  ezAbstractObjectGraph origGraph;
  ezAbstractObjectNode* pOrigRootNode = nullptr;
  {
    ezDocumentObjectConverterWriter writer(&origGraph, GetObjectManager());
    pOrigRootNode = writer.AddObjectToGraph(pObject);
  }

  // Remap native guids so they match the object manager (stuff like embedded classes will not have a guid on the native side).
  graph.ReMapNodeGuidsToMatchGraph(pRootNode, origGraph, pOrigRootNode);
  ezDeque<ezAbstractGraphDiffOperation> diffResult;

  graph.CreateDiffWithBaseGraph(origGraph, diffResult);

  // As we messed up the native side the object mirror is no longer synced and needs to be destroyed.
  m_ObjectMirror.Clear();
  m_ObjectMirror.DeInit();

  // Apply diff while object mirror is down.
  GetObjectAccessor()->StartTransaction("Apply Native Property Changes to Object");
  ezDocumentObjectConverterReader::ApplyDiffToObject(GetObjectAccessor(), pObject, diffResult);
  GetObjectAccessor()->FinishTransaction();

  // Restart mirror from scratch.
  m_ObjectMirror.InitSender(GetObjectManager());
  m_ObjectMirror.InitReceiver(&m_Context);
  m_ObjectMirror.SendDocument();
}

const char* ezTestDocument::GetDocumentTypeDisplayString() const
{
  return "Test";
}

ezDocumentInfo* ezTestDocument::CreateDocumentInfo()
{
  return EZ_DEFAULT_NEW(ezDocumentInfo);
}
