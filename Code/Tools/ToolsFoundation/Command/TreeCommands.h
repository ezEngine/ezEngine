#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/Document/Document.h>

class EZ_TOOLSFOUNDATION_DLL ezAddObjectCommand : public ezCommandBase
{
public:
  ezAddObjectCommand(ezReflectedTypeHandle hType, const ezUuid& parent, ezInt32 iChildIndex);

private:
  virtual ezStatus Do(bool bRedo) override;
  virtual ezStatus Undo() override;

private:
  ezReflectedTypeHandle m_hType;
  const ezUuid m_parent;
  ezInt32 m_iChildIndex;
  ezDocumentObjectBase* m_pObject;
};
