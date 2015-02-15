#include <PCH.h>
#include <Inspector/ResourceWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <MainWindow.moc.h>
#include <qgraphicsitem.h>
#include <QComboBox>

/// \todo Refcount ? (Max?)
/// \todo Select Resource -> send to App for preview

void FormatSize(ezStringBuilder& s, const char* szPrefix, ezUInt64 uiSize);

ezResourceWidget* ezResourceWidget::s_pWidget = nullptr;

ezResourceWidget::ezResourceWidget(QWidget* parent) : QDockWidget(parent)
{
  s_pWidget = this;

  setupUi(this);

  m_bShowDeleted = true;

  ResetStats();
}

void ezResourceWidget::ResetStats()
{
  m_Resources.Clear();

  m_bUpdateTable = true;
  m_bUpdateTypeBox = true;
  m_LastTableUpdate = ezTime::Seconds(0);

  Table->clear();
  Table->setRowCount(0);

  {
    QStringList Headers;
    Headers.append(" Resource Type ");
    Headers.append(" Priority ");
    Headers.append(" State ");
    Headers.append(" QL Disc. ");
    Headers.append(" QL Load. ");
    Headers.append(" CPU Mem. ");
    Headers.append(" GPU Mem. ");
    Headers.append(" Resource ID ");

    Table->setColumnCount(Headers.size());
    Table->setHorizontalHeaderLabels(Headers);
    Table->horizontalHeader()->show();
  }

  Table->resizeColumnsToContents();
  Table->sortByColumn(0, Qt::DescendingOrder);
  CheckShowDeleted->setChecked(m_bShowDeleted);
}


void ezResourceWidget::UpdateStats()
{
  if (!m_bUpdateTable)
    return;

  UpdateTable();
}

class ByteSizeItem : public QTableWidgetItem
{
public:
  ByteSizeItem(ezUInt32 uiBytes, const char* szString) : QTableWidgetItem(szString)
  {
    m_uiBytes = uiBytes;
  }

  bool operator< (const QTableWidgetItem& other) const
  {
    return m_uiBytes < ((ByteSizeItem&) other).m_uiBytes;
  }

  ezUInt32 m_uiBytes;
};

