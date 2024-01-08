#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/ApplyNativePropertyChangesContext.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <EditorPluginAssets/CustomDataAsset/CustomDataAsset.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCustomDataAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

void ezCustomDataDocumentObjectManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& ref_types) const
{
  ezRTTI::ForEachDerivedType<ezCustomData>(
    [&] (const ezRTTI* pCustomDataType) { ref_types.PushBack(pCustomDataType); },
    ezRTTI::ForEachOptions::ExcludeAbstract | ezRTTI::ForEachOptions::ExcludeNonAllocatable);
}

ezCustomDataAssetDocument::ezCustomDataAssetDocument(const ezRTTI* pDataType, ezStringView sDocumentPath)
: ezAssetDocument(sDocumentPath, EZ_DEFAULT_NEW(ezCustomDataDocumentObjectManager), ezAssetDocEngineConnection::None),
  m_LightSettings(false),
  m_pDataType(pDataType)
{
}

ezCustomDataAssetDocument::~ezCustomDataAssetDocument()
{
  m_ObjectMirror.Clear();
  m_ObjectMirror.DeInit();
}

void ezCustomDataAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  EnsureSettingsObjectExist();

  m_ObjectMirror.InitSender(this->GetObjectManager());
  m_ObjectMirror.InitReceiver(&m_Context);
  m_ObjectMirror.SendDocument();

  ezAssetDocument::InitializeAfterLoading(bFirstTimeCreation);

  this->AddSyncObject(&m_LightSettings);
}

ezStatus ezCustomDataAssetDocument::InternalLoadDocument()
{
  this->GetObjectManager()->DestroyAllObjects();

  ezStatus ret = ezAssetDocument::InternalLoadDocument();

  return ret;
}

// Index based remapping ignores address identity and solely uses the object's parent index to define
// its guid. Set it to true if the native changes are complete clear and replace operations and
// not incremental changes to the existing data.
void ezCustomDataAssetDocument::ApplyNativePropertyChangesToObjectManager(bool bForceIndexBasedRemapping)
{
  EZ_PROFILE_SCOPE("ApplyNativePropertyChangesToObjectManager");
  // Create object manager graph
  ezAbstractObjectGraph origGraph;
  ezAbstractObjectNode* pOrigRootNode = nullptr;
  {
    ezDocumentObjectConverterWriter writer(&origGraph, this->GetObjectManager());
    pOrigRootNode = writer.AddObjectToGraph(GetPropertyObject());
  }

  // Create native object graph
  ezAbstractObjectGraph graph;
  ezAbstractObjectNode* pRootNode = nullptr;
  {
    // The ezApplyNativePropertyChangesContext takes care of generating guids for native pointers that match those
    // of the object manager.
    ezApplyNativePropertyChangesContext nativeChangesContext(m_Context, origGraph);
    ezRttiConverterWriter rttiConverter(&graph, &nativeChangesContext, true, true);
    nativeChangesContext.RegisterObject(pOrigRootNode->GetGuid(), m_pDataType, GetProperties());
    pRootNode = rttiConverter.AddObjectToGraph(GetProperties(), "Object");
  }

  // Remapping is no longer necessary as ezApplyNativePropertyChangesContext takes care of mapping to the original nodes.
  // However, if the native changes are done like clear+rebuild everything, then no original object will be found and
  // every pointer will be deleted and re-created. Forcing the remapping (which works entirely via index and ignores
  // pointer addresses) will yield better results (e.g. no changes on two back-to -back transform calls).
  if (bForceIndexBasedRemapping)
  {
    // Remap native guids so they match the object manager (stuff like embedded classes will not have a guid on the native side).
    graph.ReMapNodeGuidsToMatchGraph(pRootNode, origGraph, pOrigRootNode);
  }

  ezDeque<ezAbstractGraphDiffOperation> diffResult;
  graph.CreateDiffWithBaseGraph(origGraph, diffResult);

  // if index-based remapping is used, we MUST send a change event of some kind
  // since the underlying data structures (memory locations) might have been changed,
  // even if there is no actual change to the content
  // the command history will detect that there was no change and actually send a "TransactionCanceled" event, but that is enough for other code to react to
  if (!diffResult.IsEmpty() || bForceIndexBasedRemapping)
  {
    // As we messed up the native side the object mirror is no longer synced and needs to be destroyed.
    m_ObjectMirror.Clear();

    // Apply diff while object mirror is down.
    this->GetObjectAccessor()->StartTransaction("Apply Native Property Changes to Object");

    ezDocumentObjectConverterReader::ApplyDiffToObject(this->GetObjectAccessor(), GetPropertyObject(), diffResult);

    // Re-apply document
    m_ObjectMirror.SendDocument();

    this->GetObjectAccessor()->FinishTransaction();
  }
}

void ezCustomDataAssetDocument::EnsureSettingsObjectExist()
{
  ezDocumentObject* pRoot = this->GetObjectManager()->GetRootObject();
  if (pRoot->GetChildren().IsEmpty())
  {
    ezDocumentObject* pObject = this->GetObjectManager()->CreateObject(m_pDataType);
    this->GetObjectManager()->AddObject(pObject, pRoot, "Children", 0);
  }
}

ezTransformStatus ezCustomDataAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
                                                                    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const ezCustomData* pProp = GetProperties();

  pProp->Save(stream);

  return ezStatus(EZ_SUCCESS);
}
