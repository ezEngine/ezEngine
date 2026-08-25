#pragma once

#include <EditorPluginJolt/EditorPluginJoltDLL.h>
#include <EditorPluginJolt/Utils/MeshColliderCreator.h>
#include <EditorPluginJolt/ui_CreateMeshColliderDlg.h>

#include <GuiFoundation/Dialogs/Dialog.moc.h>

/// Configures what ezMeshColliderCreator::CreateMeshCollider() should generate.
///
/// Also used for several meshes at once, in which case there is no single output path to configure:
/// each collider goes next to its own mesh, and the path field only says so.
/// \see ezMeshColliderCreator::CreateMeshColliders()
class ezQtCreateMeshColliderDlg : public ezQtDialog, public Ui_CreateMeshColliderDlg
{
  Q_OBJECT

public:
  /// For a single mesh, whose settings are shown and whose path can be edited.
  ezQtCreateMeshColliderDlg(const ezMeshColliderSource& source, QWidget* pParent);

  /// For several meshes. Nothing mesh specific is shown, and the path is fixed to the default.
  ezQtCreateMeshColliderDlg(ezUInt32 uiMeshCount, QWidget* pParent);

  /// The path is empty for the multi-mesh case, which is what tells the creator to use the default.
  const ezMeshColliderOptions& GetOptions() const { return m_Options; }

private Q_SLOTS:
  void on_BrowseButton_clicked();
  void on_ColliderPath_textChanged(const QString& text);
  void on_ColliderType_currentIndexChanged(int index);
  void on_OverwriteExisting_toggled(bool checked);
  void on_SurfaceButton_clicked();
  void on_SurfaceClearButton_clicked();
  void on_ButtonBox_accepted();
  void on_ButtonBox_rejected();

private:
  /// Everything both constructors do, before either fills in what is specific to its case.
  ///
  /// The collider kind starts on what was picked last.
  void Setup();

  ezEnum<ezMeshColliderKind> GetSelectedKind() const;

  /// Puts the suggested path for the currently selected collider type into the line edit, unless
  /// the user has typed their own path. Does nothing without a single source mesh.
  void UpdateSuggestedPath();
  void UpdateInfo();

  /// Null when several meshes were selected, as none of them speaks for the others.
  const ezMeshColliderSource* m_pSource = nullptr;

  /// 1 unless several meshes were selected.
  ezUInt32 m_uiMeshCount = 1;

  ezMeshColliderOptions m_Options;

  /// The surface asset, as a guid string. Kept separately because the line edit shows its path.
  ezString m_sSurface;

  /// False once the path was edited by hand, which stops the type combo from overwriting it.
  bool m_bPathIsSuggestion = true;

  /// Set while the path is filled in from code, so that this is not mistaken for the user typing.
  bool m_bSettingPath = false;

  /// Above this many meshes the "open after creation" box starts out unticked. It stays available.
  static constexpr ezUInt32 s_uiMaxAutoOpen = 5;

  // remembered across invocations
  static bool s_bOpenAfterCreate;
  static ezEnum<ezMeshColliderKind> s_LastKind;
};
