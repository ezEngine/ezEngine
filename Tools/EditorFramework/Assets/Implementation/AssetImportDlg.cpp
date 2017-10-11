#include <PCH.h>
#include <EditorFramework/Assets/AssetImportDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <QComboBox>
#include <QTableWidget>
#include <Foundation/Strings/TranslationLookup.h>

enum Columns
{
  Method,
  InputFile,
  GeneratedDoc,
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

  QTableWidget* table = AssetTable;
  table->setColumnCount(headers.size());
  table->setRowCount(m_allImports.GetCount());
  table->setHorizontalHeaderLabels(headers);

  for (ezUInt32 i = 0; i < m_allImports.GetCount(); ++i)
  {
    InitRow(i);
  }

  table->horizontalHeader()->setSectionResizeMode(Columns::Method, QHeaderView::ResizeMode::ResizeToContents);
  table->horizontalHeader()->setSectionResizeMode(Columns::InputFile, QHeaderView::ResizeMode::Interactive);
  table->horizontalHeader()->setSectionResizeMode(Columns::GeneratedDoc, QHeaderView::ResizeMode::Stretch);

  table->resizeColumnToContents(Columns::InputFile);
  table->setColumnWidth(Columns::InputFile, table->columnWidth(Columns::InputFile) + 30);
}

ezQtAssetImportDlg::~ezQtAssetImportDlg()
{
}

void ezQtAssetImportDlg::InitRow(ezUInt32 uiRow)
{
  QTableWidget* table = AssetTable;
  const auto& data = m_allImports[uiRow];

  table->setItem(uiRow, Columns::InputFile, new QTableWidgetItem(data.m_sInputFile.GetData()));
  table->item(uiRow, Columns::InputFile)->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);

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

  table->setItem(uiRow, Columns::GeneratedDoc, new QTableWidgetItem(QString()));
  table->item(uiRow, Columns::GeneratedDoc)->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsEditable | Qt::ItemFlag::ItemIsSelectable);

  UpdateRow(uiRow);
}

void ezQtAssetImportDlg::UpdateRow(ezUInt32 uiRow)
{
  QTableWidget* table = AssetTable;
  const auto& data = m_allImports[uiRow];

  QTableWidgetItem* pOutputItem = table->item(uiRow, Columns::GeneratedDoc);

  if (data.m_iSelectedOption < 0)
    pOutputItem->setText(QString());
  else
    pOutputItem->setText(data.m_ImportOptions[data.m_iSelectedOption].m_sOutputFile.GetData());
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

  data.m_ImportOptions[data.m_iSelectedOption].m_sOutputFile = table->item(uiRow, Columns::GeneratedDoc)->text().toUtf8().data();
}

void ezQtAssetImportDlg::on_ButtonImport_clicked()
{
  accept();
}
