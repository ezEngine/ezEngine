#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class ezSkeletonAssetDocument;
struct ezSkeletonAssetEvent;

class ezSkeletonActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hCategory;
  static ezActionDescriptorHandle s_hRenderBones;
  static ezActionDescriptorHandle s_hRenderColliders;
  static ezActionDescriptorHandle s_hRenderJoints;
  static ezActionDescriptorHandle s_hRenderSwingLimits;
  static ezActionDescriptorHandle s_hRenderTwistLimits;
  static ezActionDescriptorHandle s_hRenderPreviewMesh;
};

class ezSkeletonAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkeletonAction, ezButtonAction);

public:
  enum class ActionType
  {
    RenderBones,
    RenderColliders,
    RenderJoints,
    RenderSwingLimits,
    RenderTwistLimits,
    RenderPreviewMesh,
  };

  ezSkeletonAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezSkeletonAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void AssetEventHandler(const ezSkeletonAssetEvent& e);
  void UpdateState();

  ezSkeletonAssetDocument* m_pSkeletonpDocument = nullptr;
  ActionType m_Type;
};
