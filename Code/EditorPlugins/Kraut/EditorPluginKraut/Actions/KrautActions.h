#pragma once

#include <EditorPluginKraut/EditorPluginKrautDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class ezKrautTreeAssetDocument;
struct ezKrautTreeAssetEvent;

/// Wind strength presets available in the Kraut tree asset editor preview.
struct ezKrautWindStrength
{
  enum Enum
  {
    Off,
    Light,
    Moderate,
    Strong,

    Default = Light
  };
};

class ezKrautActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(ezStringView sMapping);

  static ezActionDescriptorHandle s_hCategory;
  static ezActionDescriptorHandle s_hWindStrengthMenu;
  static ezActionDescriptorHandle s_hWindStrength[4];
  static ezActionDescriptorHandle s_hToggleFrondsLeaves;
};

/// Button action used in the Kraut tree asset editor toolbar.
class ezKrautAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautAction, ezButtonAction);

public:
  enum class ActionType
  {
    WindStrength,       ///< Sets the preview wind strength to a specific ezKrautWindStrength preset.
    ToggleFrondsLeaves, ///< Toggles visibility of frond and leaf geometry in the preview.
  };

  ezKrautAction(const ezActionContext& context, const char* szName, ActionType type, ezKrautWindStrength::Enum windStrength = ezKrautWindStrength::Light);
  ~ezKrautAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void KrautEventHandler(const ezKrautTreeAssetEvent& e);
  void UpdateState();

  ezKrautTreeAssetDocument* m_pDocument = nullptr;
  ActionType m_Type;
  ezKrautWindStrength::Enum m_WindStrength = ezKrautWindStrength::Default;
};
