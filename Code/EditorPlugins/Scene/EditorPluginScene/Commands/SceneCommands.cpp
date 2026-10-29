#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Commands/SceneCommands.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDuplicateObjectsCommand, 1, ezRTTIDefaultAllocator<ezDuplicateObjectsCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("GraphText", m_sGraphTextFormat),
    EZ_MEMBER_PROPERTY("ParentNodes", m_sParentNodes),
    EZ_MEMBER_PROPERTY("NumCopies", m_uiNumberOfCopies),
    EZ_MEMBER_PROPERTY("Translate", m_vAccumulativeTranslation),
    EZ_MEMBER_PROPERTY("Rotate", m_vAccumulativeRotation),
    EZ_MEMBER_PROPERTY("RandomRotation", m_vRandomRotation),
    EZ_MEMBER_PROPERTY("RandomTranslation", m_vRandomTranslation),
    EZ_MEMBER_PROPERTY("Group", m_bGroupDuplicates),
    EZ_MEMBER_PROPERTY("RevolveAxis", m_iRevolveAxis),
    EZ_MEMBER_PROPERTY("RevoleStartAngle", m_RevolveStartAngle),
    EZ_MEMBER_PROPERTY("RevolveAngleStep", m_RevolveAngleStep),
    EZ_MEMBER_PROPERTY("RevolveRadius", m_fRevolveRadius),
    EZ_MEMBER_PROPERTY("InsertIndex", m_iInsertIndex),
    EZ_MEMBER_PROPERTY("RandomSeed", m_uiRandomSeed),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


////////////////////////////////////////////////////////////////////////
// ezDuplicateObjectsCommand
////////////////////////////////////////////////////////////////////////

ezDuplicateObjectsCommand::ezDuplicateObjectsCommand()
{
  m_uiNumberOfCopies = 0;
  m_vAccumulativeTranslation.SetZero();
  m_vAccumulativeRotation.SetZero();
  m_vRandomRotation.SetZero();
  m_vRandomTranslation.SetZero();
  m_iRevolveAxis = 0;
  m_fRevolveRadius = 0.0f;
  m_bGroupDuplicates = false;
}

ezStatus ezDuplicateObjectsCommand::DoInternal(bool bRedo)
{
  ezSceneDocument* pDocument = static_cast<ezSceneDocument*>(GetDocument());

  if (!bRedo)
  {
    EZ_ASSERT_DEV(!m_bGroupDuplicates, "Not yet implemented");

    ezAbstractObjectGraph graph;
    DeserializeGraph(graph);

    if (m_uiNumberOfCopies == 0)
    {
      ezTempHybridArray<ezDocument::PasteInfo, 16> ToBePasted;
      CreateOneDuplicate(graph, ToBePasted);
    }
    else
    {
      // store original selection
      m_OriginalSelection = m_pDocument->GetSelectionManager()->GetSelection();

      ezTempHybridArray<ezTempHybridArray<ezDocument::PasteInfo, 16>, 8> ToBePasted;
      ToBePasted.SetCount(m_uiNumberOfCopies);

      for (ezUInt32 copies = 0; copies < m_uiNumberOfCopies; ++copies)
      {
        CreateOneDuplicate(graph, ToBePasted[copies]);
      }

      ezTempHybridArray<CopyOffset, 8> offsets;
      ComputeCopyOffsets(offsets);

      for (ezUInt32 copies = 0; copies < m_uiNumberOfCopies; ++copies)
      {
        AdjustObjectPositions(ToBePasted[copies], offsets[copies]);
      }
    }


    if (m_DuplicatedObjects.IsEmpty())
      return ezStatus("Paste Objects: nothing was pasted!");
  }
  else
  {
    // Re-add at recorded place.
    for (auto& po : m_DuplicatedObjects)
    {
      pDocument->GetObjectManager()->AddObject(po.m_pObject, po.m_pParent, po.m_sParentProperty, po.m_Index);
    }
  }

  SetAsSelection();

  return ezStatus(EZ_SUCCESS);
}

void ezDuplicateObjectsCommand::SetAsSelection()
{
  if (!m_DuplicatedObjects.IsEmpty())
  {
    // m_DuplicatedObjects must stay in creation order, because redo re-adds the objects at their recorded indices,
    // which are only valid when they are inserted in the same order again
    ezTempHybridArray<DuplicatedObject, 4> sorted = m_DuplicatedObjects;
    sorted.Sort([](const auto& lhs, const auto& rhs)
      { return lhs.m_uiSelectionOrder < rhs.m_uiSelectionOrder; });

    auto pSelMan = m_pDocument->GetSelectionManager();

    ezDeque<const ezDocumentObject*> NewSelection = m_OriginalSelection;

    for (const DuplicatedObject& pi : sorted)
    {
      NewSelection.PushBack(pi.m_pObject);
    }

    pSelMan->SetSelection(NewSelection);
  }
}

