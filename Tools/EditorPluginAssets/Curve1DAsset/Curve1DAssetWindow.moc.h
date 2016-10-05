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
  void onCurveCpMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY);
  void onCurveCpDeleted(ezUInt32 curveIdx, ezUInt32 cpIdx);
  void onCurveTangentMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);
  void onCurveNormalizeX();
  void onCurveNormalizeY();

  void onCurveBeginOperation();
  void onCurveEndOperation(bool commit);
  void onCurveBeginCpChanges();
  void onCurveEndCpChanges();

private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);

  ezQtCurve1DEditorWidget* m_pCurveEditor;
};