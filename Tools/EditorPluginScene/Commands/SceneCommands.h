#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/Document/Document.h>

class ezDuplicateObjectsCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDuplicateObjectsCommand, ezCommand);

public:
  ezDuplicateObjectsCommand();

public: // Properties
  ezString m_sJsonGraph;
  ezString m_sParentNodes;

  ezUInt32 m_uiNumberOfCopies;
  //bool m_bStartAtCenter = false;
  //bool m_bGroupCopies = true;
  //ezVec3 m_vTranslation(0, 0, 0);
  //ezVec3 m_vRotation(0, 0, 0);
  //ezVec3 m_vTranslationGauss(0, 0, 0);
  //ezVec3 m_vRotationGauss(0, 0, 0);
  //int m_iStartAngle = 0;
  //int m_iRevolveAngle = 0;
  //float m_fRevolveRadius = 1.0f;
  //int m_iRevolveAxis = 0;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
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

