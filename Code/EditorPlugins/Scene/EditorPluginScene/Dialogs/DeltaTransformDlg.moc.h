#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/ui_DeltaTransformDlg.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class ezSceneDocument;
class ezDocumentObject;

class ezQtDeltaTransformDlg : public ezQtDialog, public Ui_DeltaTransformDlg
{
  Q_OBJECT

public:
  ezQtDeltaTransformDlg(QWidget* pParent, ezSceneDocument* pSceneDoc);
  ~ezQtDeltaTransformDlg();

  enum Mode
  {
    Translate,
    TranslateDeviation,
    RotateX,
    RotateXRandom,
    RotateXDeviation,
    RotateY,
    RotateYRandom,
    RotateYDeviation,
    RotateZ,
    RotateZRandom,
    RotateZDeviation,
    Scale,
    ScaleDeviation,
    UniformScale,
    UniformScaleDeviation,
    NaturalDeviationZ,
  };

  enum Space
  {
    World,
    LocalSelection,
    LocalEach,
  };

private Q_SLOTS:
  void on_ButtonApply_clicked();
  void on_ButtonUndo_clicked();
  void on_ComboMode_currentIndexChanged(int index);
  void on_ComboSpace_currentIndexChanged(int index);
  void on_Value1_valueChanged(double value);
  void on_Value2_valueChanged(double value);
  void on_Value3_valueChanged(double value);
  void on_CheckBoxSnapping_stateChanged(int state);

private:
  struct NewTransform
  {
    const ezDocumentObject* m_pObject = nullptr;
    ezUInt32 m_uiSelectionIndex = 0; ///< Index into the top-level selection and m_ObjectBounds.
    ezTransform m_Transform;
  };

  void QueryUI();
  void UpdateUI();

  /// Computes the global transforms that the selected objects get when the current settings are applied.
  ///
  /// Returns the TransformationChanges flags to pass to SetGlobalTransform.
  /// The random modes are deterministic for the same m_uiRandomSeed, so the preview matches what gets applied.
  ezUInt8 ComputeNewTransforms(ezDynamicArray<NewTransform>& out_NewTransforms) const;

  /// Sends indicators to the engine process to show where the selected objects would end up.
  ///
  /// Objects with valid bounds (see QueryObjectBounds()) are shown as a box, all others as an arrow.
  void UpdatePreview();

  /// Fills m_ObjectBounds with the bounds of the top-level selection.
  ///
  /// The selection can't change while the modal dialog is open, so this is only done once.
  void QueryObjectBounds();
  void PickNewRandomSeed();

  static Mode s_Mode;
  static Space s_Space;
  static ezVec3 s_vTranslate;
  static ezVec3 s_vTranslateDeviation;
  static ezVec3 s_vScale;
  static ezVec3 s_vScaleDeviation;
  static float s_fUniformScale;
  static float s_fUniformScaleDeviation;
  static ezVec3 s_vRotate;
  static ezVec3 s_vRotateRandom;
  static ezVec3 s_vRotateDeviation;
  static float s_fNaturalDeviationZ;
  static bool s_bUseCurrentSnapSettings;

  ezUInt32 m_uiActionsApplied = 0;
  ezUInt32 m_uiRandomSeed = 0;
  ezHybridArray<ezBoundingBox, 4> m_ObjectBounds; ///< Same order as the top-level selection.
  ezSceneDocument* m_pSceneDocument = nullptr;
};
