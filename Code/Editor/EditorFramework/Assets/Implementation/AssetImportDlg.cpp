#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetImportDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

enum Columns
{
  Method,
  InputFile,
  ENUM_COUNT
};

ezQtAssetImportDlg::ezQtAssetImportDlg(QWidget* pParent, ezDynamicArray<ezAssetDocumentGenerator::ImportGroupOptions>& ref_allImports)
  : QDialog(pParent)
  , m_AllImports(ref_allImports)
{
  setupUi(this);

  QStringList headers;
  headers.push_back(QString::fromUtf8("Import Method"));
  headers.push_back(QString::fromUtf8("Input File"));

  QTableWidget* table = AssetTable;
  table->setColumnCount(headers.size());
  table->setRowCount(m_AllImports.GetCount());
  table->setHorizontalHeaderLabels(headers);

  {
    ezQtScopedBlockSignals _1(table);

    for (ezUInt32 i = 0; i < m_AllImports.GetCount(); ++i)
    {
      InitRow(i);
    }
  }

  table->horizontalHeader()->setSectionResizeMode(Columns::Method, QHeaderView::ResizeMode::ResizeToContents);
  table->horizontalHeader()->setSectionResizeMode(Columns::InputFile, QHeaderView::ResizeMode::Stretch);
}

ezQtAssetImportDlg::~ezQtAssetImportDlg() = default;

void ezQtAssetImportDlg::InitRow(ezUInt32 uiRow)
{
  QTableWidget* table = AssetTable;
  const auto& data2 = m_AllImports[uiRow];

  table->setItem(uiRow, Columns::InputFile, new QTableWidgetItem(data2.m_sInputFileRelative.GetData()));
  table->item(uiRow, Columns::InputFile)->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);

  QComboBox* pCombo = new QComboBox();
  table->setCellWidget(uiRow, Columns::Method, pCombo);

  pCombo->addItem(ezQtUiServices::GetSingleton()->GetCachedIconResource(":/GuiFoundation/Icons/NoEntry.svg"), "No Import");
  for (const auto& option : data2.m_ImportOptions)
  {
    QIcon icon = ezQtUiServices::GetSingleton()->GetCachedIconResource(option.m_sIcon);
    pCombo->addItem(icon, ezMakeQString(ezTranslate(option.m_sName)));
  }

  pCombo->setProperty("row", uiRow);
  pCombo->setCurrentIndex(data2.m_iSelectedOption + 1);
  connect(pCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(SelectedOptionChanged(int)));
}

void ezQtAssetImportDlg::SelectedOptionChanged(int index)
{
  QComboBox* pCombo = qobject_cast<QComboBox*>(sender());
  const ezUInt32 uiRow = pCombo->property("row").toInt();

  m_AllImports[uiRow].m_iSelectedOption = index - 1;
}

void ezQtAssetImportDlg::on_ButtonImport_clicked()
{
  for (auto& data : m_AllImports)
  {
    if (data.m_iSelectedOption < 0)
      continue;

    EZ_LOG_BLOCK("Asset Import", data.m_sInputFileRelative);

    const auto& option = data.m_ImportOptions[data.m_iSelectedOption];

    option.m_pGenerator->Import(data.m_sInputFileAbsolute, option.m_sName, false).LogFailure();
  }

  accept();
}
