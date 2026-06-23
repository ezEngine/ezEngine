#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <Inspector/RenderGraphWidget.moc.h>
#include <RendererFoundation/RendererReflection.h>

#include <QCheckBox>
#include <QHideEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPalette>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QToolButton>

namespace
{
  constexpr ezUInt32 s_uiSystemId = 'RGPH';
  constexpr ezUInt16 s_uiProtocolVersion = 1;

  enum GraphListItemDataRole
  {
    GraphListRole_RenderGraphId = Qt::UserRole,
    GraphListRole_IsGraphItem = Qt::UserRole + 1,
  };

  enum SwapChainListItemDataRole
  {
    SwapChainListRole_SwapChainId = Qt::UserRole,
    SwapChainListRole_Width = Qt::UserRole + 1,
    SwapChainListRole_Height = Qt::UserRole + 2,
  };

  QString MakeGraphLabel(const ezRenderGraphExecutionSummary& graph)
  {
    ezStringBuilder label;
    if (!graph.m_sUserName.IsEmpty())
      label.SetFormat("{} ({})", graph.m_sUserName, graph.m_sGraphName);
    else
      label = graph.m_sGraphName;

    if (graph.m_uiExecutionOrder >= 0)
      label.AppendFormat("  [{}]", graph.m_uiExecutionOrder);

    return ezMakeQString(label);
  }

  QString MakeSwapChainLabel(const ezRenderGraphSwapChainSummary& swapChain)
  {
    ezStringBuilder label;
    label.SetFormat("0x{}  {}x{}", ezArgU(swapChain.m_uiSwapChainId, 8, true, 16), swapChain.m_uiWidth, swapChain.m_uiHeight);
    return ezMakeQString(label);
  }

  QString MakePhaseLabel(ezRenderGraphPhase::Enum phase)
  {
    switch (phase)
    {
      case ezRenderGraphPhase::PreRender:
        return "PreRender";
      case ezRenderGraphPhase::Render:
        return "Render";
      case ezRenderGraphPhase::PostRender:
        return "PostRender";
      default:
        return "Unknown";
    }
  }

  void AddGraphPhaseSeparator(QListWidget* pList, ezRenderGraphPhase::Enum phase)
  {
    QListWidgetItem* pItem = new QListWidgetItem(MakePhaseLabel(phase));
    pItem->setFlags(Qt::NoItemFlags);
    pItem->setData(GraphListRole_IsGraphItem, false);
    pItem->setForeground(pList->palette().color(QPalette::Text));
    pItem->setBackground(pList->palette().color(QPalette::Mid));
    pList->addItem(pItem);
  }

} // namespace

ezQtRenderGraphWidget* ezQtRenderGraphWidget::s_pWidget = nullptr;

