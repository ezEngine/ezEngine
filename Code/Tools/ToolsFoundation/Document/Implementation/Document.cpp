#include <PCH.h>
#include <ToolsFoundation/Document/Document.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/Progress.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Threading/TaskSystem.h>
#include <ToolsFoundation/Document/DocumentTasks.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentObjectMetaData, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    //EZ_MEMBER_PROPERTY("MetaHidden", m_bHidden) // remove this property to disable serialization
    EZ_MEMBER_PROPERTY("MetaFromPrefab", m_CreateFromPrefab),
    EZ_MEMBER_PROPERTY("MetaPrefabSeed", m_PrefabSeedGuid),
    EZ_MEMBER_PROPERTY("MetaBasePrefab", m_sBasePrefab),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentInfo, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DocumentID", m_DocumentID),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezDocumentInfo::ezDocumentInfo()
{
  m_DocumentID.CreateNewUuid();
}


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezEvent<const ezDocumentEvent&> ezDocument::s_EventsAny;

ezDocument::ezDocument(const char* szPath, ezDocumentObjectManager* pDocumentObjectManagerImpl)
    : m_CommandHistory(this)
{
  m_pDocumentInfo = nullptr;
  m_sDocumentPath = szPath;
  m_pObjectManager = pDocumentObjectManagerImpl;
  m_pObjectManager->SetDocument(this);

  m_SelectionManager.SetOwner(this);

  m_bWindowRequested = false;
  m_bModified = true;
  m_bReadOnly = false;
  m_bAddToRecentFilesList = true;

  m_uiUnknownObjectTypeInstances = 0;

  m_ObjectAccessor = EZ_DEFAULT_NEW(ezObjectCommandAccessor, &m_CommandHistory);
}

ezDocument::~ezDocument()
{
  m_SelectionManager.SetOwner(nullptr);

  m_pObjectManager->DestroyAllObjects();

  m_CommandHistory.ClearRedoHistory();
  m_CommandHistory.ClearUndoHistory();

  EZ_DEFAULT_DELETE(m_pObjectManager);
  EZ_DEFAULT_DELETE(m_pDocumentInfo);
  EZ_DEFAULT_DELETE(m_ObjectAccessor);
}

void ezDocument::SetupDocumentInfo(const ezDocumentTypeDescriptor* pTypeDescriptor)
{
  m_pTypeDescriptor = pTypeDescriptor;
  m_pDocumentInfo = CreateDocumentInfo();

  EZ_ASSERT_DEV(m_pDocumentInfo != nullptr, "invalid document info");
}

