#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <EditorPluginAssets/Util/MeshLodCreator.h>
#include <EditorPluginAssets/ui_CreateMeshLodsDlg.h>

#include <GuiFoundation/Dialogs/Dialog.moc.h>

/// Configures what ezMeshLodCreator::CreateMeshLods() should generate.
///
/// Also used for several meshes at once, in which case the folder and the resulting simplification
/// are decided per mesh and only the number of LODs is shown.
class ezQtCreateMeshLodsDlg : public ezQtDialog, public Ui_CreateMeshLodsDlg
{
  Q_OBJECT

public:
  /// For a single mesh, whose folder and simplification ladder are shown.
  ezQtCreateMeshLodsDlg(const ezMeshLodSource& source, QWidget* pParent);

  /// For several meshes.
  ezQtCreateMeshLodsDlg(ezUInt32 uiMeshCount, QWidget* pParent);

  const ezMeshLodOptions& GetOptions() const { return m_Options; }

private Q_SLOTS:
  void on_LodCount_valueChanged(int value);
  void on_OverwriteExisting_toggled(bool checked);
  void on_ButtonBox_accepted();
  void on_ButtonBox_rejected();

private:
  /// Everything both constructors do, before either fills in what is specific to its case.
  void Setup();

  /// Spells out the simplification each LOD will get, which depends on the mesh's own simplification.
  void UpdateInfo();

  /// Null when several meshes were selected, as none of them speaks for the others.
  const ezMeshLodSource* m_pSource = nullptr;

  /// 1 unless several meshes were selected.
  ezUInt32 m_uiMeshCount = 1;

  ezMeshLodOptions m_Options;

  /// Above this many meshes the "open after creation" box starts out unticked. It stays available.
  static constexpr ezUInt32 s_uiMaxAutoOpen = 5;

  // remembered across invocations
  static ezInt32 s_iLodCount;
  static bool s_bOpenAfterCreate;
};