ezQtRenderGraphWidget::ezQtRenderGraphWidget(ads::CDockManager* pDockManager, QWidget* pParent)
  : ads::CDockWidget(pDockManager, "Render Graph", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(HorizontalSplitter);

  QPalette overviewScrollPalette = OverviewScroll->viewport()->palette();
  overviewScrollPalette.setColor(QPalette::Window, palette().color(QPalette::Dark));
  OverviewScroll->viewport()->setAutoFillBackground(true);
  OverviewScroll->viewport()->setPalette(overviewScrollPalette);

  connect(Overview, &ezQtRenderGraphOverviewWidget::AccessSelected, this, &ezQtRenderGraphWidget::SelectAccess);
  connect(Overview, &ezQtRenderGraphOverviewWidget::AccessDeselected, this, &ezQtRenderGraphWidget::ClearAccessSelection);
  connect(Preview, &ezQtRenderGraphPreviewWidget::RequestChanged, this, &ezQtRenderGraphWidget::UpdatePreviewRequest);

  auto requestFromControls = [this]()
  {
    if (!m_bUpdatingControls)
    {
      SetRequestFromControls();
      if (m_bHasLastHistogram)
      {
        Histogram->SetHistogram(m_LastHistogram.GetArrayPtr());
      }
      SendObserverRequest();
    }
  };

  auto setRange = [this](float fMin, float fMax)
  {
    m_bUpdatingControls = true;
    RangeMinSpin->setValue(fMin);
    RangeMaxSpin->setValue(fMax);
    m_bUpdatingControls = false;

    SetRequestFromControls();
    if (m_bHasLastHistogram)
    {
      Histogram->SetHistogram(m_LastHistogram.GetArrayPtr());
    }
    SendObserverRequest();
  };

  connect(GraphList, &QListWidget::currentRowChanged, this, [this](int row)
    {
    if (row < 0)
      return;
    QListWidgetItem* pItem = GraphList->item(row);
    if (pItem == nullptr || !pItem->data(GraphListRole_IsGraphItem).toBool())
      return;

    m_uiSelectedGraphId = pItem->data(GraphListRole_RenderGraphId).toULongLong();
    m_bInfoValid = false;
    m_Info.Clear();
    Overview->Clear();
    m_Request = ezRenderGraphObserverRequest();
    m_Request.m_uiRenderGraphId = m_uiSelectedGraphId;
    Preview->SetView(m_Request.m_fZoom, m_Request.m_vPanCenter);
    SendInfoRequest();
    m_bUpdateUi = true; });

  connect(SwapChainList, &QListWidget::currentRowChanged, this, [this](int row)
    {
    if (row < 0)
      return;
    QListWidgetItem* pItem = SwapChainList->item(row);
    m_uiSelectedSwapChainId = pItem->data(SwapChainListRole_SwapChainId).toUInt();
    m_Request.m_uiSwapChainId = m_uiSelectedSwapChainId;
    Preview->SetTargetSize(ezVec2U32(pItem->data(SwapChainListRole_Width).toUInt(), pItem->data(SwapChainListRole_Height).toUInt()));
    SendObserverRequest(); });

  connect(MipSpin, qOverload<int>(&QSpinBox::valueChanged), this, requestFromControls);
  connect(SliceSpin, qOverload<int>(&QSpinBox::valueChanged), this, requestFromControls);
  connect(SampleSpin, qOverload<int>(&QSpinBox::valueChanged), this, requestFromControls);
  connect(RangeMinSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, requestFromControls);
  connect(RangeMaxSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, requestFromControls);
  connect(ChannelR, &QCheckBox::toggled, this, requestFromControls);
  connect(ChannelG, &QCheckBox::toggled, this, requestFromControls);
  connect(ChannelB, &QCheckBox::toggled, this, requestFromControls);
  connect(ChannelA, &QCheckBox::toggled, this, requestFromControls);
  connect(ResetToolButton, &QToolButton::clicked, this, [setRange]()
    { setRange(0.0f, 1.0f); });
  connect(AutoToolButton, &QToolButton::clicked, this, [this, setRange]()
    { setRange(m_Response.m_fImageMin, m_Response.m_fImageMax); });
  connect(ResetZoomToolButton, &QToolButton::clicked, this, [this]()
    {
    m_Request.m_fZoom = 1.0f;
    m_Request.m_vPanCenter = ezVec2(0.5f);
    Preview->SetView(m_Request.m_fZoom, m_Request.m_vPanCenter);
    SendObserverRequest(); });

  ResetStats();
}

void ezQtRenderGraphWidget::ResetStats()
{
  m_Summary.m_AvailableSwapChains.Clear();
  m_Summary.m_RenderGraphs.Clear();
  m_Info.Clear();
  m_Request = ezRenderGraphObserverRequest();
  m_Response = ezRenderGraphObserverResponse();
  m_sLastPixelValue.Clear();
  m_vLastPixelPosition = ezVec2I32(-1, -1);
  m_LastHistogram.Clear();
  m_uiSelectedGraphId = 0;
  m_uiSelectedSwapChainId = 0xFFFFFFFF;
  m_bHasLastPixelValue = false;
  m_bHasLastHistogram = false;
  m_bInfoValid = false;
  m_bUpdateUi = true;
  Overview->Clear();
  Preview->SetTextureSize(ezVec2U32(0, 0));
  Histogram->Clear();
  MipSpin->setMaximum(0);
  MipSpin->setEnabled(false);
  SliceSpin->setMaximum(0);
  SliceSpin->setEnabled(false);
  SampleSpin->setMaximum(-1);
  SampleSpin->setEnabled(false);
  SendSummaryRequest();
}