void ezDocument::SetModified(bool b)
{
  if (m_bModified == b)
    return;

  m_bModified = b;

  ezDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = ezDocumentEvent::Type::ModifiedChanged;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

void ezDocument::SetReadOnly(bool b)
{
  if (m_bReadOnly == b)
    return;

  m_bReadOnly = b;

  ezDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = ezDocumentEvent::Type::ReadOnlyChanged;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

ezStatus ezDocument::SaveDocument(bool bForce)
{
  if (!IsModified() && !bForce)
    return ezStatus(EZ_SUCCESS);

  ezStatus result;
  m_activeSaveTask = InternalSaveDocument([&result](ezDocument* doc, ezStatus res)
  {
    result = res;
  });
  ezTaskSystem::WaitForGroup(m_activeSaveTask);
  return result;
}


ezTaskGroupID ezDocument::SaveDocumentAsync(AfterSaveCallback callback, bool bForce)
{
  if (!IsModified() && !bForce)
    return ezTaskGroupID();

  m_activeSaveTask = InternalSaveDocument(callback);
  return m_activeSaveTask;
}

void ezDocument::EnsureVisible()
{
  ezDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = ezDocumentEvent::Type::EnsureVisible;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

ezTaskGroupID ezDocument::InternalSaveDocument(AfterSaveCallback callback)
{
  EZ_PROFILE("InternalSaveDocument");
  ezTaskGroupID saveID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::LongRunningHighPriority);
  auto saveTask = EZ_DEFAULT_NEW(ezSaveDocumentTask);
  {
    saveTask->m_document = this;
    saveTask->file.SetOutput(m_sDocumentPath);
    saveTask->SetOnTaskFinished([](ezTask* pTask) { EZ_DEFAULT_DELETE(pTask); });
    ezTaskSystem::AddTaskToGroup(saveID, saveTask);

    {
      ezRttiConverterContext context;
      ezRttiConverterWriter rttiConverter(&saveTask->headerGraph, &context, true, true);
      context.RegisterObject(GetGuid(), m_pDocumentInfo->GetDynamicRTTI(), m_pDocumentInfo);
      rttiConverter.AddObjectToGraph(m_pDocumentInfo, "Header");
    }
    {
      // Do not serialize any temporary properties into the document.
      auto filter = [](const ezAbstractProperty* pProp) -> bool {
        if (pProp->GetAttributeByType<ezTemporaryAttribute>() != nullptr)
          return false;
        return true;
      };
      ezDocumentObjectConverterWriter objectConverter(&saveTask->objectGraph, GetObjectManager(), filter);
      objectConverter.AddObjectToGraph(GetObjectManager()->GetRootObject(), "ObjectTree");

      AttachMetaDataBeforeSaving(saveTask->objectGraph);
    }
    {
      ezSet<const ezRTTI*> types;
      ezToolsReflectionUtils::GatherObjectTypes(GetObjectManager()->GetRootObject(), types);
      ezToolsReflectionUtils::SerializeTypes(types, saveTask->typesGraph);
    }
  }

  ezTaskGroupID afterSaveID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::SomeFrameMainThread);
  {
    auto afterSaveTask = EZ_DEFAULT_NEW(ezAfterSaveDocumentTask);
    afterSaveTask->m_document = this;
    afterSaveTask->m_callback = callback;
    afterSaveTask->SetOnTaskFinished([](ezTask* pTask) { EZ_DEFAULT_DELETE(pTask); });
    ezTaskSystem::AddTaskToGroup(afterSaveID, afterSaveTask);
  }
  ezTaskSystem::AddTaskGroupDependency(afterSaveID, saveID);
  if (!ezTaskSystem::IsTaskGroupFinished(m_activeSaveTask))
  {
    ezTaskSystem::AddTaskGroupDependency(saveID, m_activeSaveTask);
  }

  ezTaskSystem::StartTaskGroup(saveID);
  ezTaskSystem::StartTaskGroup(afterSaveID);
  return afterSaveID;
}

ezStatus ezDocument::InternalLoadDocument()
{
  EZ_PROFILE("InternalLoadDocument");
  // this would currently crash in Qt, due to the processEvents in the QtProgressBar
  // ezProgressRange range("Loading Document", 5, false);

  ezUniquePtr<ezAbstractObjectGraph> header;
  ezUniquePtr<ezAbstractObjectGraph> objects;
  ezUniquePtr<ezAbstractObjectGraph> types;

  ezMemoryStreamStorage storage;
  ezMemoryStreamReader memreader(&storage);

  {
    EZ_PROFILE("Read File");
    ezFileReader file;
    if (file.Open(m_sDocumentPath) == EZ_FAILURE)
    {
      return ezStatus("Unable to open file for reading!");
    }

    // range.BeginNextStep("Reading File");
    storage.ReadAll(file);

    // range.BeginNextStep("Parsing Graph");
    {
      EZ_PROFILE("parse DDL graph");
      ezStopwatch sw;
      if (ezAbstractGraphDdlSerializer::ReadDocument(memreader, header, objects, types, true).Failed())
        return ezStatus("Failed to parse DDL graph");

      ezTime t = sw.GetRunningTotal();
      ezLog::Debug("DDL parsing time: {0} msec", ezArgF(t.GetMilliseconds(), 1));
    }
  }

  {
    EZ_PROFILE("Deserializing Types");
    // range.BeginNextStep("Deserializing Types");

    // Deserialize and register serialized phantom types.
    ezString sDescTypeName = ezGetStaticRTTI<ezReflectedTypeDescriptor>()->GetTypeName();
    ezDynamicArray<ezReflectedTypeDescriptor*> descriptors;
    auto& nodes = types->GetAllNodes();
    descriptors.Reserve(nodes.GetCount()); // Overkill but doesn't matter much as it's just temporary.
    ezRttiConverterContext context;
    ezRttiConverterReader rttiConverter(types.Borrow(), &context);

    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value()->GetType() == sDescTypeName)
      {
        ezReflectedTypeDescriptor* pDesc = static_cast<ezReflectedTypeDescriptor*>(rttiConverter.CreateObjectFromNode(it.Value()));
        if (pDesc->m_Flags.IsSet(ezTypeFlags::Minimal))
        {
          ezGetStaticRTTI<ezReflectedTypeDescriptor>()->GetAllocator()->Deallocate(pDesc);
        }
        else
        {
          descriptors.PushBack(pDesc);
        }
      }
    }
    ezToolsReflectionUtils::DependencySortTypeDescriptorArray(descriptors);
    for (ezReflectedTypeDescriptor* desc : descriptors)
    {
      if (!ezRTTI::FindTypeByName(desc->m_sTypeName))
      {
        ezPhantomRttiManager::RegisterType(*desc);
      }
      ezGetStaticRTTI<ezReflectedTypeDescriptor>()->GetAllocator()->Deallocate(desc);
    }

    //
  }

  {
    EZ_PROFILE("Restoring Header");
    ezRttiConverterContext context;
    ezRttiConverterReader rttiConverter(header.Borrow(), &context);
    auto* pHeaderNode = header->GetNodeByName("Header");
    rttiConverter.ApplyPropertiesToObject(pHeaderNode, m_pDocumentInfo->GetDynamicRTTI(), m_pDocumentInfo);
  }

  {
    EZ_PROFILE("Restoring Objects");
    ezDocumentObjectConverterReader objectConverter(objects.Borrow(), GetObjectManager(),
                                                    ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
    // range.BeginNextStep("Restoring Objects");
    auto* pRootNode = objects->GetNodeByName("ObjectTree");
    objectConverter.ApplyPropertiesToObject(pRootNode, GetObjectManager()->GetRootObject());

    SetUnknownObjectTypes(objectConverter.GetUnknownObjectTypes(), objectConverter.GetNumUnknownObjectCreations());
  }

  {
    EZ_PROFILE("Restoring Meta-Data");
    // range.BeginNextStep("Restoring Meta-Data");
    RestoreMetaDataAfterLoading(*objects.Borrow(), false);
  }

  SetModified(false);
  return ezStatus(EZ_SUCCESS);
}


void ezDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  m_DocumentObjectMetaData.AttachMetaDataToAbstractGraph(graph);
}


void ezDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  m_DocumentObjectMetaData.RestoreMetaDataFromAbstractGraph(graph);
}

void ezDocument::SetUnknownObjectTypes(const ezSet<ezString>& Types, ezUInt32 uiInstances)
{
  m_UnknownObjectTypes = Types;
  m_uiUnknownObjectTypeInstances = uiInstances;
}


void ezDocument::BroadcastInterDocumentMessage(ezReflectedClass* pMessage, ezDocument* pSender)
{
  for (auto& man : ezDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDoc : man->GetAllDocuments())
    {
      if (pDoc == pSender)
        continue;

      pDoc->OnInterDocumentMessage(pMessage, pSender);
    }
  }
}

void ezDocument::DeleteSelectedObjects() const
{
  auto objects = GetSelectionManager()->GetSelection();

  // make sure the whole selection is cleared, otherwise each delete command would reduce the selection one by one
  GetSelectionManager()->Clear();

  auto history = GetCommandHistory();
  history->StartTransaction("Delete Object");

  ezRemoveObjectCommand cmd;

  for (const ezDocumentObject* pObject : objects)
  {
    cmd.m_Object = pObject->GetGuid();

    if (history->AddCommand(cmd).m_Result.Failed())
    {
      history->CancelTransaction();
      return;
    }
  }

  history->FinishTransaction();
}

void ezDocument::ShowDocumentStatus(const ezFormatString& msg) const
{
  ezStringBuilder tmp;

  ezDocumentEvent e;
  e.m_pDocument = this;
  e.m_szStatusMsg = msg.GetText(tmp);
  e.m_Type = ezDocumentEvent::Type::DocumentStatusMsg;

  m_EventsOne.Broadcast(e);
}


ezResult ezDocument::ComputeObjectTransformation(const ezDocumentObject* pObject, ezTransform& out_Result) const
{
  out_Result.SetIdentity();
  return EZ_FAILURE;
}

ezObjectAccessorBase* ezDocument::GetObjectAccessor() const
{
  return m_ObjectAccessor;
}

ezVariant ezDocument::GetDefaultValue(const ezDocumentObject* pObject, const char* szProperty, ezVariant index) const
{
  ezUuid rootObjectGuid = ezPrefabUtils::GetPrefabRoot(pObject, m_DocumentObjectMetaData);

  const ezAbstractProperty* pProp = pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty);
  if (pProp && rootObjectGuid.IsValid())
  {
    auto pMeta = m_DocumentObjectMetaData.BeginReadMetaData(rootObjectGuid);
    const ezAbstractObjectGraph* pGraph = ezPrefabCache::GetSingleton()->GetCachedPrefabGraph(pMeta->m_CreateFromPrefab);
    ezUuid objectPrefabGuid = pObject->GetGuid();
    objectPrefabGuid.RevertCombinationWithSeed(pMeta->m_PrefabSeedGuid);
    m_DocumentObjectMetaData.EndReadMetaData();

    if (pGraph)
    {
      ezVariant defaultValue = ezPrefabUtils::GetDefaultValue(*pGraph, objectPrefabGuid, szProperty, index);
      if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags) && defaultValue.IsA<ezString>())
      {
        ezInt64 iValue = 0;
        if (ezReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), defaultValue.Get<ezString>(), iValue))
        {
          defaultValue = iValue;
        }
        else
        {
          defaultValue = ezVariant();
        }
      }
      if (defaultValue.IsValid())
      {
        return defaultValue;
      }
    }
  }

  ezVariant defaultValue = ezToolsReflectionUtils::GetDefaultValue(pProp);
  return defaultValue;
}

bool ezDocument::IsDefaultValue(const ezDocumentObject* pObject, const char* szProperty, bool bReturnOnInvalid, ezVariant index) const
{
  const ezVariant def = GetDefaultValue(pObject, szProperty, index);

  if (!def.IsValid())
    return bReturnOnInvalid;

  return pObject->GetTypeAccessor().GetValue(szProperty, index) == def;
}
