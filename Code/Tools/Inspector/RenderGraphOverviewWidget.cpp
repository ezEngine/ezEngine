#include <Inspector/InspectorPCH.h>

#include <Foundation/Math/ColorScheme.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <Inspector/RenderGraphOverviewWidget.moc.h>
#include <RendererFoundation/RendererReflection.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPolygon>

#include <algorithm>

namespace
{
  QColor GetAccessColor(ezBitflags<ezGALResourceState> access)
  {
    const bool bRead = access.IsAnySet(ezGALResourceState::AllReadStates);
    const bool bWrite = access.IsAnySet(ezGALResourceState::AllWriteStates);

    if (access.IsSet(ezGALResourceState::RenderTarget))
      return ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Orange));

    if (access.IsSet(ezGALResourceState::DepthStencilWrite))
      return ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Grape));
    if (access.IsSet(ezGALResourceState::DepthStencilRead))
      return ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Cyan));

    if (bWrite)
      return ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Red));
    if (bRead)
      return ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Lime));

    return ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Gray));
  }

  QString MakePassTooltip(ezUInt32 uiPassIndex, const ezRenderGraphInspectionInfo::PassInfo& pass)
  {
    ezStringBuilder text;
    text.SetFormat("Pass {}\n{}\nQueue: {}\n{}", uiPassIndex, pass.m_sName, ezArgEnum(pass.m_QueueType), pass.m_bAlive ? "Alive" : "Culled");
    return ezMakeQString(text);
  }

  QString MakeTextureTooltip(ezUInt32 uiTextureIndex, const ezRenderGraphInspectionInfo::TextureResourceInfo& texture)
  {
    ezStringBuilder text;
    text.SetFormat("Texture {} {}\nFormat: {}\nSize: {}x{}", uiTextureIndex,
      texture.m_bImported ? "(Imported)" : "(Transient)", ezArgEnum(texture.m_Desc.m_Format), texture.m_Desc.m_uiWidth, texture.m_Desc.m_uiHeight);
    if (texture.m_Desc.m_uiDepth > 1)
      text.AppendFormat("x{}", texture.m_Desc.m_uiDepth);
    if (texture.m_Desc.m_uiArraySize > 1)
      text.AppendFormat("\nSlices: {}", texture.m_Desc.m_uiArraySize);
    if (texture.m_Desc.m_uiMipLevelCount > 1)
      text.AppendFormat("\nMipLevels: {}", texture.m_Desc.m_uiMipLevelCount);
    if (texture.m_Desc.m_SampleCount != ezGALMSAASampleCount::None)
      text.AppendFormat("\nMSAA Samples: {}", texture.m_Desc.m_SampleCount.GetValue());

    text.AppendFormat("\nFlags: {}", ezArgEnum(texture.m_Desc.m_TextureFlags));
    text.AppendFormat("\nPasses: {} - {}\nResolved: {}", texture.m_uiFirstUsePassIndex, texture.m_uiLastUsePassIndex, texture.m_uiResolvedIndex);

    return ezMakeQString(text);
  }

  QString MakeBufferTooltip(ezUInt32 uiBufferIndex, const ezRenderGraphInspectionInfo::BufferResourceInfo& buffer)
  {
    ezStringBuilder text;
    text.SetFormat("Buffer {} {}\nSize: {} bytes\nStruct: {} bytes\nFormat: {}", uiBufferIndex,
      buffer.m_bImported ? "(Imported)" : "(Transient)", buffer.m_Desc.m_uiTotalSize, buffer.m_Desc.m_uiStructSize, ezArgEnum(buffer.m_Desc.m_Format));

    text.AppendFormat("\nFlags: {}", ezArgEnum(buffer.m_Desc.m_BufferFlags));
    text.AppendFormat("\nPasses: {} - {}\nResolved: {}", buffer.m_uiFirstUsePassIndex, buffer.m_uiLastUsePassIndex, buffer.m_uiResolvedIndex);

    return ezMakeQString(text);
  }

  void AppendAccessTooltipDetails(ezStringBuilder& ref_sText, const ezRenderGraphInspectionInfo::AccessInfo& access)
  {
    ref_sText.AppendFormat("\nAccess: {}", ezArgEnum(access.m_Access));

    if (access.m_bIsTexture)
    {
      const auto& range = access.m_TextureRange;
      ref_sText.AppendFormat("\nRange: mip {}+{}, slice {}+{}", range.m_uiBaseMipLevel, range.m_uiMipLevels,
        range.m_uiBaseArraySlice, range.m_uiArraySlices);
    }
  }
} // namespace

