#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Serialization/DocumentJSONReader.h>
#include <ToolsFoundation/Serialization/DocumentJSONWriter.h>
#include <ToolsFoundation/Serialization/SerializedTypeAccessorObject.h>
#include <ToolsFoundation/Object/SerializedDocumentObject.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentInfo, ezReflectedClass, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("DocumentID", m_DocumentID),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezDocumentInfo::ezDocumentInfo()
{
  m_DocumentID.CreateNewUuid();
}


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentBase, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezEvent<const ezDocumentBase::Event&> ezDocumentBase::s_EventsAny;

ezDocumentBase::ezDocumentBase(const char* szPath, ezDocumentObjectManager* pDocumentObjectManagerImpl) :
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
}

ezDocumentBase::~ezDocumentBase()
{
  m_SelectionManager.SetOwner(nullptr);

  m_pObjectManager->DestroyAllObjects();

  m_CommandHistory.ClearRedoHistory();
  m_CommandHistory.ClearUndoHistory();

  EZ_DEFAULT_DELETE(m_pObjectManager);
  EZ_DEFAULT_DELETE(m_pDocumentInfo);
}

void ezDocumentBase::SetupDocumentInfo(const ezDocumentTypeDescriptor& TypeDescriptor)
{
  m_TypeDescriptor = TypeDescriptor;
  m_pDocumentInfo = CreateDocumentInfo();

  EZ_ASSERT_DEV(m_pDocumentInfo != nullptr, "invalid document info");
}

static void WriteObjectRecursive(ezDocumentJSONWriter& writer, const ezDocumentObjectBase* pObject)
{
  ezSerializedDocumentObjectWriter object(pObject);
  writer.WriteObject(object);

  auto children = pObject->GetChildren();
  const ezUInt32 uiCount = children.GetCount();
  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    WriteObjectRecursive(writer, children[i]);
  }
}

void ezDocumentBase::SetModified(bool b)
{
  if (m_bModified == b)
    return;

  m_bModified = b;

  Event e;
  e.m_pDocument = this;
  e.m_Type = Event::Type::ModifiedChanged;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

void ezDocumentBase::SetReadOnly(bool b)
{
  if (m_bReadOnly == b)
    return;

  m_bReadOnly = b;

  Event e;
  e.m_pDocument = this;
  e.m_Type = Event::Type::ReadOnlyChanged;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

void ezDocumentBase::BroadcastSaveDocumentMetaState()
{
  Event e;
  e.m_pDocument = this;
  e.m_Type = Event::Type::SaveDocumentMetaState;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

ezStatus ezDocumentBase::SaveDocument()
{
  ezStatus ret = InternalSaveDocument();

  if (ret.m_Result.Succeeded())
  {
    Event e;
    e.m_pDocument = this;
    e.m_Type = Event::Type::DocumentSaved;
    m_EventsOne.Broadcast(e);
    s_EventsAny.Broadcast(e);

    SetModified(false);
  }

  BroadcastSaveDocumentMetaState();

  return ret;
}

void ezDocumentBase::EnsureVisible()
{
  Event e;
  e.m_pDocument = this;
  e.m_Type = Event::Type::EnsureVisible;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

ezStatus ezDocumentBase::InternalSaveDocument()
{
  ezFileWriter file;
  if (file.Open(m_sDocumentPath) == EZ_FAILURE)
  {
    return ezStatus(EZ_FAILURE, "Unable to open file for writing!");
  }

  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;
  ezRttiConverterWriter rttiConverter(&graph, &context, true, true);
  ezDocumentObjectConverterWriter objectConverter(&graph, GetObjectManager(), true, true);
  context.RegisterObject(GetGuid(), m_pDocumentInfo->GetDynamicRTTI(), m_pDocumentInfo);

  rttiConverter.AddObjectToGraph(m_pDocumentInfo, "Header");
  objectConverter.AddObjectToGraph(GetObjectManager()->GetRootObject(), "ObjectTree");

  ezAbstractGraphJsonSerializer::Write(file, &graph, ezJSONWriter::WhitespaceMode::All);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezDocumentBase::InternalLoadDocument()
{
  /*
  // TODO: BLA
  ezFileReader file;
  if (file.Open(m_sDocumentPath) == EZ_FAILURE)
  {
    return ezStatus(EZ_FAILURE, "Unable to open file for reading!");
  }

  ezDocumentJSONReader reader(&file);

  if (reader.OpenGroup("Header"))
  {
    ezUuid objectGuid;
    ezStringBuilder sType;
    ezUuid parentGuid;
    while (reader.PeekNextObject(objectGuid, sType, parentGuid))
    {
      if (sType == m_pDocumentInfo->GetDynamicRTTI()->GetTypeName())
      {
        const ezRTTI* pRtti = ezRTTI::FindTypeByName(m_pDocumentInfo->GetDynamicRTTI()->GetTypeName());
        EZ_ASSERT_DEV(pRtti != nullptr, "Need to register ezDocumentInfo at the ezPhantomRttiManager first!");

        ezReflectedTypeDirectAccessor acc(m_pDocumentInfo, nullptr);
        ezSerializedTypeAccessorObjectReader objectReader(&acc);
        reader.ReadObject(objectReader);
      }
    }
  }

  if (reader.OpenGroup("ObjectTree"))
  {
    ezUuid objectGuid;
    ezStringBuilder sType;
    ezUuid parentGuid;
    while (reader.PeekNextObject(objectGuid, sType, parentGuid))
    {
      const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
      ezDocumentObjectBase* pObject = GetObjectManager()->CreateObject(pRtti, objectGuid);

      if (pObject)
      {
        ezSerializedDocumentObjectReader objectReader(pObject);
        reader.ReadObject(objectReader);
        if (parentGuid.IsValid())
        {
          ezDocumentObjectBase* pParent = GetObjectManager()->GetObject(parentGuid);
          if (pParent)
          {
            GetObjectManager()->AddObject(pObject, pParent);
          }
          else
          {
            // TODO: Oh noes!
          }
          
        }
        else
        {
          GetObjectManager()->AddObject(pObject, nullptr);
        }
      }
      else
      {
        // TODO: Accumulate failed objects.
      }
      
    }
    
  }
*/
  SetModified(false);
  return ezStatus(EZ_SUCCESS);
}

void ezDocumentBase::DeleteSelectedObjects()
{
  auto objects = GetSelectionManager()->GetSelection();

  // make sure the whole selection is cleared, otherwise each delete command would reduce the selection one by one
  GetSelectionManager()->Clear();

  auto history = GetCommandHistory();
  history->StartTransaction();

  ezRemoveObjectCommand cmd;

  for (const ezDocumentObjectBase* pObject : objects)
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