void ezQtRenderGraphWidget::UpdateStats()
{
  UpdateObservationVisibility();

  if (!isVisible())
    return;

  if (!m_bUpdateUi)
    return;
  m_bUpdateUi = false;
  UpdateGraphList();
  UpdateSwapChainList();
  UpdateInfoWidgets();
}

void ezQtRenderGraphWidget::SendSummaryRequest()
{
  ezTelemetryMessage msg;
  msg.SetMessageID(s_uiSystemId, 'RSUM');
  msg.GetWriter() << s_uiProtocolVersion;
  ezTelemetry::SendToServer(msg);
}

void ezQtRenderGraphWidget::SendInfoRequest()
{
  if (m_uiSelectedGraphId == 0)
    return;
  ezTelemetryMessage msg;
  msg.SetMessageID(s_uiSystemId, 'RINF');
  msg.GetWriter() << s_uiProtocolVersion;
  msg.GetWriter() << m_uiSelectedGraphId;
  ezTelemetry::SendToServer(msg);
}

void ezQtRenderGraphWidget::SendObserverRequest()
{
  if (m_bObservationPaused)
    return;

  SendObserverRequest(m_Request);
}

void ezQtRenderGraphWidget::SendObserverRequest(const ezRenderGraphObserverRequest& request)
{
  if (request.m_uiRenderGraphId == 0)
    return;
  ezTelemetryMessage msg;
  msg.SetMessageID(s_uiSystemId, 'RREQ');
  msg.GetWriter() << s_uiProtocolVersion;
  msg.GetWriter() << request;
  ezTelemetry::SendToServer(msg);
}

void ezQtRenderGraphWidget::PauseObservation()
{
  if (m_bObservationPaused)
    return;

  m_bObservationPaused = true;

  ezRenderGraphObserverRequest pauseRequest = m_Request;
  pauseRequest.m_sPassName.Clear();
  pauseRequest.m_uiAccessIndex = 0;
  pauseRequest.m_vPixelPosition = ezVec2I32(-1, -1);
  pauseRequest.m_bHighlightPixel = false;
  SendObserverRequest(pauseRequest);
}

void ezQtRenderGraphWidget::ResumeObservation()
{
  if (!m_bObservationPaused)
    return;

  m_bObservationPaused = false;

  if (!m_Request.m_sPassName.IsEmpty())
  {
    SendObserverRequest();
  }
}

void ezQtRenderGraphWidget::UpdateObservationVisibility()
{
  if (isVisible())
  {
    ResumeObservation();
  }
  else
  {
    PauseObservation();
  }
}

void ezQtRenderGraphWidget::showEvent(QShowEvent* pEvent)
{
  ads::CDockWidget::showEvent(pEvent);
  UpdateObservationVisibility();
}

void ezQtRenderGraphWidget::hideEvent(QHideEvent* pEvent)
{
  ads::CDockWidget::hideEvent(pEvent);
  UpdateObservationVisibility();
}

void ezQtRenderGraphWidget::UpdateGraphList()
{
  const QSignalBlocker blocker(GraphList);
  GraphList->clear();
  ezInt32 iSelectedRow = -1;

  constexpr ezRenderGraphPhase::Enum phases[] = {ezRenderGraphPhase::PreRender, ezRenderGraphPhase::Render, ezRenderGraphPhase::PostRender};
  for (ezRenderGraphPhase::Enum phase : phases)
  {
    AddGraphPhaseSeparator(GraphList, phase);

    for (ezUInt32 i = 0; i < m_Summary.m_RenderGraphs.GetCount(); ++i)
    {
      const auto& graph = m_Summary.m_RenderGraphs[i];
      if (graph.m_Phase != phase)
        continue;

      const ezUInt64 uiIdentity = graph.m_uiRenderGraphId;
      QListWidgetItem* pItem = new QListWidgetItem(MakeGraphLabel(graph));
      pItem->setData(GraphListRole_RenderGraphId, QVariant::fromValue<qulonglong>(uiIdentity));
      pItem->setData(GraphListRole_IsGraphItem, true);
      pItem->setForeground(graph.m_uiExecutionOrder >= 0 ? ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Green)) : GraphList->palette().color(QPalette::Disabled, QPalette::Text));
      GraphList->addItem(pItem);
      if (uiIdentity == m_uiSelectedGraphId)
        iSelectedRow = GraphList->count() - 1;
    }
  }

  if (iSelectedRow >= 0)
    GraphList->setCurrentRow(iSelectedRow);
}