ezQtRenderGraphOverviewWidget::ezQtRenderGraphOverviewWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setMinimumSize(600, 300);
  setMouseTracking(true);
}

void ezQtRenderGraphOverviewWidget::SetInfo(ezUInt64 uiSelectedGraphId, const ezRenderGraphInspectionInfo& info)
{
  if (m_uiSelectedGraphId != uiSelectedGraphId)
  {
    // While a render graph is selected, we allow the size to only grow to not have the scroll bars flickers (see below).
    // This is reset here when we change to a different graph.
    m_uiSelectedGraphId = uiSelectedGraphId;
    m_MinContentSize = QSize(600, 300);
  }

  m_Info = info;

  // Build caches
  ezUInt16 uiMaxResolvedTexture = 0;
  for (const auto& texture : m_Info.m_Textures)
  {
    if (texture.m_uiResolvedIndex != 0xFFFF)
      uiMaxResolvedTexture = ezMath::Max(uiMaxResolvedTexture, texture.m_uiResolvedIndex);
  }
  m_uiTextureColumnCount = m_Info.m_Textures.IsEmpty() ? 0 : uiMaxResolvedTexture + 1;

  ezUInt16 uiMaxResolvedBuffer = 0;
  for (const auto& buffer : m_Info.m_Buffers)
  {
    if (buffer.m_uiResolvedIndex != 0xFFFF)
      uiMaxResolvedBuffer = ezMath::Max(uiMaxResolvedBuffer, buffer.m_uiResolvedIndex);
  }
  m_uiBufferColumnCount = m_Info.m_Buffers.IsEmpty() ? 0 : uiMaxResolvedBuffer + 1;
  m_uiTotalResourceColumnCount = m_uiTextureColumnCount + m_uiBufferColumnCount;

  m_TextureColumnResources.Clear();
  for (ezUInt32 i = 0; i < m_Info.m_Textures.GetCount(); ++i)
  {
    const ezUInt16 uiResolvedIndex = m_Info.m_Textures[i].m_uiResolvedIndex;
    if (uiResolvedIndex != 0xFFFF)
      m_TextureColumnResources.PushBack({uiResolvedIndex, (ezUInt16)i});
  }
  std::sort(begin(m_TextureColumnResources), end(m_TextureColumnResources), [](const TextureColumnResource& lhs, const TextureColumnResource& rhs)
    { return lhs.m_uiResolvedIndex < rhs.m_uiResolvedIndex; });

  m_BufferColumnResources.Clear();
  for (ezUInt32 i = 0; i < m_Info.m_Buffers.GetCount(); ++i)
  {
    const ezUInt16 uiResolvedIndex = m_Info.m_Buffers[i].m_uiResolvedIndex;
    if (uiResolvedIndex != 0xFFFF)
      m_BufferColumnResources.PushBack({uiResolvedIndex, (ezUInt16)i});
  }
  std::sort(begin(m_BufferColumnResources), end(m_BufferColumnResources), [](const BufferColumnResource& lhs, const BufferColumnResource& rhs)
    { return lhs.m_uiResolvedIndex < rhs.m_uiResolvedIndex; });

  // Compute size
  const QSize required(s_iPassLabelWidth + s_iContentPadding + ezMath::Max<ezInt32>(1, (ezInt32)m_uiTotalResourceColumnCount) * s_iCellSize,
    s_iHeaderHeight + s_iContentPadding + ezMath::Max<ezInt32>(1, (ezInt32)m_Info.m_Passes.GetCount()) * s_iCellSize);
  m_MinContentSize.setWidth(ezMath::Max(m_MinContentSize.width(), required.width()));
  m_MinContentSize.setHeight(ezMath::Max(m_MinContentSize.height(), required.height()));
  setMinimumSize(m_MinContentSize);
  update();
}

