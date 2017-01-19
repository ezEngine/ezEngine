#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Document/Document.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <CoreUtils/Other/Progress.h>

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

ezStatus ezDocument::SaveDocument()
{
  if (!IsModified())
    return ezStatus(EZ_SUCCESS);

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
  ezDeferredFileWriter file;
  file.SetOutput(m_sDocumentPath);

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


  ezAbstractGraphDdlSerializer::Write(file, &graph, &typesGraph, false);

  if (file.Close() == EZ_FAILURE)
  {
    return ezStatus(ezFmt("Unable to open file '{0}' for writing!", m_sDocumentPath.GetData()));
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezDocument::InternalLoadDocument()
{
  // this would currently crash in Qt, due to the processEvents in the QtProgressBar
  //ezProgressRange range("Loading Document", 5, false);

  ezAbstractObjectGraph graph;
  ezAbstractObjectGraph typesGraph;

  ezMemoryStreamStorage storage;
  ezMemoryStreamReader memreader(&storage);

  {
    ezFileReader file;
    if (file.Open(m_sDocumentPath) == EZ_FAILURE)
    {
      return ezStatus("Unable to open file for reading!");
    }

    //range.BeginNextStep("Reading File");
    storage.ReadAll(file);

    //range.BeginNextStep("Parsing Graph");
    {
      ezStopwatch sw;
      if (ezAbstractGraphDdlSerializer::Read(memreader, &graph, &typesGraph).Failed())
        return ezStatus("Failed to parse DDL graph");

      ezTime t = sw.GetRunningTotal();
      ezLog::Debug("DDL parsing time: {0} msec", ezArgF(t.GetMilliseconds(), 1));
    }
  }

  {
    //range.BeginNextStep("Deserializing Types");

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

  {
    //range.BeginNextStep("Restoring Objects");

    auto* pHeaderNode = graph.GetNodeByName("Header");
    rttiConverter.ApplyPropertiesToObject(pHeaderNode, m_pDocumentInfo->GetDynamicRTTI(), m_pDocumentInfo);

    auto* pRootNode = graph.GetNodeByName("ObjectTree");
    objectConverter.ApplyPropertiesToObject(pRootNode, GetObjectManager()->GetRootObject());

    SetUnknownObjectTypes(objectConverter.GetUnknownObjectTypes(), objectConverter.GetNumUnknownObjectCreations());
  }

  {
    //range.BeginNextStep("Restoring Meta-Data");
    RestoreMetaDataAfterLoading(graph);
  }

  SetModified(false);
  return ezStatus(EZ_SUCCESS);
}


void ezDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
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