void ezDuplicateObjectsCommand::DeserializeGraph(ezAbstractObjectGraph& graph)
{
  ezRawMemoryStreamReader memoryReader(m_sGraphTextFormat.GetData(), m_sGraphTextFormat.GetElementCount());
  ezAbstractGraphDdlSerializer::Read(memoryReader, &graph).IgnoreResult();
}

void ezDuplicateObjectsCommand::CreateOneDuplicate(ezAbstractObjectGraph& graph, ezDynamicArray<ezDocument::PasteInfo>& out_toBePasted)
{
  ezSceneDocument* pDocument = static_cast<ezSceneDocument*>(GetDocument());

  // Remap
  const ezUuid seed = ezUuid::MakeUuid();
  graph.ReMapNodeGuids(seed);

  ezDocumentObjectConverterReader reader(&graph, pDocument->GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateOnly);


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

  ezTempHybridArray<ezUInt32, 32> selectionOrder;

  auto& nodes = graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();

    if (pNode->GetNodeName() == "root")
    {
      auto* pNewObject = reader.CreateObjectFromNode(pNode);

      if (pNewObject)
      {
        reader.ApplyPropertiesToObject(pNode, pNewObject);

        selectionOrder.PushBack(0);

        if (auto* pProperty = pNode->FindProperty("__SelectionOrder"))
        {
          selectionOrder.PeekBack() = pProperty->m_Value.ConvertTo<ezUInt32>();
        }

        auto& ref = out_toBePasted.ExpandAndGetRef();
        ref.m_pObject = pNewObject;
        ref.m_pParent = nullptr;
        if (m_uiNumberOfCopies == 0 && m_iInsertIndex >= 0)
          ref.m_Index = m_iInsertIndex;

        const ezUuid guidParent = ParentGuids[pNode->GetGuid()];

        if (guidParent.IsValid())
          ref.m_pParent = pDocument->GetObjectManager()->GetObject(guidParent);
      }
    }
  }

  if (pDocument->DuplicateSelectedObjects(out_toBePasted, graph, false))
  {
    for (ezUInt32 i = 0; i < out_toBePasted.GetCount(); ++i)
    {
      const auto& item = out_toBePasted[i];
      auto& po = m_DuplicatedObjects.ExpandAndGetRef();
      po.m_pObject = item.m_pObject;
      po.m_Index = item.m_pObject->GetPropertyIndex();
      po.m_pParent = item.m_pParent;
      po.m_sParentProperty = item.m_pObject->GetParentProperty();
      po.m_uiSelectionOrder = selectionOrder[i];
    }
  }
  else
  {
    for (const auto& item : out_toBePasted)
    {
      pDocument->GetObjectManager()->DestroyObject(item.m_pObject);
    }
  }

  // undo uuid changes, so that we can do this again with another seed
  graph.ReMapNodeGuids(seed, true);
}