void ezResourceWidget::UpdateTable()
{
  if (!m_bUpdateTable)
    return;

  if (ezTime::Now() - m_LastTableUpdate < ezTime::Seconds(0.25))
    return;

  bool bResizeFirstColumn = false;

  if (m_bUpdateTypeBox)
  {
    ComboResourceTypes->blockSignals(true);

    m_bUpdateTypeBox = false;

    if (ComboResourceTypes->currentIndex() == 0)
    {
      m_sTypeFilter.Clear();
    }
    else
    {
      m_sTypeFilter = ComboResourceTypes->currentText().toUtf8().data();
    }

    ComboResourceTypes->clear();
    ComboResourceTypes->addItem("All Resource Types");

    ezUInt32 uiSelected = 0;
    for (auto it = m_ResourceTypes.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Key() == m_sTypeFilter)
        uiSelected = ComboResourceTypes->count();

      ComboResourceTypes->addItem(QLatin1String(it.Key().GetData()));
    }

    ComboResourceTypes->setCurrentIndex(uiSelected);

    ComboResourceTypes->blockSignals(false);

    bResizeFirstColumn = true;
  }

  m_LastTableUpdate = ezTime::Now();
  m_bUpdateTable = false;

  Table->blockSignals(true);
  Table->setSortingEnabled(false);

  ezStringBuilder sTemp;

  for (auto it = m_Resources.GetIterator(); it.IsValid(); ++it)
  {
    auto& res = it.Value();

    if (res.m_bUpdate)
    {
      res.m_bUpdate = false;

      bool bShowItem = true;

      if (!m_bShowDeleted && res.m_LoadingState.m_State == ezResourceState::Invalid)
      {
        bShowItem = false;
      }
      else if (!m_sTypeFilter.IsEmpty() && res.m_sResourceType != m_sTypeFilter)
      {
        bShowItem = false;
      }
      else if (!m_sNameFilter.IsEmpty() && res.m_sResourceID.FindSubString_NoCase(m_sNameFilter) == nullptr)
      {
        bShowItem = false;
      }

      if (!bShowItem)
      {
        if (res.m_pMainItem != nullptr)
        {
          Table->removeRow(Table->row(res.m_pMainItem));
          res.m_pMainItem = nullptr;
        }

        continue;
      }


      QTableWidgetItem* pItem;

      ezInt32 iTableRow = -1;

      if (res.m_pMainItem == nullptr)
      {
        iTableRow = Table->rowCount();
        Table->insertRow(iTableRow);

        res.m_pMainItem = new QTableWidgetItem();
        Table->setItem(iTableRow, 0, res.m_pMainItem);
        Table->setItem(iTableRow, 1, new QTableWidgetItem());
        Table->setItem(iTableRow, 2, new QTableWidgetItem());
        Table->setItem(iTableRow, 3, new QTableWidgetItem());
        Table->setItem(iTableRow, 4, new QTableWidgetItem());
        Table->setItem(iTableRow, 5, new ByteSizeItem(0, ""));
        Table->setItem(iTableRow, 6, new ByteSizeItem(0, ""));
        Table->setItem(iTableRow, 7, new QTableWidgetItem());
      }
      else
      {
        iTableRow = Table->row(res.m_pMainItem);
      }

      pItem = Table->item(iTableRow, 7);
      pItem->setText(res.m_sResourceID.GetData());

      if (res.m_LoadingState.m_State == ezResourceState::LoadedResourceMissing)
      {
        pItem->setIcon(QIcon(":/Icons/Icons/ResourceMissing.png"));
        pItem->setToolTip("The resource could not be loaded.");
      }
      else if (!res.m_Flags.IsAnySet(ezResourceFlags::IsReloadable))
      {
        pItem->setIcon(QIcon(":/Icons/Icons/ResourceCreated.png"));
        pItem->setToolTip("Resource is not reloadable.");
      }
      else if (res.m_Flags.IsAnySet(ezResourceFlags::ResourceHasFallback))
      {
        pItem->setIcon(QIcon(":/Icons/Icons/ResourceFallback.png"));
        pItem->setToolTip("A fallback resource is specified.");
      }
      else
      {
        pItem->setIcon(QIcon(":/Icons/Icons/Resource.png"));
        pItem->setToolTip("Resource is reloadable but no fallback is available.");
      }

      pItem = Table->item(iTableRow, 0);
      pItem->setText(res.m_sResourceType.GetData());

      pItem = Table->item(iTableRow, 1);
      pItem->setTextAlignment(Qt::AlignHCenter);

      switch (res.m_Priority)
      {
      case ezResourcePriority::Highest:
        pItem->setText("Highest");
        pItem->setTextColor(QColor::fromRgb(255, 106, 0));
        break;
      case ezResourcePriority::High:
        pItem->setText("High");
        pItem->setTextColor(QColor::fromRgb(255, 216, 0));
        break;
      case ezResourcePriority::Normal:
        pItem->setText("Normal");
        pItem->setTextColor(QColor::fromRgb(0, 148, 255));
        break;
      case ezResourcePriority::Low:
        pItem->setText("Low");
        pItem->setTextColor(QColor::fromRgb(127, 146, 255));
        break;
      case ezResourcePriority::Lowest:
        pItem->setText("Lowest");
        pItem->setTextColor(QColor::fromRgb(127, 201, 255));
        break;
      }

      if (res.m_Flags.IsAnySet(ezResourceFlags::IsPreloading))
      {
        pItem->setText("Preloading");
        pItem->setTextColor(QColor::fromRgb(86, 255, 25));
      }

      pItem = Table->item(iTableRow, 2);
      pItem->setTextAlignment(Qt::AlignHCenter);
      switch (res.m_LoadingState.m_State)
      {
      case ezResourceState::Invalid:
        pItem->setText("Deleted");
        pItem->setTextColor(QColor::fromRgb(128, 128, 128));
        break;
      case ezResourceState::Unloaded:
        pItem->setText("Unloaded");
        pItem->setTextColor(QColor::fromRgb(255, 216, 0));
        break;
      case ezResourceState::UnloadedMetaInfoAvailable:
        pItem->setText("Meta Data");
        pItem->setTextColor(QColor::fromRgb(255, 127, 237));
        break;
      case ezResourceState::Loaded:
        pItem->setText("Loaded");
        pItem->setTextColor(QColor::fromRgb(182, 255, 0));
        break;
      case ezResourceState::LoadedResourceMissing:
        pItem->setText("Missing");
        pItem->setTextColor(QColor::fromRgb(255, 0, 0));
        break;
      }

      pItem = Table->item(iTableRow, 3);
      sTemp.Format("%u", res.m_LoadingState.m_uiQualityLevelsDiscardable);
      pItem->setText(sTemp.GetData());
      pItem->setToolTip("The number of quality levels that could be discarded to free up memory.");

      pItem = Table->item(iTableRow, 4);
      sTemp.Format("%u", res.m_LoadingState.m_uiQualityLevelsLoadable);
      pItem->setText(sTemp.GetData());
      pItem->setToolTip("The number of quality levels that could be additionally loaded for higher quality.");

      ByteSizeItem* pByteItem;

      pByteItem = (ByteSizeItem*) Table->item(iTableRow, 5);
      sTemp.Format("%u Bytes", res.m_Memory.m_uiMemoryCPU);
      pByteItem->setToolTip(sTemp.GetData());
      FormatSize(sTemp, "", res.m_Memory.m_uiMemoryCPU);
      pByteItem->setText(sTemp.GetData());
      pByteItem->m_uiBytes = res.m_Memory.m_uiMemoryCPU;

      pByteItem = (ByteSizeItem*) Table->item(iTableRow, 6);
      sTemp.Format("%u Bytes", res.m_Memory.m_uiMemoryGPU);
      pByteItem->setToolTip(sTemp.GetData());
      FormatSize(sTemp, "", res.m_Memory.m_uiMemoryGPU);
      pByteItem->setText(sTemp.GetData());
      pByteItem->m_uiBytes = res.m_Memory.m_uiMemoryGPU;

      if (res.m_LoadingState.m_State == ezResourceState::Invalid)
      {
        Table->item(iTableRow, 7)->setIcon(QIcon(":/Icons/Icons/ResourceDeleted.png"));
        //Table->item(iTableRow, 1)->setText(""); // Priority
        //Table->item(iTableRow, 3)->setText(""); // QL D
        //Table->item(iTableRow, 4)->setText(""); // QL L

        for (int i = 0; i < 8; ++i)
          Table->item(iTableRow, i)->setTextColor(QColor::fromRgb(128, 128, 128));
      }
    }
  }

  if (bResizeFirstColumn)
  {
    Table->resizeColumnToContents(0);
  }

  Table->setSortingEnabled(true);
  Table->blockSignals(false);
}

