#pragma once

#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/BaseActions.h>

//////////////////////////////////////////////////////////////////////////

class ezRecastAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRecastAction, ezButtonAction);

public:
  enum class ButtonType
  {
    GenerateNavMesh,
  };

  ezRecastAction(const ezActionContext& context, const char* szName, ButtonType button);
  ~ezRecastAction();

  virtual void Execute(const ezVariant& value) override;

private:
  ButtonType m_ButtonType;
};

//////////////////////////////////////////////////////////////////////////

struct ezRecastActions
{
  static ezActionDescriptorHandle s_hRecastMenu;
  static ezActionDescriptorHandle s_hNavMeshCategory;
  static ezActionDescriptorHandle s_hGenerateNavMesh;

  static void RegisterActions();
  static void UnregisterActions();
  static void MapActions(const char* szMapping);
};