void ezDuplicateObjectsCommand::ComputeCopyOffsets(ezDynamicArray<CopyOffset>& out_offsets) const
{
  out_offsets.Clear();
  out_offsets.Reserve(m_uiNumberOfCopies);

  const ezUInt64 uiSeed = m_uiRandomSeed != 0 ? m_uiRandomSeed : (ezUInt64)ezTime::Now().GetNanoseconds();

  ezRandomGauss rngRotX, rngRotY, rngRotZ, rngTransX, rngTransY, rngTransZ;

  // SignedValue() returns values in [-max + 1; max - 1] and Initialize() requires max >= 2,
  // so the range is extended by one to include the requested value, and tiny values still produce some variation
  auto GetMaxValue = [](float fRange) -> ezUInt32
  { return ezMath::Max((ezUInt32)ezMath::Round(fRange), 1u) + 1; };

  if (m_vRandomRotation.x > 0)
    rngRotX.Initialize(uiSeed, GetMaxValue(m_vRandomRotation.x));
  if (m_vRandomRotation.y > 0)
    rngRotY.Initialize(uiSeed + 1, GetMaxValue(m_vRandomRotation.y));
  if (m_vRandomRotation.z > 0)
    rngRotZ.Initialize(uiSeed + 2, GetMaxValue(m_vRandomRotation.z));

  if (m_vRandomTranslation.x > 0)
    rngTransX.Initialize(uiSeed + 3, GetMaxValue(m_vRandomTranslation.x * 100));
  if (m_vRandomTranslation.y > 0)
    rngTransY.Initialize(uiSeed + 4, GetMaxValue(m_vRandomTranslation.y * 100));
  if (m_vRandomTranslation.z > 0)
    rngTransZ.Initialize(uiSeed + 5, GetMaxValue(m_vRandomTranslation.z * 100));

  for (ezUInt32 uiNumDuplicate = 0; uiNumDuplicate < m_uiNumberOfCopies; ++uiNumDuplicate)
  {
    const float fStep = (float)uiNumDuplicate;

    ezVec3 vRandT(0.0f);
    ezVec3 vRandR(0.0f);

    if (m_vRandomRotation.x > 0)
      vRandR.x = (float)rngRotX.SignedValue();
    if (m_vRandomRotation.y > 0)
      vRandR.y = (float)rngRotY.SignedValue();
    if (m_vRandomRotation.z > 0)
      vRandR.z = (float)rngRotZ.SignedValue();

    if (m_vRandomTranslation.x > 0)
      vRandT.x = rngTransX.SignedValue() / 100.0f;
    if (m_vRandomTranslation.y > 0)
      vRandT.y = rngTransY.SignedValue() / 100.0f;
    if (m_vRandomTranslation.z > 0)
      vRandT.z = rngTransZ.SignedValue() / 100.0f;

    ezVec3 vPosOffset(0.0f);

    if (m_iRevolveAxis > 0 && m_fRevolveRadius != 0.0f && m_RevolveAngleStep != ezAngle())
    {
      ezVec3 vRevolveAxis(0.0f);
      ezAngle revolve = m_RevolveStartAngle;

      switch (m_iRevolveAxis)
      {
        case 1:
          vRevolveAxis.Set(1, 0, 0);
          vPosOffset.Set(0, 0, m_fRevolveRadius);
          break;
        case 2:
          vRevolveAxis.Set(0, 1, 0);
          vPosOffset.Set(m_fRevolveRadius, 0, 0);
          break;
        case 3:
          vRevolveAxis.Set(0, 0, 1);
          vPosOffset.Set(0, m_fRevolveRadius, 0);
          break;
      }

      revolve += fStep * m_RevolveAngleStep;

      ezMat3 mRevolve = ezMat3::MakeAxisRotation(vRevolveAxis, revolve);

      vPosOffset = mRevolve * vPosOffset;
    }

    CopyOffset& offset = out_offsets.ExpandAndGetRef();
    offset.m_vPositionOffset = vPosOffset + (1.0f + fStep) * m_vAccumulativeTranslation + vRandT;
    offset.m_qRotation = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(fStep * m_vAccumulativeRotation.x + vRandR.x), ezAngle::MakeFromDegree(fStep * m_vAccumulativeRotation.y + vRandR.y), ezAngle::MakeFromDegree(fStep * m_vAccumulativeRotation.z + vRandR.z));
  }
}

void ezDuplicateObjectsCommand::AdjustObjectPositions(const ezArrayPtr<ezDocument::PasteInfo>& duplicates, const CopyOffset& offset)
{
  ezSceneDocument* pScene = static_cast<ezSceneDocument*>(m_pDocument);

  for (const auto& pi : duplicates)
  {
    ezTransform tGlobal = pScene->GetGlobalTransform(pi.m_pObject);

    tGlobal.m_vScale.Set(1.0f);
    tGlobal.m_vPosition += offset.m_vPositionOffset;
    tGlobal.m_qRotation = offset.m_qRotation * tGlobal.m_qRotation;

    /// \todo Christopher: Modifying the position through a command after creating the object seems to destroy the undo-ability of this
    /// operation Duplicating multiple objects (with some translation) and then undoing that will crash the editor process

    pScene->SetGlobalTransform(pi.m_pObject, tGlobal, TransformationChanges::Translation | TransformationChanges::Rotation);
  }
}

ezStatus ezDuplicateObjectsCommand::UndoInternal(bool bFireEvents)
{
  EZ_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  ezDocument* pDocument = GetDocument();

  // The duplicates are usually selected. Removing a selected object sends a selection event for each object, which is slow for many
  // duplicates, so deselect all of them at once first.
  {
    ezHashSet<const ezDocumentObject*> duplicates;
    for (const auto& po : m_DuplicatedObjects)
    {
      duplicates.Insert(po.m_pObject);
    }

    ezDeque<const ezDocumentObject*> newSelection;
    for (const ezDocumentObject* pObject : pDocument->GetSelectionManager()->GetSelection())
    {
      if (!duplicates.Contains(pObject))
        newSelection.PushBack(pObject);
    }

    if (newSelection.GetCount() != pDocument->GetSelectionManager()->GetSelection().GetCount())
    {
      pDocument->GetSelectionManager()->SetSelection(newSelection);
    }
  }

  for (auto& po : m_DuplicatedObjects)
  {
    EZ_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(po.m_pObject));

    pDocument->GetObjectManager()->RemoveObject(po.m_pObject);
  }

  return ezStatus(EZ_SUCCESS);
}

void ezDuplicateObjectsCommand::CleanupInternal(CommandState state)
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