void ezResourceWidget::UpdateAll()
{
  m_bUpdateTable = true;

  for (auto it = m_Resources.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_bUpdate = true;
  }
}

void ezResourceWidget::on_LineFilterByName_textChanged()
{
  m_sNameFilter = LineFilterByName->text().toUtf8().data();

  UpdateAll();
}

void ezResourceWidget::on_ComboResourceTypes_currentIndexChanged(int state)
{
  if (state == 0)
    m_sTypeFilter.Clear();
  else
    m_sTypeFilter = ComboResourceTypes->currentText().toUtf8().data();

  UpdateAll();
}

void ezResourceWidget::on_CheckShowDeleted_toggled(bool checked)
{
  m_bShowDeleted = checked;
  UpdateAll();
}

void ezResourceWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage('RESM', Msg) == EZ_SUCCESS)
  {
    s_pWidget->m_bUpdateTable = true;

    ezUInt32 uiResourceNameHash = 0;
    Msg.GetReader() >> uiResourceNameHash;

    ResourceData& rd = s_pWidget->m_Resources[uiResourceNameHash];
    rd.m_bUpdate = true;

    if (Msg.GetMessageID() == 'SET')
    {
      Msg.GetReader() >> rd.m_sResourceID;

      Msg.GetReader() >> rd.m_sResourceType;

      if (!s_pWidget->m_ResourceTypes.Contains(rd.m_sResourceType))
      {
        s_pWidget->m_bUpdateTypeBox = true;
        s_pWidget->m_ResourceTypes.Insert(rd.m_sResourceType);
      }

      ezUInt8 uiPriority = 0;
      Msg.GetReader() >> uiPriority;
      rd.m_Priority = (ezResourcePriority) uiPriority;

      ezUInt8 uiFlags = 0;
      Msg.GetReader() >> uiFlags;
      rd.m_Flags.Clear();
      rd.m_Flags.Add((ezResourceFlags::Enum) uiFlags);

      ezUInt8 uiLoadingState = 0;
      Msg.GetReader() >> uiLoadingState;

      rd.m_LoadingState.m_State = (ezResourceState) uiLoadingState;
      Msg.GetReader() >> rd.m_LoadingState.m_uiQualityLevelsDiscardable;
      Msg.GetReader() >> rd.m_LoadingState.m_uiQualityLevelsLoadable;

      Msg.GetReader() >> rd.m_Memory.m_uiMemoryCPU;
      Msg.GetReader() >> rd.m_Memory.m_uiMemoryGPU;
    }

    if (Msg.GetMessageID() == 'UPDT')
    {
      ezUInt8 uiPriority = 0;
      Msg.GetReader() >> uiPriority;
      rd.m_Priority = (ezResourcePriority) uiPriority;

      ezUInt8 uiFlags = 0;
      Msg.GetReader() >> uiFlags;
      rd.m_Flags.Clear();
      rd.m_Flags.Add((ezResourceFlags::Enum) uiFlags);

      ezUInt8 uiLoadingState = 0;
      Msg.GetReader() >> uiLoadingState;

      rd.m_LoadingState.m_State = (ezResourceState) uiLoadingState;
      Msg.GetReader() >> rd.m_LoadingState.m_uiQualityLevelsDiscardable;
      Msg.GetReader() >> rd.m_LoadingState.m_uiQualityLevelsLoadable;

      Msg.GetReader() >> rd.m_Memory.m_uiMemoryCPU;
      Msg.GetReader() >> rd.m_Memory.m_uiMemoryGPU;
    }

    if (Msg.GetMessageID() == 'DEL')
    {
      rd.m_Flags.Remove(ezResourceFlags::IsPreloading);
      rd.m_LoadingState.m_State = ezResourceState::Invalid;
    }
  }
}
