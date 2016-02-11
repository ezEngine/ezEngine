#include <PCH.h>
#include <EditorFramework/Dialogs/InputConfigDlg.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <QTreeWidget>
#include <QCheckBox>
#include <QMessageBox>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QSpinBox>
#include <QComboBox>

InputConfigDlg::InputConfigDlg(QWidget* parent) : QDialog(parent)
{
  setupUi(this);

  LoadActions();

  // dummy
  // todo: use array, sorted as they are given to the input manager
  {
	  m_AllInputSlots.Insert( "Key_A" );
	  m_AllInputSlots.Insert( "Key_B" );
	  m_AllInputSlots.Insert( "Key_C" );
	  for ( const auto& action : m_Actions )
	  {
		  m_AllInputSlots.Insert( action.m_sInputSlotTrigger[0] );
		  m_AllInputSlots.Insert( action.m_sInputSlotTrigger[1] );
		  m_AllInputSlots.Insert( action.m_sInputSlotTrigger[2] );
	  }
  }

  FillList();

  on_TreeActions_itemSelectionChanged();
}

void InputConfigDlg::on_ButtonNewInputSet_clicked()
{

}

void InputConfigDlg::on_ButtonNewAction_clicked()
{
	auto pItem = TreeActions->currentItem();

	if ( !pItem )
		return;

	if ( TreeActions->indexOfTopLevelItem( pItem ) < 0 )
		pItem = pItem->parent();

	ezGameAppInputConfig action;
	auto pNewItem = CreateActionItem( pItem, action );

	TreeActions->clearSelection();
	pNewItem->setSelected( true );
	TreeActions->editItem( pNewItem );
}

void InputConfigDlg::on_ButtonRemove_clicked()
{
	auto pItem = TreeActions->currentItem();

	if ( !pItem )
		return;

	if ( TreeActions->indexOfTopLevelItem( pItem ) >= 0 )
	{
		if ( ezUIServices::GetInstance()->MessageBoxQuestion( "Do you really want to remove the entire Input Set?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) == QMessageBox::No )
			return;
	}
	else
	{
		if ( ezUIServices::GetInstance()->MessageBoxQuestion( "Do you really want to remove this action?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) == QMessageBox::No )
			return;
	}

	delete pItem;
}

void InputConfigDlg::on_ButtonBox_clicked( QAbstractButton* pButton )
{
	if ( pButton == ButtonBox->button( QDialogButtonBox::Ok ) )
	{
		accept();
		return;
	}

	if ( pButton == ButtonBox->button( QDialogButtonBox::Cancel ) )
	{
		reject();
		return;
	}
}

void InputConfigDlg::on_TreeActions_itemSelectionChanged()
{
	const bool hasSelection = TreeActions->currentItem() != nullptr;

	ButtonRemove->setEnabled( hasSelection );
	ButtonNewAction->setEnabled( hasSelection );
}

void InputConfigDlg::LoadActions()
{
  m_Actions.Clear();

  ezStringBuilder sPath = ezToolsProject::GetInstance()->GetProjectPath();
  sPath.PathParentDirectory();
  sPath.AppendPath("InputConfig.json");

  ezFileReader file;
  if (file.Open(sPath).Failed())
    return;
  
  ezGameAppInputConfig::ReadFromJson(file, m_Actions);
}

void InputConfigDlg::FillList()
{
  QtScopedBlockSignals bs(TreeActions);
  QtScopedUpdatesDisabled bu(TreeActions);

  TreeActions->clear();

  ezSet<ezString> InputSets;

  for (const auto& action : m_Actions)
  {
    InputSets.Insert(action.m_sInputSet);
  }

  for (auto it = InputSets.GetIterator(); it.IsValid(); ++it)
  {
    auto* pItem = new QTreeWidgetItem(TreeActions);
    pItem->setText(0, it.Key().GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);

    m_InputSetToItem[it.Key()] = pItem;
  }

  for (const auto& action : m_Actions)
  {
    QTreeWidgetItem* pParentItem = m_InputSetToItem[action.m_sInputSet];

	CreateActionItem( pParentItem, action );


	pParentItem->setExpanded( true );
  }

  TreeActions->resizeColumnToContents( 0 );
  TreeActions->resizeColumnToContents( 1 );
  TreeActions->resizeColumnToContents( 2 );
  TreeActions->resizeColumnToContents( 3 );
  TreeActions->resizeColumnToContents( 4 );
  TreeActions->resizeColumnToContents( 5 );
  TreeActions->resizeColumnToContents( 6 );
  TreeActions->resizeColumnToContents( 7 );
}

QTreeWidgetItem* InputConfigDlg::CreateActionItem( QTreeWidgetItem* pParentItem, const ezGameAppInputConfig& action )
{
	auto* pItem = new QTreeWidgetItem( pParentItem );
	pItem->setText( 0, action.m_sInputAction.GetData() );
	pItem->setFlags( Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsEditable );

	QCheckBox* pTimeScale = new QCheckBox( TreeActions );
	pTimeScale->setChecked( action.m_bApplyTimeScaling );
	TreeActions->setItemWidget( pItem, 1, pTimeScale );

	for ( int i = 0; i < 3; ++i )
	{
		QDoubleSpinBox* spin = new QDoubleSpinBox( TreeActions );
		spin->setDecimals( 3 );
		spin->setMinimum( 0.0 );
		spin->setMaximum( 100.0 );
		spin->setSingleStep( 0.01 );
		spin->setValue( action.m_fInputSlotScale[i] );

		TreeActions->setItemWidget( pItem, 3 + 2 * i, spin );

		QComboBox* combo = new QComboBox( TreeActions );
		combo->setAutoCompletion( true );
		combo->setAutoCompletionCaseSensitivity( Qt::CaseInsensitive );
		combo->setEditable( false );
		combo->setInsertPolicy( QComboBox::InsertAtBottom );
		combo->setMaxVisibleItems( 15 );

		for ( auto it = m_AllInputSlots.GetIterator(); it.IsValid(); ++it )
		{
			combo->addItem( it.Key().GetData() );
		}

		int index = combo->findText( action.m_sInputSlotTrigger[i].GetData() );

		if ( index > 0 )
			combo->setCurrentIndex( index );
		else
			combo->setCurrentIndex( 0 );

		TreeActions->setItemWidget( pItem, 2 + 2 * i, combo );
	}

	return pItem;
}




