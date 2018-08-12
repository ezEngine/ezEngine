#include <PCH.h>

#include <EditorFramework/Assets/AssetImportDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <QComboBox>
#include <QFileDialog>
#include <QTableWidget>
#include <QToolButton>

enum Columns
{
  Method,
  InputFile,
  GeneratedDoc,
  Browse,
  Status,
  ENUM_COUNT
};

ezQtAssetImportDlg::ezQtAssetImportDlg(QWidget* parent, ezDynamicArray<ezAssetDocumentGenerator::ImportData>& allImports)
    : QDialog(parent)
    , m_allImports(allImports)
{
  setupUi(this);

  QStringList headers;
  headers.push_back(QString::fromUtf8("Import Method"));
  headers.push_back(QString::fromUtf8("Input File"));
  headers.push_back(QString::fromUtf8("Generated Document"));
  headers.push_back(QString());
  headers.push_back(QString("Status"));

  QTableWidget* table = AssetTable;
  table->setColumnCount(headers.size());
  table->setRowCount(m_allImports.GetCount());
  table->setHorizontalHeaderLabels(headers);

  {
    ezQtScopedBlockSignals _1(table);

    for (ezUInt32 i = 0; i < m_allImports.GetCount(); ++i)
    {
      InitRow(i);
    }
  }

  table->horizontalHeader()->setSectionResizeMode(Columns::Method, QHeaderView::ResizeMode::ResizeToContents);
  table->horizontalHeader()->setSectionResizeMode(Columns::InputFile, QHeaderView::ResizeMode::Interactive);
  table->horizontalHeader()->setSectionResizeMode(Columns::GeneratedDoc, QHeaderView::ResizeMode::Interactive);
  table->horizontalHeader()->setSectionResizeMode(Columns::Browse, QHeaderView::ResizeMode::ResizeToContents);
  table->horizontalHeader()->setSectionResizeMode(Columns::Status, QHeaderView::ResizeMode::Stretch);

  table->resizeColumnToContents(Columns::GeneratedDoc);
  table->resizeColumnToContents(Columns::InputFile);
  table->setColumnWidth(Columns::InputFile, table->columnWidth(Columns::InputFile) + 30);
  table->setColumnWidth(Columns::GeneratedDoc, table->columnWidth(Columns::GeneratedDoc) + 30);
}

ezQtAssetImportDlg::~ezQtAssetImportDlg() {}

void ezQtAssetImportDlg::InitRow(ezUInt32 uiRow)
{
  QTableWidget* table = AssetTable;
  const auto& data = m_allImports[uiRow];

  table->setItem(uiRow, Columns::InputFile, new QTableWidgetItem(data.m_sInputFileParentRelative.GetData()));
  table->setItem(uiRow, Columns::Status, new QTableWidgetItem());
  table->item(uiRow, Columns::InputFile)->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
  table->item(uiRow, Columns::Status)->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);

  QComboBox* pCombo = new QComboBox();
  table->setCellWidget(uiRow, Columns::Method, pCombo);

  pCombo->addItem(ezQtUiServices::GetSingleton()->GetCachedIconResource(":/GuiFoundation/Icons/No16.png"), "No Import");
  for (const auto& option : data.m_ImportOptions)
  {
    QIcon icon = ezQtUiServices::GetSingleton()->GetCachedIconResource(option.m_sIcon);
    pCombo->addItem(icon, ezTranslate(option.m_sName.GetData()));
  }

  pCombo->setProperty("row", uiRow);
  pCombo->setCurrentIndex(data.m_iSelectedOption + 1);
  connect(pCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(SelectedOptionChanged(int)));
  connect(table, &QTableWidget::cellChanged, this, &ezQtAssetImportDlg::TableCellChanged);

  table->setItem(uiRow, Columns::GeneratedDoc, new QTableWidgetItem(QString()));
  table->item(uiRow, Columns::GeneratedDoc)
      ->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsEditable | Qt::ItemFlag::ItemIsSelectable);

  QToolButton* pBrowse = new QToolButton();
  pBrowse->setText("Browse...");
  pBrowse->setProperty("row", uiRow);
  connect(pBrowse, &QToolButton::clicked, this, &ezQtAssetImportDlg::BrowseButtonClicked);
  table->setCellWidget(uiRow, Columns::Browse, pBrowse);

  UpdateRow(uiRow);
}