void ezQtRenderGraphWidget::UpdateSwapChainList()
{
  const QSignalBlocker blocker(SwapChainList);
  SwapChainList->clear();
  ezInt32 iSelectedRow = -1;
  for (ezUInt32 i = 0; i < m_Summary.m_AvailableSwapChains.GetCount(); ++i)
  {
    const ezRenderGraphSwapChainSummary& swapChain = m_Summary.m_AvailableSwapChains[i];
    const ezUInt32 uiIdentity = swapChain.m_uiSwapChainId;
    QListWidgetItem* pItem = new QListWidgetItem(MakeSwapChainLabel(swapChain));
    pItem->setData(SwapChainListRole_SwapChainId, uiIdentity);
    pItem->setData(SwapChainListRole_Width, swapChain.m_uiWidth);
    pItem->setData(SwapChainListRole_Height, swapChain.m_uiHeight);
    SwapChainList->addItem(pItem);
    if (uiIdentity == m_uiSelectedSwapChainId)
      iSelectedRow = (ezInt32)i;
  }

  // If the previously selected swap chain is no longer available (or none was selected yet), fall back to
  // the first one so that there is always a target selected as long as at least one swap chain exists.
  bool bSelectionChanged = false;
  if (iSelectedRow < 0 && SwapChainList->count() > 0)
  {
    iSelectedRow = 0;
    m_uiSelectedSwapChainId = SwapChainList->item(0)->data(SwapChainListRole_SwapChainId).toUInt();
    m_Request.m_uiSwapChainId = m_uiSelectedSwapChainId;
    bSelectionChanged = true;
  }

  if (iSelectedRow >= 0)
  {
    SwapChainList->setCurrentRow(iSelectedRow);
    Preview->SetTargetSize(ezVec2U32(SwapChainList->item(iSelectedRow)->data(SwapChainListRole_Width).toUInt(), SwapChainList->item(iSelectedRow)->data(SwapChainListRole_Height).toUInt()));
  }

  if (bSelectionChanged)
  {
    SendObserverRequest();
  }
}

void ezQtRenderGraphWidget::UpdateInfoWidgets()
{
  if (m_bInfoValid)
  {
    Overview->SetInfo(m_uiSelectedGraphId, m_Info);
    Overview->SetRequest(m_Request);
  }

  if (m_Response.m_PixelValue.IsValid())
  {
    m_sLastPixelValue = m_Response.m_PixelValue.ConvertTo<ezString>();
    m_vLastPixelPosition = m_Request.m_vPixelPosition;
    m_bHasLastPixelValue = true;
  }

  if (m_bHasLastPixelValue)
  {
    ezStringBuilder text;
    text.SetFormat("Pixel ({}, {}): {}", m_vLastPixelPosition.x, m_vLastPixelPosition.y, m_sLastPixelValue);
    PixelLabel->setText(ezMakeQString(text));
  }
  else
  {
    PixelLabel->setText("Pixel: -");
  }

  MinValue->setText(QString::number(m_Response.m_fImageMin, 'g', 9));
  MaxValue->setText(QString::number(m_Response.m_fImageMax, 'g', 9));

  if (m_Response.m_bHistogramValid)
  {
    m_LastHistogram = m_Response.m_Histogram;
    m_bHasLastHistogram = true;
  }

  if (m_bHasLastHistogram)
  {
    Histogram->SetHistogram(m_LastHistogram.GetArrayPtr());
  }
}