void ezQtRenderGraphOverviewWidget::SetRequest(const ezRenderGraphObserverRequest& request)
{
  m_sObservedPassName = request.m_sPassName;
  m_uiObservedAccessIndex = request.m_uiAccessIndex;
  update();
}

void ezQtRenderGraphOverviewWidget::Clear()
{
  m_Info = ezRenderGraphInspectionInfo();
  m_uiTextureColumnCount = 0;
  m_uiBufferColumnCount = 0;
  m_uiTotalResourceColumnCount = 0;
  m_TextureColumnResources.Clear();
  m_BufferColumnResources.Clear();
  m_sObservedPassName.Clear();
  m_uiObservedAccessIndex = 0;
  m_MinContentSize = QSize(600, 300);
  setMinimumSize(m_MinContentSize);
  update();
}

void ezQtRenderGraphOverviewWidget::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.fillRect(rect(), palette().color(QPalette::Dark));
  painter.setRenderHint(QPainter::Antialiasing, false);

  if (m_Info.m_Passes.IsEmpty() || m_uiTotalResourceColumnCount == 0)
  {
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(8, 18, "Graph has no passes or resources.");
    return;
  }

  {
    painter.setPen(palette().color(QPalette::Text));
    ezStringBuilder text;
    text.SetFormat("Passes: {}  Textures: {}  Buffers: {}  Accesses: {}", m_Info.m_Passes.GetCount(), m_Info.m_Textures.GetCount(), m_Info.m_Buffers.GetCount(),
      m_Info.m_Accesses.GetCount());
    painter.drawText(8, 18, ezMakeQString(text));
  }

  DrawResourceNames(painter);
  DrawPassNames(painter);
  DrawGrid(painter);
  DrawResourceLifetimes(painter);
  DrawAccesses(painter);
}

void ezQtRenderGraphOverviewWidget::mousePressEvent(QMouseEvent* e)
{
  const HoverInfo hover = GetHoverInfo(e->pos());
  const ezRenderGraphInspectionInfo::AccessInfo* pAccess = FindAccess(hover);
  if (pAccess != nullptr && pAccess->m_bIsTexture)
  {
    Q_EMIT AccessSelected(pAccess->m_uiPassIndex, pAccess->m_uiAccessIndex);
    return;
  }

  Q_EMIT AccessDeselected();
}

void ezQtRenderGraphOverviewWidget::mouseMoveEvent(QMouseEvent* e)
{
  setToolTip(MakeHoverTooltip(GetHoverInfo(e->pos())));
  QWidget::mouseMoveEvent(e);
}

ezQtRenderGraphOverviewWidget::HoverInfo ezQtRenderGraphOverviewWidget::GetHoverInfo(const QPoint& pos) const
{
  HoverInfo hover;
  hover.m_Position = pos;

  if (pos.x() < 0 || pos.y() < 0)
    return hover;

  if (pos.y() < s_iHeaderHeight)
  {
    if (pos.x() >= s_iPassLabelWidth)
    {
      const ezUInt32 uiResourceColumn = (ezUInt32)((pos.x() - s_iPassLabelWidth) / s_iCellSize);
      if (uiResourceColumn < m_uiTextureColumnCount)
      {
        hover.m_Area = HoverInfo::Area::TextureHeader;
        hover.m_uiResourceColumn = (ezUInt16)uiResourceColumn;
        hover.m_bIsTexture = true;
      }
      else if (uiResourceColumn < m_uiTotalResourceColumnCount)
      {
        hover.m_Area = HoverInfo::Area::BufferHeader;
        hover.m_uiResourceColumn = (ezUInt16)(uiResourceColumn - m_uiTextureColumnCount);
        hover.m_bIsTexture = false;
      }
    }

    return hover;
  }

  const ezUInt32 uiPassIndex = (ezUInt32)((pos.y() - s_iHeaderHeight) / s_iCellSize);
  if (uiPassIndex >= m_Info.m_Passes.GetCount())
    return hover;

  hover.m_uiPassIndex = (ezUInt16)uiPassIndex;

  if (pos.x() < s_iPassLabelWidth)
  {
    hover.m_Area = HoverInfo::Area::PassHeader;
    return hover;
  }

  const ezUInt32 uiResourceColumn = (ezUInt32)((pos.x() - s_iPassLabelWidth) / s_iCellSize);
  if (uiResourceColumn >= m_uiTotalResourceColumnCount)
  {
    hover.m_Area = HoverInfo::Area::None;
    hover.m_uiPassIndex = 0xFFFF;
    return hover;
  }

  hover.m_Area = HoverInfo::Area::ResourceCell;
  hover.m_uiResourceColumn = (ezUInt16)uiResourceColumn;
  hover.m_bIsTexture = uiResourceColumn < m_uiTextureColumnCount;
  return hover;
}

