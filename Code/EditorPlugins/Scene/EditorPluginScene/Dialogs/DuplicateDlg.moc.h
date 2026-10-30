#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/ui_DuplicateDlg.h>

#include <GuiFoundation/Dialogs/Dialog.moc.h>

class ezSceneDocument;
class ezDocumentObject;
class ezDuplicateObjectsCommand;

class ezQtDuplicateDlg : public ezQtDialog, public Ui_DuplicateDlg
{
  Q_OBJECT

public:
  ezQtDuplicateDlg(QWidget* pParent, ezSceneDocument* pSceneDoc);
  ~ezQtDuplicateDlg();

  static ezUInt32 s_uiNumberOfCopies;
  static bool s_bGroupCopies;
  static ezVec3 s_vTranslationStep;
  static ezVec3 s_vRotationStep;
  static ezVec3 s_vRandomTranslation;
  static ezVec3 s_vRandomRotation;
  static int s_iRevolveStartAngle;
  static int s_iRevolveAngleStep;
  static float s_fRevolveRadius;
  static int s_iRevolveAxis;

public Q_SLOTS:
  virtual void on_DefaultButtons_clicked(QAbstractButton* pButton);

  virtual void on_toolButtonTransX_clicked();
  virtual void on_toolButtonTransY_clicked();
  virtual void on_toolButtonTransZ_clicked();

  virtual void on_RevolveNone_clicked();
  virtual void on_RevolveX_clicked();
  virtual void on_RevolveY_clicked();
  virtual void on_RevolveZ_clicked();

private:
  void QueryUI();
  void FillCommandSettings(ezDuplicateObjectsCommand& ref_cmd) const;
  void Apply();
  void Undo();
  void UpdateUndoButton();

  /// Sends indicators to the engine process to show where the duplicates would end up.
  ///
  /// Objects with bounds are shown as a box, all others as an arrow.
  void UpdatePreview();
  void PickNewRandomSeed();

  /// Takes the current top-level selection as the objects to duplicate and queries their bounds.
  ///
  /// Called again after every Apply and Undo, since those change the selection.
  void UpdateSourceObjects();

  /// Computes m_vBoundingBoxSize from m_ObjectBounds and shows the buttons that set the translation step to it.
  void SetupBoundingBoxHelper();

  ezVec3 m_vBoundingBoxSize;

  ezSceneDocument* m_pSceneDocument = nullptr;
  QPushButton* m_pButtonUndo = nullptr;
  ezUInt32 m_uiActionsApplied = 0;
  ezUInt64 m_uiRandomSeed = 0;

  ezHybridArray<const ezDocumentObject*, 64> m_TopLevelObjects;
  ezHybridArray<ezBoundingBox, 64> m_ObjectBounds; ///< Same order as m_TopLevelObjects.
};
