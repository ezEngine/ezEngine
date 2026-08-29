#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/Utils/MeshPrefabCreator.h>
#include <EditorPluginScene/ui_CreateMeshPrefabDlg.h>

#include <GuiFoundation/Dialogs/Dialog.moc.h>

/// Configures what ezMeshPrefabCreator::CreateMeshPrefab() should generate.
///
/// Options the mesh can't support are disabled rather than hidden, with a tooltip saying why.
///
/// Also used for several meshes at once, in which case there is no single output path or render
/// component to configure: each prefab goes next to its own mesh, and each gets the component that
/// suits it. \see ezMeshPrefabCreator::CreateMeshPrefabs()
class ezQtCreateMeshPrefabDlg : public ezQtDialog, public Ui_CreateMeshPrefabDlg
{
  Q_OBJECT

public:
  /// For a single mesh, whose LODs are shown and whose path can be edited.
  ezQtCreateMeshPrefabDlg(const ezMeshPrefabSource& source, QWidget* pParent);

  /// For several meshes. Nothing mesh specific is shown, and the path is fixed to the default.
  ezQtCreateMeshPrefabDlg(ezUInt32 uiMeshCount, QWidget* pParent);

  /// The path and the render component type are empty for the multi-mesh case, which is what tells
  /// the creator to decide both per mesh.
  const ezMeshPrefabOptions& GetOptions() const { return m_Options; }

private Q_SLOTS:
  void on_BrowseButton_clicked();
  void on_PrefabPath_textChanged(const QString& text);
  void on_RenderComponent_currentIndexChanged(int index);
  void on_OverwriteExisting_toggled(bool checked);
  void on_ButtonBox_accepted();
  void on_ButtonBox_rejected();

private:
  /// Everything both constructors do, before either fills in what is specific to its case.
  void Setup();

  void UpdateWarnings();

  /// The LODs are only used by the LOD components, so what this says depends on the selected
  /// component type as much as on what was found next to the mesh.
  void UpdateLodInfo();

  /// Puts the suggested path into the line edit, unless the user has typed or browsed to their own.
  /// Does nothing without a single source mesh.
  void UpdateSuggestedPath();

  /// Null when several meshes were selected, as none of them speaks for the others.
  const ezMeshPrefabSource* m_pSource = nullptr;

  /// 1 unless several meshes were selected.
  ezUInt32 m_uiMeshCount = 1;

  /// False once the path was edited by hand, which stops the overwrite box from overwriting it.
  bool m_bPathIsSuggestion = true;

  /// Set while the path is filled in from code, so that this is not mistaken for the user typing.
  bool m_bSettingPath = false;

  ezMeshPrefabOptions m_Options;

  /// Above this many meshes the "open after creation" box starts out unticked. It stays available.
  static constexpr ezUInt32 s_uiMaxAutoOpen = 5;

  // remembered across invocations
  static ezInt32 s_iPhysicsMode;
  static ezInt32 s_iCollisionLayer;
  static bool s_bOpenAfterCreate;
};