const ezRenderGraphInspectionInfo::AccessInfo* ezQtRenderGraphOverviewWidget::FindAccess(const HoverInfo& hover, bool bLastMatch) const
{
  if (hover.m_Area != HoverInfo::Area::ResourceCell || hover.m_uiPassIndex == 0xFFFF || hover.m_uiResourceColumn == 0xFFFF)
    return nullptr;

  auto it = std::lower_bound(begin(m_Info.m_Accesses), end(m_Info.m_Accesses), hover.m_uiPassIndex,
    [](const ezRenderGraphInspectionInfo::AccessInfo& access, ezUInt16 uiPassIndex)
    { return access.m_uiPassIndex < uiPassIndex; });

  const ezRenderGraphInspectionInfo::AccessInfo* pMatch = nullptr;
  for (; it != end(m_Info.m_Accesses) && it->m_uiPassIndex == hover.m_uiPassIndex; ++it)
  {
    if (it->m_bIsTexture != hover.m_bIsTexture)
      continue;

    if (it->m_bIsTexture)
    {
      const auto& texture = m_Info.m_Textures[it->m_uiResourceIndex];
      if (texture.m_uiResolvedIndex == hover.m_uiResourceColumn)
      {
        pMatch = &(*it);
        if (!bLastMatch)
          return pMatch;
      }
    }
    else
    {
      const auto& buffer = m_Info.m_Buffers[it->m_uiResourceIndex];
      if (m_uiTextureColumnCount + buffer.m_uiResolvedIndex == hover.m_uiResourceColumn)
      {
        pMatch = &(*it);
        if (!bLastMatch)
          return pMatch;
      }
    }
  }

  return pMatch;
}

QString ezQtRenderGraphOverviewWidget::MakeAccessesTooltip(const HoverInfo& hover) const
{
  if (hover.m_Area != HoverInfo::Area::ResourceCell || hover.m_uiPassIndex == 0xFFFF || hover.m_uiResourceColumn == 0xFFFF)
    return QString();

  auto it = std::lower_bound(begin(m_Info.m_Accesses), end(m_Info.m_Accesses), hover.m_uiPassIndex,
    [](const ezRenderGraphInspectionInfo::AccessInfo& access, ezUInt16 uiPassIndex)
    { return access.m_uiPassIndex < uiPassIndex; });

  ezStringBuilder tooltip;
  bool bHasAccess = false;
  // Iterate through all access in this pass
  for (; it != end(m_Info.m_Accesses) && it->m_uiPassIndex == hover.m_uiPassIndex; ++it)
  {
    if (it->m_bIsTexture != hover.m_bIsTexture)
      continue;

    ezUInt32 uiResourceColumn = 0;
    if (it->m_bIsTexture)
    {
      const auto& texture = m_Info.m_Textures[it->m_uiResourceIndex];
      uiResourceColumn = texture.m_uiResolvedIndex;
    }
    else
    {
      const auto& buffer = m_Info.m_Buffers[it->m_uiResourceIndex];
      uiResourceColumn = m_uiTextureColumnCount + buffer.m_uiResolvedIndex;
    }

    if (uiResourceColumn != hover.m_uiResourceColumn)
      continue;

    if (!bHasAccess)
    {
      tooltip.SetFormat("Pass: {}", m_Info.m_Passes[it->m_uiPassIndex].m_sName);
      if (it->m_bIsTexture)
        tooltip.AppendFormat("\nResource: Texture {}", it->m_uiResourceIndex);
      else
        tooltip.AppendFormat("\nResource: Buffer {}", it->m_uiResourceIndex);
      bHasAccess = true;
    }
    else
    {
      tooltip.Append("\n");
    }

    AppendAccessTooltipDetails(tooltip, *it);
  }

  return bHasAccess ? ezMakeQString(tooltip) : QString();
}

