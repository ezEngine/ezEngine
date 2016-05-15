#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentObjectMetaData, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    //EZ_MEMBER_PROPERTY("MetaHidden", m_bHidden) // remove this property to disable serialization
    EZ_MEMBER_PROPERTY("MetaFromPrefab", m_CreateFromPrefab),
    EZ_MEMBER_PROPERTY("MetaPrefabSeed", m_PrefabSeedGuid),
    EZ_MEMBER_PROPERTY("MetaBasePrefab", m_sBasePrefab),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentInfo, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DocumentID", m_DocumentID),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezDocumentInfo::ezDocumentInfo()
{
  m_DocumentID.CreateNewUuid();
}


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezEvent<const ezDocumentEvent&> ezDocument::s_EventsAny;

ezDocument::ezDocument(const char* szPath, ezDocumentObjectManager* pDocumentObjectManagerImpl) :
  m_CommandHistory(this)
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
}

ezDocument::~ezDocument()
{
  m_SelectionManager.SetOwner(nullptr);

  m_pObjectManager->DestroyAllObjects();

  m_CommandHistory.ClearRedoHistory();
  m_CommandHistory.ClearUndoHistory();

  EZ_DEFAULT_DELETE(m_pObjectManager);
  EZ_DEFAULT_DELETE(m_pDocumentInfo);
}

void ezDocument::SetupDocumentInfo(const ezDocumentTypeDescriptor& TypeDescriptor)
{
  m_TypeDescriptor = TypeDescriptor;
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

void ezDocument::BroadcastSaveDocumentMetaState()
{
  ezDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = ezDocumentEvent::Type::SaveDocumentMetaState;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

ezStatus ezDocument::SaveDocument()
{
  ezStatus ret = InternalSaveDocument();

  if (ret.m_Result.Succeeded())
  {
    ezDocumentEvent e;
    e.m_pDocument = this;
    e.m_Type = ezDocumentEvent::Type::DocumentSaved;
    m_EventsOne.Broadcast(e);
    s_EventsAny.Broadcast(e);

    SetModified(false);

    // after saving once, this information is pointless
    m_uiUnknownObjectTypeInstances = 0;
    m_UnknownObjectTypes.Clear();
  }

  BroadcastSaveDocumentMetaState();

  if (ret.m_Result.Succeeded())
  {
    InternalAfterSaveDocument();
  }

  return ret;
}

void ezDocument::EnsureVisible()
{
  ezDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = ezDocumentEvent::Type::EnsureVisible;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

ezStatus ezDocument::InternalSaveDocument()
{
  ezFileWriter file;
  if (file.Open(m_sDocumentPath) == EZ_FAILURE)
  {
    return ezStatus(EZ_FAILURE, "Unable to open file for writing!");
  }

  ezAbstractObjectGraph graph;
  {
    ezRttiConverterContext context;
    ezRttiConverterWriter rttiConverter(&graph, &context, true, true);
    ezDocumentObjectConverterWriter objectConverter(&graph, GetObjectManager(), true, true);
    context.RegisterObject(GetGuid(), m_pDocumentInfo->GetDynamicRTTI(), m_pDocumentInfo);

    rttiConverter.AddObjectToGraph(m_pDocumentInfo, "Header");
    objectConverter.AddObjectToGraph(GetObjectManager()->GetRootObject(), "ObjectTree");

    AttachMetaDataBeforeSaving(graph);
  }
  ezAbstractObjectGraph typesGraph;
  {
    ezRttiConverterContext context;
    ezRttiConverterWriter rttiConverter(&typesGraph, &context, true, true);

    ezSet<const ezRTTI*> types;
    ezToolsReflectionUtils::GatherObjectTypes(GetObjectManager()->GetRootObject(), types);
    for (const ezRTTI* pType : types)
    {
      ezReflectedTypeDescriptor desc;
      ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pType, desc);

      context.RegisterObject(ezUuid::StableUuidForString(pType->GetTypeName()), ezGetStaticRTTI<ezReflectedTypeDescriptor>(), &desc);
      rttiConverter.AddObjectToGraph(ezGetStaticRTTI<ezReflectedTypeDescriptor>(), &desc);
    }
  }
  ezAbstractGraphJsonSerializer::Write(file, &graph, &typesGraph, ezJSONWriter::WhitespaceMode::LessIndentation);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezDocument::InternalLoadDocument()
{
  ezFileReader file;
  if (file.Open(m_sDocumentPath) == EZ_FAILURE)
  {
    return ezStatus(EZ_FAILURE, "Unable to open file for reading!");
  }

  ezAbstractObjectGraph graph;
  ezAbstractObjectGraph typesGraph;
  ezAbstractGraphJsonSerializer::Read(file, &graph, &typesGraph);

  {
    // Deserialize and register serialized phantom types.
    ezString sDescTypeName = ezGetStaticRTTI<ezReflectedTypeDescriptor>()->GetTypeName();
    ezDynamicArray<ezReflectedTypeDescriptor*> descriptors;
    auto& nodes = typesGraph.GetAllNodes();
    descriptors.Reserve(nodes.GetCount()); // Overkill but doesn't matter much as it's just temporary.
    ezRttiConverterContext context;
    ezRttiConverterReader rttiConverter(&typesGraph, &context);

    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value()->GetType() == sDescTypeName)
      {
        descriptors.PushBack(static_cast<ezReflectedTypeDescriptor*>(rttiConverter.CreateObjectFromNode(it.Value())));
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
  }

  ezRttiConverterContext context;
  ezRttiConverterReader rttiConverter(&graph, &context);
  ezDocumentObjectConverterReader objectConverter(&graph, GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument);

  auto* pHeaderNode = graph.GetNodeByName("Header");
  rttiConverter.ApplyPropertiesToObject(pHeaderNode, m_pDocumentInfo->GetDynamicRTTI(), m_pDocumentInfo);



  auto* pRootNode = graph.GetNodeByName("ObjectTree");
  objectConverter.ApplyPropertiesToObject(pRootNode, GetObjectManager()->GetRootObject());

  SetUnknownObjectTypes(objectConverter.GetUnknownObjectTypes(), objectConverter.GetNumUnknownObjectCreations());

  RestoreMetaDataAfterLoading(graph);

  SetModified(false);
  return ezStatus(EZ_SUCCESS);
}


void ezDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph)
{
  m_DocumentObjectMetaData.AttachMetaDataToAbstractGraph(graph);
}


void ezDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph)
{
  m_DocumentObjectMetaData.RestoreMetaDataFromAbstractGraph(graph);
}

void ezDocument::SetUnknownObjectTypes(const ezSet<ezString>& Types, ezUInt32 uiInstances)
{
  m_UnknownObjectTypes = Types;
  m_uiUnknownObjectTypeInstances = uiInstances;
}

void ezDocument::DeleteSelectedObjects()
{
  auto objects = GetSelectionManager()->GetSelection();

  // make sure the whole selection is cleared, otherwise each delete command would reduce the selection one by one
  GetSelectionManager()->Clear();

  auto history = GetCommandHistory();
  history->StartTransaction();

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

void ezDocument::ShowDocumentStatus(const char* szFormat, ...)
{
  va_list args;
  va_start(args, szFormat);
  ezStringBuilder sMsg;
  sMsg.FormatArgs(szFormat, args);
  va_end(args);

  ezDocumentEvent e;
  e.m_pDocument = this;
  e.m_szStatusMsg = sMsg;
  e.m_Type = ezDocumentEvent::Type::DocumentStatusMsg;

  m_EventsOne.Broadcast(e);
}

