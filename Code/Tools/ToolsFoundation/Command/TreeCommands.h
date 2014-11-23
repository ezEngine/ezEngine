#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/Document/Document.h>

class EZ_TOOLSFOUNDATION_DLL ezAddObjectCommand : public ezCommandBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAddObjectCommand);

public:
  ezAddObjectCommand();

public: // Properties
  void SetType(const char* szType);
  const char* GetType() const;

  ezReflectedTypeHandle m_hType;
  ezUuid m_Parent;
  ezInt32 m_iChildIndex;
  ezVariant m_Variant;

private:
  virtual ezStatus Do(bool bRedo) override;
  virtual ezStatus Undo() override;
  virtual void Cleanup(CommandState state) override;

private:
  ezDocumentObjectBase* m_pObject;
};

class EZ_TOOLSFOUNDATION_DLL ezRemoveObjectCommand : public ezCommandBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRemoveObjectCommand);

public:
  ezRemoveObjectCommand();

public: // Properties
  ezUuid m_Object;

private:
  virtual ezStatus Do(bool bRedo) override;
  virtual ezStatus Undo() override;
  virtual void Cleanup(CommandState state) override;

private:
  ezDocumentObjectBase* m_pParent;
  ezInt32 m_iChildIndex;
  ezDocumentObjectBase* m_pObject;
};