ezUInt16 ezQtRenderGraphOverviewWidget::FindTextureLifetimeResource(const HoverInfo& hover) const
{
  if (hover.m_Area != HoverInfo::Area::ResourceCell || !hover.m_bIsTexture)
    return 0xFFFF;

  auto it = std::lower_bound(begin(m_TextureColumnResources), end(m_TextureColumnResources), hover.m_uiResourceColumn,
    [](const TextureColumnResource& resource, ezUInt16 uiResolvedIndex)
    { return resource.m_uiResolvedIndex < uiResolvedIndex; });

  for (; it != end(m_TextureColumnResources) && it->m_uiResolvedIndex == hover.m_uiResourceColumn; ++it)
  {
    const ezUInt16 uiTextureIndex = it->m_uiTextureIndex;
    const auto& texture = m_Info.m_Textures[uiTextureIndex];
    if (texture.m_uiFirstUsePassIndex == 0xFFFF || texture.m_uiResolvedIndex == 0xFFFF)
      continue;

    if (GetLifetimeRect(texture.m_uiFirstUsePassIndex, texture.m_uiLastUsePassIndex, texture.m_uiResolvedIndex).contains(hover.m_Position))
      return uiTextureIndex;
  }

  return 0xFFFF;
}

ezUInt16 ezQtRenderGraphOverviewWidget::FindBufferLifetimeResource(const HoverInfo& hover) const
{
  if (hover.m_Area != HoverInfo::Area::ResourceCell || hover.m_bIsTexture)
    return 0xFFFF;

  const ezUInt32 uiBufferColumn = hover.m_uiResourceColumn - m_uiTextureColumnCount;
  auto it = std::lower_bound(begin(m_BufferColumnResources), end(m_BufferColumnResources), uiBufferColumn,
    [](const BufferColumnResource& resource, ezUInt32 uiResolvedIndex)
    { return resource.m_uiResolvedIndex < uiResolvedIndex; });

  for (; it != end(m_BufferColumnResources) && it->m_uiResolvedIndex == uiBufferColumn; ++it)
  {
    const ezUInt16 uiBufferIndex = it->m_uiBufferIndex;
    const auto& buffer = m_Info.m_Buffers[uiBufferIndex];
    if (buffer.m_uiFirstUsePassIndex == 0xFFFF || buffer.m_uiResolvedIndex == 0xFFFF)
      continue;

    if (GetLifetimeRect(buffer.m_uiFirstUsePassIndex, buffer.m_uiLastUsePassIndex, m_uiTextureColumnCount + buffer.m_uiResolvedIndex).contains(hover.m_Position))
      return uiBufferIndex;
  }

  return 0xFFFF;
}

QString ezQtRenderGraphOverviewWidget::MakeHoverTooltip(const HoverInfo& hover) const
{
  switch (hover.m_Area)
  {
    case HoverInfo::Area::PassHeader:
      return MakePassTooltip(hover.m_uiPassIndex, m_Info.m_Passes[hover.m_uiPassIndex]);

    case HoverInfo::Area::TextureHeader:
      return MakeTextureColumnTooltip(hover.m_uiResourceColumn);

    case HoverInfo::Area::BufferHeader:
      return MakeBufferColumnTooltip(hover.m_uiResourceColumn);

    case HoverInfo::Area::ResourceCell:
    {
      QString accessTooltip = MakeAccessesTooltip(hover);
      if (!accessTooltip.isEmpty())
        return accessTooltip;

      if (hover.m_bIsTexture)
      {
        const ezUInt16 uiTextureIndex = FindTextureLifetimeResource(hover);
        if (uiTextureIndex != 0xFFFF)
          return MakeTextureTooltip(uiTextureIndex, m_Info.m_Textures[uiTextureIndex]);
      }
      else
      {
        const ezUInt16 uiBufferIndex = FindBufferLifetimeResource(hover);
        if (uiBufferIndex != 0xFFFF)
          return MakeBufferTooltip(uiBufferIndex, m_Info.m_Buffers[uiBufferIndex]);
      }
    }
    break;

    case HoverInfo::Area::None:
      break;
  }

  return QString();
}

