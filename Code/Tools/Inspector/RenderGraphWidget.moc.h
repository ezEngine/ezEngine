#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/String.h>
#include <Inspector/RenderGraphHistogramWidget.moc.h>
#include <Inspector/RenderGraphOverviewWidget.moc.h>
#include <Inspector/RenderGraphPreviewWidget.moc.h>
#include <Inspector/ui_RenderGraphWidget.h>
#include <RendererCore/RenderGraph/RenderGraphInspectionInfo.h>
#include <RendererCore/RenderGraph/RenderGraphPassObserver.h>
#include <ads/DockWidget.h>

class QHideEvent;
class QShowEvent;

/// Dock widget that coordinates render graph inspection telemetry and preview controls.
class ezQtRenderGraphWidget : public ads::CDockWidget, public Ui_RenderGraphWidget
{
  Q_OBJECT

public:
  ezQtRenderGraphWidget(ads::CDockManager* pDockManager, QWidget* pParent = nullptr);

  static ezQtRenderGraphWidget* s_pWidget;
  static void ProcessTelemetry(void* pUnused);

  void ResetStats();
  void UpdateStats();

protected:
  void showEvent(QShowEvent* pEvent) override;
  void hideEvent(QHideEvent* pEvent) override;

private:
  void SendSummaryRequest();
  void SendInfoRequest();
  void SendObserverRequest();
  void SendObserverRequest(const ezRenderGraphObserverRequest& request);
  void PauseObservation();
  void ResumeObservation();
  void UpdateObservationVisibility();
  void UpdateGraphList();
  void UpdateSwapChainList();
  void UpdateInfoWidgets();
  void UpdateRequestControls();
  void SelectAccess(ezUInt16 uiPassIndex, ezUInt16 uiAccessIndex);
  void ClearAccessSelection();
  void UpdatePreviewRequest(float fZoom, ezVec2 panCenter, ezVec2I32 pixel, bool bUpdatePixelPosition, bool bHighlightPixel);
  void SetRequestFromControls();

private:
  ezRenderGraphInspectionSummary m_Summary;
  ezRenderGraphInspectionInfo m_Info;
  ezRenderGraphObserverRequest m_Request;
  ezRenderGraphObserverResponse m_Response;
  ezString m_sLastPixelValue;
  ezVec2I32 m_vLastPixelPosition = ezVec2I32(-1, -1);
  ezStaticArray<ezUInt8, 1024> m_LastHistogram;
  ezUInt64 m_uiSelectedGraphId = 0;
  ezUInt32 m_uiSelectedSwapChainId = 0xFFFFFFFF;
  bool m_bHasLastPixelValue = false;
  bool m_bHasLastHistogram = false;
  bool m_bInfoValid = false;
  bool m_bUpdateUi = true;
  bool m_bUpdatingControls = false;
  bool m_bObservationPaused = false;
};