void ezQtRenderGraphWidget::UpdateRequestControls()
{
  m_bUpdatingControls = true;
  MipSpin->setValue(m_Request.m_uiMipLevel);
  SliceSpin->setValue(m_Request.m_uiArraySlice);
  SampleSpin->setValue(m_Request.m_iSampleIndex);
  RangeMinSpin->setValue(m_Request.m_fRangeMin);
  RangeMaxSpin->setValue(m_Request.m_fRangeMax);
  ChannelR->setChecked((m_Request.m_uiChannelMask & EZ_BIT(0)) != 0);
  ChannelG->setChecked((m_Request.m_uiChannelMask & EZ_BIT(1)) != 0);
  ChannelB->setChecked((m_Request.m_uiChannelMask & EZ_BIT(2)) != 0);
  ChannelA->setChecked((m_Request.m_uiChannelMask & EZ_BIT(3)) != 0);
  m_bUpdatingControls = false;
}

void ezQtRenderGraphWidget::UpdatePreviewRequest(float fZoom, ezVec2 panCenter, ezVec2I32 pixel, bool bUpdatePixelPosition, bool bHighlightPixel)
{
  m_Request.m_fZoom = fZoom;
  m_Request.m_vPanCenter = panCenter;
  if (bUpdatePixelPosition)
  {
    m_Request.m_vPixelPosition = pixel;
  }
  m_Request.m_bHighlightPixel = bHighlightPixel;
  SendObserverRequest();
}

void ezQtRenderGraphWidget::SetRequestFromControls()
{
  m_Request.m_uiMipLevel = static_cast<ezUInt8>(ezMath::Clamp(MipSpin->value(), 0, 255));
  m_Request.m_uiArraySlice = static_cast<ezUInt16>(ezMath::Clamp(SliceSpin->value(), 0, 65535));
  m_Request.m_iSampleIndex = static_cast<ezInt8>(ezMath::Clamp(SampleSpin->value(), -1, 127));
  m_Request.m_fRangeMin = (float)RangeMinSpin->value();
  m_Request.m_fRangeMax = (float)RangeMaxSpin->value();
  m_Request.m_uiChannelMask = 0;
  m_Request.m_uiChannelMask |= ChannelR->isChecked() ? EZ_BIT(0) : 0;
  m_Request.m_uiChannelMask |= ChannelG->isChecked() ? EZ_BIT(1) : 0;
  m_Request.m_uiChannelMask |= ChannelB->isChecked() ? EZ_BIT(2) : 0;
  m_Request.m_uiChannelMask |= ChannelA->isChecked() ? EZ_BIT(3) : 0;
  m_Request.m_uiSwapChainId = m_uiSelectedSwapChainId;
}

void ezQtRenderGraphWidget::SelectAccess(ezUInt16 uiPassIndex, ezUInt16 uiAccessIndex)
{
  if (uiPassIndex >= m_Info.m_Passes.GetCount())
    return;
  const ezRenderGraphInspectionInfo::AccessInfo* pSelectedAccess = nullptr;
  for (const auto& access : m_Info.m_Accesses)
  {
    if (access.m_bIsTexture && access.m_uiPassIndex == uiPassIndex && access.m_uiAccessIndex == uiAccessIndex)
    {
      pSelectedAccess = &access;
      break;
    }
  }
  if (pSelectedAccess == nullptr || pSelectedAccess->m_uiResourceIndex >= m_Info.m_Textures.GetCount())
    return;

  const auto& texture = m_Info.m_Textures[pSelectedAccess->m_uiResourceIndex];
  const ezInt32 iMipMax = ezMath::Max<ezInt32>(0, texture.m_Desc.m_uiMipLevelCount - 1);
  const ezInt32 iSliceMax = ezMath::Max<ezInt32>(0, texture.m_Desc.GetNumberOfSlices() - 1);
  const ezInt32 iSampleCount = (ezInt32)texture.m_Desc.m_SampleCount;
  const bool bHasMsaaSamples = texture.m_Desc.m_SampleCount != ezGALMSAASampleCount::None;
  MipSpin->setMaximum(iMipMax);
  MipSpin->setEnabled(iMipMax > 0);
  SliceSpin->setMaximum(iSliceMax);
  SliceSpin->setEnabled(iSliceMax > 0);
  SampleSpin->setMaximum(bHasMsaaSamples ? iSampleCount - 1 : -1);
  SampleSpin->setEnabled(bHasMsaaSamples);
  Preview->SetTextureSize(ezVec2U32(texture.m_Desc.m_uiWidth, texture.m_Desc.m_uiHeight));
  m_Request.m_uiRenderGraphId = m_uiSelectedGraphId;
  m_Request.m_sPassName = m_Info.m_Passes[uiPassIndex].m_sName;
  m_Request.m_uiAccessIndex = uiAccessIndex;
  m_Request.m_uiSwapChainId = m_uiSelectedSwapChainId;
  SetRequestFromControls();
  UpdateRequestControls();
  Overview->SetRequest(m_Request);
  SendObserverRequest();
}