QString ezQtRenderGraphOverviewWidget::MakeTextureColumnTooltip(ezUInt32 uiResolvedIndex) const
{
  auto it = std::lower_bound(begin(m_TextureColumnResources), end(m_TextureColumnResources), uiResolvedIndex,
    [](const TextureColumnResource& resource, ezUInt32 uiResolvedIndex)
    { return resource.m_uiResolvedIndex < uiResolvedIndex; });

  QString tooltip;
  for (; it != end(m_TextureColumnResources) && it->m_uiResolvedIndex == uiResolvedIndex; ++it)
  {
    if (!tooltip.isEmpty())
      tooltip += "\n\n";
    tooltip += MakeTextureTooltip(it->m_uiTextureIndex, m_Info.m_Textures[it->m_uiTextureIndex]);
  }
  return tooltip;
}

QString ezQtRenderGraphOverviewWidget::MakeBufferColumnTooltip(ezUInt32 uiResolvedIndex) const
{
  auto it = std::lower_bound(begin(m_BufferColumnResources), end(m_BufferColumnResources), uiResolvedIndex,
    [](const BufferColumnResource& resource, ezUInt32 uiResolvedIndex)
    { return resource.m_uiResolvedIndex < uiResolvedIndex; });

  QString tooltip;
  for (; it != end(m_BufferColumnResources) && it->m_uiResolvedIndex == uiResolvedIndex; ++it)
  {
    if (!tooltip.isEmpty())
      tooltip += "\n\n";
    tooltip += MakeBufferTooltip(it->m_uiBufferIndex, m_Info.m_Buffers[it->m_uiBufferIndex]);
  }
  return tooltip;
}

QRect ezQtRenderGraphOverviewWidget::GetCellRect(ezUInt32 uiPassIndex, ezUInt32 uiResourceColumn) const
{
  return QRect(s_iPassLabelWidth + (ezInt32)uiResourceColumn * s_iCellSize, s_iHeaderHeight + (ezInt32)uiPassIndex * s_iCellSize, s_iCellSize, s_iCellSize);
}

void ezQtRenderGraphOverviewWidget::DrawResourceNames(QPainter& painter)
{
  for (ezUInt32 column = 0; column < m_uiTextureColumnCount; ++column)
  {
    const ezInt32 x = s_iPassLabelWidth + (ezInt32)column * s_iCellSize;
    painter.fillRect(QRect(x, s_iHeaderHeight - s_iCellSize, s_iCellSize, s_iCellSize), column % 2 == 0 ? palette().color(QPalette::Mid) : palette().color(QPalette::AlternateBase));
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(QRect(x, s_iHeaderHeight - s_iCellSize, s_iCellSize, s_iCellSize), Qt::AlignCenter, QString::number(column));
  }

  for (ezUInt32 column = 0; column < m_uiBufferColumnCount; ++column)
  {
    const ezUInt32 resourceColumn = m_uiTextureColumnCount + column;
    const ezInt32 x = s_iPassLabelWidth + (ezInt32)resourceColumn * s_iCellSize;
    painter.fillRect(QRect(x, s_iHeaderHeight - s_iCellSize, s_iCellSize, s_iCellSize), resourceColumn % 2 == 0 ? palette().color(QPalette::Mid) : palette().color(QPalette::AlternateBase));
    painter.setPen(ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Cyan)));
    painter.drawText(QRect(x, s_iHeaderHeight - s_iCellSize, s_iCellSize, s_iCellSize), Qt::AlignCenter, QString::number(resourceColumn));
  }
}

