#include <PCH.h>
#include <EditorPluginScene/Commands/SceneCommands.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDuplicateObjectsCommand, ezCommandBase, 1, ezRTTIDefaultAllocator<ezDuplicateObjectsCommand>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("JsonGraph", m_sJsonGraph),
EZ_MEMBER_PROPERTY("ParentNodes", m_sParentNodes),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();



////////////////////////////////////////////////////////////////////////
// ezDuplicateObjectsCommand
////////////////////////////////////////////////////////////////////////

ezDuplicateObjectsCommand::ezDuplicateObjectsCommand()
{
}

ezStatus ezDuplicateObjectsCommand::Do(bool bRedo)
{
  ezSceneDocument* pDocument = static_cast<ezSceneDocument*>(GetDocument());

  if (!bRedo)
  {
    ezAbstractObjectGraph graph;

    {
      // Deserialize 
      ezMemoryStreamStorage streamStorage;
      ezMemoryStreamWriter memoryWriter(&streamStorage);
      memoryWriter.WriteBytes(m_sJsonGraph.GetData(), m_sJsonGraph.GetElementCount());

      ezMemoryStreamReader memoryReader(&streamStorage);
      ezAbstractGraphJsonSerializer::Read(memoryReader, &graph);
    }

    // Remap
    ezUuid seed;
    seed.CreateNewUuid();
    graph.ReMapNodeGuids(seed);

    ezDocumentObjectConverterReader reader(&graph, pDocument->GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateOnly);

    ezHybridArray<ezDocumentBase::PasteInfo, 16> ToBePasted;
    ezStringBuilder sParentGuids = m_sParentNodes;
    ezStringBuilder sNextParentGuid;

    ezMap<ezUuid, ezUuid> ParentGuids;

    while (!sParentGuids.IsEmpty())
    {
      sNextParentGuid.SetSubString_ElementCount(sParentGuids, 40);
      sParentGuids.Shrink(41, 0);

      ezUuid guidObj = ezConversionUtils::ConvertStringToUuid(sNextParentGuid);
      guidObj.CombineWithSeed(seed);

      sNextParentGuid.SetSubString_ElementCount(sParentGuids, 40);
      sParentGuids.Shrink(41, 0);

      ParentGuids[guidObj] = ezConversionUtils::ConvertStringToUuid(sNextParentGuid);
    }

    auto& nodes = graph.GetAllNodes();
    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      auto* pNode = it.Value();
      if (ezStringUtils::IsEqual(pNode->GetNodeName(), "root"))
      {
        auto* pNewObject = reader.CreateObjectFromNode(pNode, nullptr, nullptr, ezVariant());
        reader.ApplyPropertiesToObject(pNode, pNewObject);

        auto& ref = ToBePasted.ExpandAndGetRef();
        ref.m_pObject = pNewObject;
        ref.m_pParent = nullptr;

        const ezUuid guidParent = ParentGuids[pNode->GetGuid()];

        if (guidParent.IsValid())
          ref.m_pParent = pDocument->GetObjectManager()->GetObject(guidParent);
      }
    }

    if (pDocument->Duplicate(ToBePasted))
    {
      for (const auto& item : ToBePasted)
      {
        auto& po = m_DuplicatedObjects.ExpandAndGetRef();
        po.m_pObject = item.m_pObject;
        po.m_Index = item.m_pObject->GetPropertyIndex();
        po.m_pParent = item.m_pParent;
        po.m_sParentProperty = item.m_pObject->GetParentProperty();
      }
    }
    else
    {
      for (const auto& item : ToBePasted)
      {
        pDocument->GetObjectManager()->DestroyObject(item.m_pObject);
      }
    }

    if (m_DuplicatedObjects.IsEmpty())
      return ezStatus(EZ_FAILURE, "Paste Objects: nothing was pasted!");
  }
  else
  {
    // Re-add at recorded place.
    for (auto& po : m_DuplicatedObjects)
    {
      pDocument->GetObjectManager()->AddObject(po.m_pObject, po.m_pParent, po.m_sParentProperty, po.m_Index);
    }
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezDuplicateObjectsCommand::Undo(bool bFireEvents)
{
  EZ_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  ezDocumentBase* pDocument = GetDocument();

  for (auto& po : m_DuplicatedObjects)
  {
    if (!pDocument->GetObjectManager()->CanRemove(po.m_pObject))
      return ezStatus(EZ_FAILURE, "Add Object: Removal of the object is forbidden!");

    pDocument->GetObjectManager()->RemoveObject(po.m_pObject);
  }

  return ezStatus(EZ_SUCCESS);
}

void ezDuplicateObjectsCommand::Cleanup(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    for (auto& po : m_DuplicatedObjects)
    {
      GetDocument()->GetObjectManager()->DestroyObject(po.m_pObject);
    }
    m_DuplicatedObjects.Clear();
  }

}