void ezQtAssetImportDlg::UpdateRow(ezUInt32 uiRow)
{
  QTableWidget* table = AssetTable;
  ezQtScopedBlockSignals _1(table);

  const auto& data = m_allImports[uiRow];

  QTableWidgetItem* pOutputItem = table->item(uiRow, Columns::GeneratedDoc);
  QTableWidgetItem* pStatusItem = table->item(uiRow, Columns::Status);

  if (data.m_iSelectedOption < 0)
    pOutputItem->setText(QString());
  else
    pOutputItem->setText(data.m_ImportOptions[data.m_iSelectedOption].m_sOutputFileParentRelative.GetData());

  QToolButton* pBrowse = qobject_cast<QToolButton*>(table->cellWidget(uiRow, Columns::Browse));
  pBrowse->setEnabled(data.m_iSelectedOption >= 0);

  pStatusItem->setTextColor(QColor::fromRgba(qRgb(200, 0, 0)));
  pStatusItem->setText(data.m_sImportMessage.GetData());

  if (data.m_bDoNotImport)
  {
    pOutputItem->setFlags(Qt::ItemFlag::ItemIsSelectable);
    pBrowse->setEnabled(false);
    QComboBox* pCombo = qobject_cast<QComboBox*>(table->cellWidget(uiRow, Columns::Method));
    pCombo->setEnabled(false);

    pBrowse->setEnabled(true);
    pBrowse->setText("Open");
    pStatusItem->setTextColor(QColor::fromRgba(qRgb(0, 200, 0)));

    if (data.m_sImportMessage.IsEmpty())
    {
      pStatusItem->setText("Asset Imported");
    }
  }
}

void ezQtAssetImportDlg::SelectedOptionChanged(int index)
{
  QComboBox* pCombo = qobject_cast<QComboBox*>(sender());
  const ezUInt32 uiRow = pCombo->property("row").toInt();

  QueryRow(uiRow);
  m_allImports[uiRow].m_iSelectedOption = index - 1;
  UpdateRow(uiRow);
}

void ezQtAssetImportDlg::QueryRow(ezUInt32 uiRow)
{
  QTableWidget* table = AssetTable;
  auto& data = m_allImports[uiRow];

  if (data.m_iSelectedOption < 0)
    return;

  auto& option = data.m_ImportOptions[data.m_iSelectedOption];

  ezStringBuilder file = table->item(uiRow, Columns::GeneratedDoc)->text().toUtf8().data();
  file.ChangeFileExtension(option.m_pGenerator->GetDocumentExtension());
  option.m_sOutputFileParentRelative = file;
}

void ezQtAssetImportDlg::UpdateAllRows()
{
  for (ezUInt32 i = 0; i < m_allImports.GetCount(); ++i)
  {
    UpdateRow(i);
  }
}

void ezQtAssetImportDlg::on_ButtonImport_clicked()
{
  ezAssetDocumentGenerator::ExecuteImport(m_allImports);

  UpdateAllRows();
}

void ezQtAssetImportDlg::TableCellChanged(int row, int column)
{
  if (column == Columns::GeneratedDoc)
  {
    QueryRow(row);
    UpdateRow(row);
  }
}

void ezQtAssetImportDlg::BrowseButtonClicked(bool)
{
  QToolButton* pButton = qobject_cast<QToolButton*>(sender());
  const ezUInt32 uiRow = pButton->property("row").toInt();

  auto& data = m_allImports[uiRow];
  if (data.m_iSelectedOption < 0)
    return;

  auto& option = data.m_ImportOptions[data.m_iSelectedOption];

  if (data.m_bDoNotImport)
  {
    // asset was already imported

    ezQtEditorApp::GetSingleton()->OpenDocument(option.m_sOutputFileAbsolute);
  }
  else
  {
    ezStringBuilder filter;
    filter.Format("{0} (*.{0})", option.m_pGenerator->GetDocumentExtension());
    QString result = QFileDialog::getSaveFileName(this, "Target Document", ezToolsProject::GetSingleton()->GetProjectDirectory().GetData(),
                                                  filter.GetData(), nullptr, QFileDialog::Option::DontResolveSymlinks);

    if (result.isEmpty())
      return;

    ezStringBuilder tmp = result.toUtf8().data();
    if (!ezQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(tmp))
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning("The selected file is not located in any of the project's data directories.");
      return;
    }

    option.m_sOutputFileAbsolute = result.toUtf8().data();
    option.m_sOutputFileParentRelative = tmp;
    UpdateRow(uiRow);
  }
}