void ezQtRenderGraphWidget::ClearAccessSelection()
{
  m_Request.m_uiRenderGraphId = m_uiSelectedGraphId;
  m_Request.m_sPassName.Clear();
  m_Request.m_uiAccessIndex = 0;
  m_Request.m_vPixelPosition = ezVec2I32(-1, -1);
  m_Request.m_bHighlightPixel = false;
  m_Request.m_uiSwapChainId = m_uiSelectedSwapChainId;

  m_sLastPixelValue.Clear();
  m_vLastPixelPosition = ezVec2I32(-1, -1);
  m_LastHistogram.Clear();
  m_Response = ezRenderGraphObserverResponse();
  m_bHasLastPixelValue = false;
  m_bHasLastHistogram = false;

  MipSpin->setMaximum(0);
  MipSpin->setEnabled(false);
  SliceSpin->setMaximum(0);
  SliceSpin->setEnabled(false);
  SampleSpin->setMaximum(-1);
  SampleSpin->setEnabled(false);
  Preview->SetTextureSize(ezVec2U32(0, 0));
  Overview->SetRequest(m_Request);
  Histogram->Clear();
  UpdateInfoWidgets();
  SendObserverRequest();
}

void ezQtRenderGraphWidget::ProcessTelemetry(void*)
{
  if (s_pWidget == nullptr)
    return;

  ezTelemetryMessage msg;
  while (ezTelemetry::RetrieveMessage(s_uiSystemId, msg) == EZ_SUCCESS)
  {
    ezUInt16 uiVersion = 0;
    switch (msg.GetMessageID())
    {
      case 'SUMM':
        msg.GetReader() >> uiVersion;
        msg.GetReader() >> s_pWidget->m_Summary;
        s_pWidget->m_bUpdateUi = true;
        break;
      case 'INFO':
      {
        ezUInt64 uiGraphIdentity = 0;
        bool bSuccess = false;
        msg.GetReader() >> uiVersion;
        msg.GetReader() >> uiGraphIdentity;
        msg.GetReader() >> bSuccess;
        if (bSuccess)
        {
          msg.GetReader() >> s_pWidget->m_Info;
          s_pWidget->m_bInfoValid = uiGraphIdentity == s_pWidget->m_uiSelectedGraphId;
        }
        else if (uiGraphIdentity == s_pWidget->m_uiSelectedGraphId)
        {
          s_pWidget->m_Info = ezRenderGraphInspectionInfo();
          s_pWidget->m_bInfoValid = false;
        }
        s_pWidget->m_bUpdateUi = true;
      }
      break;
      case 'RESP':
        msg.GetReader() >> uiVersion;
        msg.GetReader() >> s_pWidget->m_Response;
        s_pWidget->m_bUpdateUi = true;
        break;
      case ' CLR':
        s_pWidget->ResetStats();
        break;
      default:
        break;
    }
  }
}
