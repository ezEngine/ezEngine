#pragma once

#include <EditorPluginParticle/Plugin.h>
#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>

class ezParticleEffectAssetDocument;

class ezParticleActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hCategory;
  static ezActionDescriptorHandle s_hRestartEffect;
};

class ezParticleAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleAction, ezButtonAction);

public:

  enum class ActionType
  {
    RestartEffect,
  };

  ezParticleAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezParticleAction();

  virtual void Execute(const ezVariant& value) override;

private:
  ezParticleEffectAssetDocument* m_pEffectDocument;
  ActionType m_Type;
};




