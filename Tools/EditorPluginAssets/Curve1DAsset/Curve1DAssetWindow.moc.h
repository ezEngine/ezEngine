#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtCurve1DEditorWidget;

class ezQtCurve1DAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtCurve1DAssetDocumentWindow(ezDocument* pDocument);
  ~ezQtCurve1DAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const { return "Curve1DAsset"; }

private slots:
  void onInsertCpAt(float clickPosX, float clickPosY, float epsilon);
  void onCurveCpMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY);
  void onCurveCpDeleted(ezUInt32 curveIdx, ezUInt32 cpIdx);
  void onCurveTangentMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);
  void onCurveNormalizeX();
  void onCurveNormalizeY();

  void onCurveBeginOperation(QString name);
  void onCurveEndOperation(bool commit);
  void onCurveBeginCpChanges(QString name);
  void onCurveEndCpChanges();

private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  bool PickCurveAt(float x, float y, float fMaxYDistance, ezInt32& out_iCurveIdx, float& out_ValueY) const;
  bool PickControlPointAt(float x, float y, float fMaxDistance, ezInt32& out_iCurveIdx, ezInt32& out_iCpIdx) const;

  ezQtCurve1DEditorWidget* m_pCurveEditor;
};
