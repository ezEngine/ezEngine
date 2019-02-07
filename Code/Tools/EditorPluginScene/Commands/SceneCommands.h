#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>
#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/Document/Document.h>

class ezRandomGauss;

class ezDuplicateObjectsCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDuplicateObjectsCommand, ezCommand);

public:
  ezDuplicateObjectsCommand();

public: // Properties
  ezString m_sGraphTextFormat;
  ezString m_sParentNodes; /// A stringyfied map in format "uuidObj1=uuidParent1;..." that defines the previous parents of all top level objects

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

private:
  virtual ezStatus DoInternal(bool bRedo) override;

  void SetAsSelection();

  void DeserializeGraph(ezAbstractObjectGraph& graph);

  void CreateOneDuplicate(ezAbstractObjectGraph& graph, ezHybridArray<ezDocument::PasteInfo, 16>& ToBePasted);
  void AdjustObjectPositions(ezHybridArray<ezDocument::PasteInfo, 16>& Duplicates, ezUInt32 uiNumDuplicate, ezRandomGauss& rngRotX, ezRandomGauss& rngRotY, ezRandomGauss& rngRotZ, ezRandomGauss& rngTransX, ezRandomGauss& rngTransY, ezRandomGauss& rngTransZ);

  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  struct DuplicatedObject
  {
    ezDocumentObject* m_pObject;
    ezDocumentObject* m_pParent;
    ezString m_sParentProperty;
    ezVariant m_Index;
  };

  ezHybridArray<DuplicatedObject, 4> m_DuplicatedObjects;
};