void ezQtRenderGraphOverviewWidget::DrawPassNames(QPainter& painter)
{
  for (ezUInt32 passIndex = 0; passIndex < m_Info.m_Passes.GetCount(); ++passIndex)
  {
    const auto& pass = m_Info.m_Passes[passIndex];
    const QRect labelRect(0, s_iHeaderHeight + (ezInt32)passIndex * s_iCellSize, s_iPassLabelWidth, s_iCellSize);
    const QColor textColor = !pass.m_bAlive ? palette().color(QPalette::Disabled, QPalette::Text) : pass.m_QueueType == ezGALQueueType::Compute  ? ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Green))
                                                                                                  : pass.m_QueueType == ezGALQueueType::Transfer ? ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Blue))
                                                                                                                                                 : palette().color(QPalette::Text);
    painter.fillRect(labelRect, passIndex % 2 == 0 ? palette().color(QPalette::Base) : palette().color(QPalette::AlternateBase));
    painter.setPen(textColor);
    painter.drawText(labelRect.adjusted(6, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft, ezMakeQString(pass.m_sName));
  }
}

void ezQtRenderGraphOverviewWidget::DrawGrid(QPainter& painter)
{
  const ezInt32 iGridWidth = (ezInt32)m_uiTotalResourceColumnCount * s_iCellSize;
  const ezInt32 iGridHeight = (ezInt32)m_Info.m_Passes.GetCount() * s_iCellSize;
  const QRect gridRect(s_iPassLabelWidth, s_iHeaderHeight, iGridWidth, iGridHeight);

  painter.fillRect(gridRect, palette().color(QPalette::Dark));
  painter.setPen(QPen(palette().color(QPalette::Midlight), 1));
  for (ezUInt32 column = 0; column <= m_uiTotalResourceColumnCount; ++column)
  {
    const ezInt32 x = s_iPassLabelWidth + (ezInt32)column * s_iCellSize;
    painter.drawLine(x, s_iHeaderHeight, x, s_iHeaderHeight + iGridHeight);
  }
  for (ezUInt32 passIndex = 0; passIndex <= m_Info.m_Passes.GetCount(); ++passIndex)
  {
    const ezInt32 y = s_iHeaderHeight + (ezInt32)passIndex * s_iCellSize;
    painter.drawLine(s_iPassLabelWidth, y, s_iPassLabelWidth + iGridWidth, y);
  }
}

void ezQtRenderGraphOverviewWidget::DrawResourceLifetimes(QPainter& painter)
{
  painter.setBrush(Qt::NoBrush);
  for (ezUInt32 i = 0; i < m_Info.m_Textures.GetCount(); ++i)
  {
    const auto& texture = m_Info.m_Textures[i];
    if (texture.m_uiFirstUsePassIndex == 0xFFFF || texture.m_uiResolvedIndex == 0xFFFF)
      continue;

    const QRect rect = GetLifetimeRect(texture.m_uiFirstUsePassIndex, texture.m_uiLastUsePassIndex, texture.m_uiResolvedIndex);
    painter.setPen(QPen(texture.m_bImported ? ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Orange)) : ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Yellow)), 1));
    painter.drawRect(rect);
  }

  for (ezUInt32 i = 0; i < m_Info.m_Buffers.GetCount(); ++i)
  {
    const auto& buffer = m_Info.m_Buffers[i];
    if (buffer.m_uiFirstUsePassIndex == 0xFFFF || buffer.m_uiResolvedIndex == 0xFFFF)
      continue;

    const QRect rect = GetLifetimeRect(buffer.m_uiFirstUsePassIndex, buffer.m_uiLastUsePassIndex, m_uiTextureColumnCount + buffer.m_uiResolvedIndex);
    painter.setPen(QPen(buffer.m_bImported ? ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Cyan)) : ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Blue)), 1));
    painter.drawRect(rect);
  }
}

QRect ezQtRenderGraphOverviewWidget::GetLifetimeRect(ezUInt32 uiFirstPass, ezUInt32 uiLastPass, ezUInt32 uiResourceColumn) const
{
  const ezInt32 x = s_iPassLabelWidth + (ezInt32)uiResourceColumn * s_iCellSize + s_iLifetimeGap;
  const ezInt32 y = s_iHeaderHeight + (ezInt32)uiFirstPass * s_iCellSize + s_iLifetimeGap;
  const ezInt32 width = s_iCellSize - 2 * s_iLifetimeGap;
  const ezInt32 height = ((ezInt32)uiLastPass - (ezInt32)uiFirstPass + 1) * s_iCellSize - 2 * s_iLifetimeGap;
  return QRect(x, y, width, height);
}

