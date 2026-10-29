#pragma once

#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class ezRandomGauss;

class ezDuplicateObjectsCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDuplicateObjectsCommand, ezCommand);

public:
  ezDuplicateObjectsCommand();

public:                        // Properties
  ezString m_sGraphTextFormat;
  ezString m_sParentNodes;     /// A stringyfied map in format "uuidObj1=uuidParent1;..." that defines the previous parents of all top level objects

  ezUInt32 m_uiNumberOfCopies; /// if set to 0 (the default), all the 'advanced' duplication code is skipped and only a single straight copy is made

  ezVec3 m_vAccumulativeTranslation;
  ezVec3 m_vAccumulativeRotation;
  ezVec3 m_vRandomRotation;
  ezVec3 m_vRandomTranslation;
  bool m_bGroupDuplicates;

  ezInt8 m_iRevolveAxis; ///< 0 = disabled, 1 = x, 2 = y, 3 = z
  float m_fRevolveRadius;
  ezAngle m_RevolveStartAngle;
  ezAngle m_RevolveAngleStep;

  /// When duplicating a single object, this is set to the original object's index + 1, so the duplicate ends up right after the original.
  /// A value of -1 (the default) appends to the end of the parent's children list.
  ezInt32 m_iInsertIndex = -1;

  /// Seed for the random translation and rotation. If zero, the current time is used.
  ezUInt64 m_uiRandomSeed = 0;

  struct CopyOffset
  {
    ezVec3 m_vPositionOffset;
    ezQuat m_qRotation;
  };

  /// Computes how each of the m_uiNumberOfCopies copies is placed.
  ///
  /// A copy gets the global position of its original plus m_vPositionOffset and the global rotation of its original pre-multiplied by m_qRotation.
  /// With a non-zero m_uiRandomSeed the result is deterministic, which ezQtDuplicateDlg uses to preview the duplicates.
  void ComputeCopyOffsets(ezDynamicArray<CopyOffset>& out_offsets) const;

private:
  virtual ezStatus DoInternal(bool bRedo) override;

  void SetAsSelection();

  void DeserializeGraph(ezAbstractObjectGraph& graph);

  void CreateOneDuplicate(ezAbstractObjectGraph& graph, ezDynamicArray<ezDocument::PasteInfo>& out_toBePasted);
  void AdjustObjectPositions(const ezArrayPtr<ezDocument::PasteInfo>& duplicates, const CopyOffset& offset);

  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  struct DuplicatedObject
  {
    ezDocumentObject* m_pObject;
    ezDocumentObject* m_pParent;
    ezString m_sParentProperty;
    ezVariant m_Index;
    ezUInt32 m_uiSelectionOrder = 0;
  };

  ezDeque<const ezDocumentObject*> m_OriginalSelection;
  ezHybridArray<DuplicatedObject, 4> m_DuplicatedObjects;
};