void ezQtRenderGraphOverviewWidget::DrawAccesses(QPainter& painter)
{
  ezDynamicArray<ezBitflags<ezGALResourceState>> accessMasks;
  accessMasks.SetCount(m_uiTotalResourceColumnCount);

  auto it = begin(m_Info.m_Accesses);
  for (ezUInt32 passIndex = 0; passIndex < m_Info.m_Passes.GetCount(); ++passIndex)
  {
    ezMemoryUtils::ZeroFill(accessMasks.GetData(), accessMasks.GetCount());
    ezUInt32 uiObservedResourceColumn = 0xFFFFFFFF;

    // m_Accesses is sorted by pass index first.
    while (it != end(m_Info.m_Accesses) && it->m_uiPassIndex < passIndex)
    {
      ++it;
    }

    // Accumulate all resource accesses on each resource
    for (; it != end(m_Info.m_Accesses) && it->m_uiPassIndex == passIndex; ++it)
    {
      ezUInt32 resourceColumn = 0;
      if (it->m_bIsTexture)
      {
        resourceColumn = m_Info.m_Textures[it->m_uiResourceIndex].m_uiResolvedIndex;
      }
      else
      {
        resourceColumn = m_uiTextureColumnCount + m_Info.m_Buffers[it->m_uiResourceIndex].m_uiResolvedIndex;
      }

      accessMasks[resourceColumn] |= it->m_Access;

      if (it->m_bIsTexture &&
          !m_sObservedPassName.IsEmpty() &&
          m_Info.m_Passes[it->m_uiPassIndex].m_sName == m_sObservedPassName &&
          it->m_uiAccessIndex == m_uiObservedAccessIndex)
      {
        uiObservedResourceColumn = resourceColumn;
      }
    }

    // Draw accumulated accesses
    painter.setPen(Qt::NoPen);
    for (ezUInt32 resourceColumn = 0; resourceColumn < accessMasks.GetCount(); ++resourceColumn)
    {
      const ezBitflags<ezGALResourceState> accessMask = accessMasks[resourceColumn];
      if (accessMask.IsNoFlagSet())
        continue;

      const QRect cellRect = GetCellRect(passIndex, resourceColumn);
      const QRect markerRect = cellRect.adjusted(s_iAccessPadding, s_iAccessPadding, -s_iAccessPadding, -s_iAccessPadding);
      const ezBitflags<ezGALResourceState> readMask = accessMask & ezBitflags<ezGALResourceState>(ezGALResourceState::AllReadStates);
      const ezBitflags<ezGALResourceState> writeMask = accessMask & ezBitflags<ezGALResourceState>(ezGALResourceState::AllWriteStates);

      if (!readMask.IsNoFlagSet() && !writeMask.IsNoFlagSet())
      {
        const QPoint topLeft = markerRect.topLeft();
        const QPoint topRight(markerRect.right() + 1, markerRect.top());
        const QPoint bottomLeft(markerRect.left(), markerRect.bottom() + 1);
        const QPoint bottomRight(markerRect.right() + 1, markerRect.bottom() + 1);

        QPolygon readTriangle;
        readTriangle << topLeft << topRight << bottomLeft;
        painter.setBrush(GetAccessColor(readMask));
        painter.drawPolygon(readTriangle);

        QPolygon writeTriangle;
        writeTriangle << bottomRight << topRight << bottomLeft;
        painter.setBrush(GetAccessColor(writeMask));
        painter.drawPolygon(writeTriangle);
      }
      else
      {
        painter.fillRect(markerRect, GetAccessColor(accessMask));
      }
    }

    if (uiObservedResourceColumn != 0xFFFFFFFF)
    {
      const QRect cellRect = GetCellRect(passIndex, uiObservedResourceColumn);
      painter.setPen(QPen(palette().color(QPalette::Highlight), 2));
      painter.setBrush(Qt::NoBrush);
      painter.drawRect(cellRect.adjusted(s_iLifetimeGap, s_iLifetimeGap, -s_iLifetimeGap, -s_iLifetimeGap));
    }
  }
}
