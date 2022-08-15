/*******************************************************************************
** Qt Advanced Docking System
** Copyright (C) 2017 Uwe Kindler
** 
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


//============================================================================
/// \file   DockWidgetTab.cpp
/// \author Uwe Kindler
/// \date   27.02.2017
/// \brief  Implementation of CDockWidgetTab class
//============================================================================


//============================================================================
//                                   INCLUDES
//============================================================================
#include "FloatingDragPreview.h"
#include "ElidingLabel.h"
#include "DockWidgetTab.h"

#include <QBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QStyle>
#include <QApplication>
#include <QSplitter>
#include <QDebug>
#include <QToolButton>
#include <QPushButton>
#include <QMenu>

#include "ads_globals.h"
#include "DockWidget.h"
#include "DockAreaWidget.h"
#include "FloatingDockContainer.h"
#include "DockOverlay.h"
#include "DockManager.h"
#include "IconProvider.h"
#include "DockFocusController.h"


namespace ads
{

using tTabLabel = CElidingLabel;

/**
 * Private data class of CDockWidgetTab class (pimpl)
 */
struct DockWidgetTabPrivate
{
	CDockWidgetTab* _this;
	CDockWidget* DockWidget;
	QLabel* IconLabel = nullptr;
	tTabLabel* TitleLabel;
	QPoint GlobalDragStartMousePosition;
	QPoint DragStartMousePosition;
	bool IsActiveTab = false;
	CDockAreaWidget* DockArea = nullptr;
	eDragState DragState = DraggingInactive;
	IFloatingWidget* FloatingWidget = nullptr;
	QIcon Icon;
	QAbstractButton* CloseButton = nullptr;
	QSpacerItem* IconTextSpacer;
	QPoint TabDragStartPosition;
	QSize IconSize;

	/**
	 * Private data constructor
	 */
	DockWidgetTabPrivate(CDockWidgetTab* _public);

	/**
	 * Creates the complete layout including all controls
	 */
	void createLayout();

	/**
	 * Moves the tab depending on the position in the given mouse event
	 */
	void moveTab(QMouseEvent* ev);

	/**
	 * Test function for current drag state
	 */
	bool isDraggingState(eDragState dragState) const
	{
		return this->DragState == dragState;
	}


	/**
	 * Starts floating of the dock widget that belongs to this title bar
	 * Returns true, if floating has been started and false if floating
	 * is not possible for any reason
	 */
	bool startFloating(eDragState DraggingState = DraggingFloatingWidget);

	/**
	 * Returns true if the given config flag is set
	 */
	bool testConfigFlag(CDockManager::eConfigFlag Flag) const
	{
		return CDockManager::testConfigFlag(Flag);
	}

	/**
	 * Creates the close button as QPushButton or as QToolButton
	 */
	QAbstractButton* createCloseButton() const
	{
		if (testConfigFlag(CDockManager::TabCloseButtonIsToolButton))
		{
			auto Button = new QToolButton();
			Button->setAutoRaise(true);
			return Button;
		}
		else
		{
			return new QPushButton();
		}
	}

	/**
	 * Update the close button visibility from current feature/config
	 */
	void updateCloseButtonVisibility(bool active)
	{
		bool DockWidgetClosable = DockWidget->features().testFlag(CDockWidget::DockWidgetClosable);
		bool ActiveTabHasCloseButton = testConfigFlag(CDockManager::ActiveTabHasCloseButton);
		bool AllTabsHaveCloseButton = testConfigFlag(CDockManager::AllTabsHaveCloseButton);
		bool TabHasCloseButton = (ActiveTabHasCloseButton && active) | AllTabsHaveCloseButton;
		CloseButton->setVisible(DockWidgetClosable && TabHasCloseButton);
	}

	/**
	 * Update the size policy of the close button depending on the
	 * RetainTabSizeWhenCloseButtonHidden feature
	 */
	void updateCloseButtonSizePolicy()
	{
		auto Features = DockWidget->features();
		auto SizePolicy = CloseButton->sizePolicy();
		SizePolicy.setRetainSizeWhenHidden(Features.testFlag(CDockWidget::DockWidgetClosable)
			&& testConfigFlag(CDockManager::RetainTabSizeWhenCloseButtonHidden));
		CloseButton->setSizePolicy(SizePolicy);
	}

	template <typename T>
	IFloatingWidget* createFloatingWidget(T* Widget, bool OpaqueUndocking)
	{
		if (OpaqueUndocking)
		{
			return new CFloatingDockContainer(Widget);
		}
		else
		{
			auto w = new CFloatingDragPreview(Widget);
			_this->connect(w, &CFloatingDragPreview::draggingCanceled, [=]()
			{
				DragState = DraggingInactive;
			});
			return w;
		}
	}

	/**
	 * Saves the drag start position in global and local coordinates
	 */
	void saveDragStartMousePosition(const QPoint& GlobalPos)
	{
		GlobalDragStartMousePosition = GlobalPos;
		DragStartMousePosition = _this->mapFromGlobal(GlobalPos);
	}

	/**
	 * Update the icon in case the icon size changed
	 */
	void updateIcon()
	{
		if (!IconLabel || Icon.isNull())
		{
			return;
		}

		if (IconSize.isValid())
		{
			IconLabel->setPixmap(Icon.pixmap(IconSize));
		}
		else
		{
			IconLabel->setPixmap(Icon.pixmap(_this->style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, _this)));
		}
		IconLabel->setVisible(true);
	}

	/**
	 * Convenience function for access to the dock manager dock focus controller
	 */
	CDockFocusController* focusController() const
	{
		return DockWidget->dockManager()->dockFocusController();
	}

};
// struct DockWidgetTabPrivate


//============================================================================
DockWidgetTabPrivate::DockWidgetTabPrivate(CDockWidgetTab* _public) :
	_this(_public)
{

}


//============================================================================
void DockWidgetTabPrivate::createLayout()
{
	TitleLabel = new tTabLabel();
	TitleLabel->setElideMode(Qt::ElideRight);
	TitleLabel->setText(DockWidget->windowTitle());
	TitleLabel->setObjectName("dockWidgetTabLabel");
	TitleLabel->setAlignment(Qt::AlignCenter);
	_this->connect(TitleLabel, SIGNAL(elidedChanged(bool)), SIGNAL(elidedChanged(bool)));


	CloseButton = createCloseButton();
	CloseButton->setObjectName("tabCloseButton");
	internal::setButtonIcon(CloseButton, QStyle::SP_TitleBarCloseButton, TabCloseIcon);
    CloseButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    CloseButton->setFocusPolicy(Qt::NoFocus);
    updateCloseButtonSizePolicy();
	internal::setToolTip(CloseButton, QObject::tr("Close Tab"));
	_this->connect(CloseButton, SIGNAL(clicked()), SIGNAL(closeRequested()));

	QFontMetrics fm(TitleLabel->font());
	int Spacing = qRound(fm.height() / 4.0);

	// Fill the layout
	QBoxLayout* Layout = new QBoxLayout(QBoxLayout::LeftToRight);
	Layout->setContentsMargins(2 * Spacing,0,0,0);
	Layout->setSpacing(0);
	_this->setLayout(Layout);
	Layout->addWidget(TitleLabel, 1);
	Layout->addSpacing(Spacing);
	Layout->addWidget(CloseButton);
	Layout->addSpacing(qRound(Spacing * 4.0 / 3.0));
	Layout->setAlignment(Qt::AlignCenter);

	TitleLabel->setVisible(true);
}

//============================================================================
void DockWidgetTabPrivate::moveTab(QMouseEvent* ev)
{
    ev->accept();
    QPoint Distance = internal::globalPositionOf(ev) - GlobalDragStartMousePosition;
    Distance.setY(0);
    auto TargetPos = Distance + TabDragStartPosition;
    TargetPos.rx() = qMax(TargetPos.x(), 0);
    TargetPos.rx() = qMin(_this->parentWidget()->rect().right() - _this->width() + 1, TargetPos.rx());
    _this->move(TargetPos);
    _this->raise();
}


//============================================================================
bool DockWidgetTabPrivate::startFloating(eDragState DraggingState)
{
	auto dockContainer = DockWidget->dockContainer();
    ADS_PRINT("isFloating " << dockContainer->isFloating());
    ADS_PRINT("areaCount " << dockContainer->dockAreaCount());
    ADS_PRINT("widgetCount " << DockWidget->dockAreaWidget()->dockWidgetsCount());
	// if this is the last dock widget inside of this floating widget,
	// then it does not make any sense, to make it floating because
	// it is already floating
	 if (dockContainer->isFloating()
	 && (dockContainer->visibleDockAreaCount() == 1)
	 && (DockWidget->dockAreaWidget()->dockWidgetsCount() == 1))
	{
		return false;
	}

    ADS_PRINT("startFloating");
	DragState = DraggingState;
	IFloatingWidget* FloatingWidget = nullptr;
	bool OpaqueUndocking = CDockManager::testConfigFlag(CDockManager::OpaqueUndocking) ||
		(DraggingFloatingWidget != DraggingState);

	// If section widget has multiple tabs, we take only one tab
	// If it has only one single tab, we can move the complete
	// dock area into floating widget
	QSize Size;
	if (DockArea->dockWidgetsCount() > 1)
	{
		FloatingWidget = createFloatingWidget(DockWidget, OpaqueUndocking);
		Size = DockWidget->size();
	}
	else
	{
		FloatingWidget = createFloatingWidget(DockArea, OpaqueUndocking);
		Size = DockArea->size();
	}

    if (DraggingFloatingWidget == DraggingState)
    {
        FloatingWidget->startFloating(DragStartMousePosition, Size, DraggingFloatingWidget, _this);
    	auto Overlay = DockWidget->dockManager()->containerOverlay();
    	Overlay->setAllowedAreas(OuterDockAreas);
    	this->FloatingWidget = FloatingWidget;
    }
    else
    {
     	FloatingWidget->startFloating(DragStartMousePosition, Size, DraggingInactive, nullptr);
    }

	return true;
}


//============================================================================
CDockWidgetTab::CDockWidgetTab(CDockWidget* DockWidget, QWidget *parent) :
	QFrame(parent),
	d(new DockWidgetTabPrivate(this))
{
	setAttribute(Qt::WA_NoMousePropagation, true);
	d->DockWidget = DockWidget;
	d->createLayout();
	setFocusPolicy(Qt::NoFocus);
	/*if (CDockManager::testConfigFlag(CDockManager::FocusHighlighting))
	{
		setFocusPolicy(Qt::ClickFocus);
	}*/
}

//============================================================================
CDockWidgetTab::~CDockWidgetTab()
{
    ADS_PRINT("~CDockWidgetTab()");
	delete d;
}


//============================================================================
void CDockWidgetTab::mousePressEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton)
	{
		ev->accept();
        d->saveDragStartMousePosition(internal::globalPositionOf(ev));
        d->DragState = DraggingMousePressed;
        if (CDockManager::testConfigFlag(CDockManager::FocusHighlighting))
        {
        	d->focusController()->setDockWidgetTabFocused(this);
        }
        Q_EMIT clicked();
		return;
	}
	Super::mousePressEvent(ev);
}



//============================================================================
void CDockWidgetTab::mouseReleaseEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton)
	{
		auto CurrentDragState = d->DragState;
		d->GlobalDragStartMousePosition = QPoint();
		d->DragStartMousePosition = QPoint();
		d->DragState = DraggingInactive;

		switch (CurrentDragState)
		{
		case DraggingTab:
			// End of tab moving, emit signal
			if (d->DockArea)
			{
				ev->accept();
                Q_EMIT moved(internal::globalPositionOf(ev));
			}
			break;

		case DraggingFloatingWidget:
			 ev->accept();
			 d->FloatingWidget->finishDragging();
			 break;

		default:; // do nothing
		}
	} 
	else if (ev->button() == Qt::MiddleButton)
	{
		if (CDockManager::testConfigFlag(CDockManager::MiddleMouseButtonClosesTab) && d->DockWidget->features().testFlag(CDockWidget::DockWidgetClosable))
		{
			// Only attempt to close if the mouse is still
			// on top of the widget, to allow the user to cancel.
			if (rect().contains(mapFromGlobal(QCursor::pos()))) {
				ev->accept();
				Q_EMIT closeRequested();
			}
		}
	}

	Super::mouseReleaseEvent(ev);
}


//============================================================================
void CDockWidgetTab::mouseMoveEvent(QMouseEvent* ev)
{
    if (!(ev->buttons() & Qt::LeftButton) || d->isDraggingState(DraggingInactive))
    {
    	d->DragState = DraggingInactive;
        Super::mouseMoveEvent(ev);
        return;
    }

    // move floating window
    if (d->isDraggingState(DraggingFloatingWidget))
    {
        d->FloatingWidget->moveFloating();
        Super::mouseMoveEvent(ev);
        return;
    }

    // move tab
    if (d->isDraggingState(DraggingTab))
    {
        // Moving the tab is always allowed because it does not mean moving the
    	// dock widget around
    	d->moveTab(ev);
    }

    auto MappedPos = mapToParent(ev->pos());
    bool MouseOutsideBar = (MappedPos.x() < 0) || (MappedPos.x() > parentWidget()->rect().right());
    // Maybe a fixed drag distance is better here ?
    int DragDistanceY = qAbs(d->GlobalDragStartMousePosition.y() - internal::globalPositionOf(ev).y());
    if (DragDistanceY >= CDockManager::startDragDistance() || MouseOutsideBar)
	{
		// If this is the last dock area in a dock container with only
    	// one single dock widget it does not make  sense to move it to a new
    	// floating widget and leave this one empty
		if (d->DockArea->dockContainer()->isFloating()
		 && d->DockArea->openDockWidgetsCount() == 1
		 && d->DockArea->dockContainer()->visibleDockAreaCount() == 1)
		{
			return;
		}


    	// Floating is only allowed for widgets that are floatable
		// If we do non opaque undocking, then can create the drag preview
		// if the widget is movable.
		auto Features = d->DockWidget->features();
        if (Features.testFlag(CDockWidget::DockWidgetFloatable)
        || (Features.testFlag(CDockWidget::DockWidgetMovable) && !CDockManager::testConfigFlag(CDockManager::OpaqueUndocking)))
        {
        	// If we undock, we need to restore the initial position of this
        	// tab because it looks strange if it remains on its dragged position
        	if (d->isDraggingState(DraggingTab) && !CDockManager::testConfigFlag(CDockManager::OpaqueUndocking))
			{
        		parentWidget()->layout()->update();
			}
            d->startFloating();
        }
    	return;
	}
    else if (d->DockArea->openDockWidgetsCount() > 1
     && (internal::globalPositionOf(ev) - d->GlobalDragStartMousePosition).manhattanLength() >= QApplication::startDragDistance()) // Wait a few pixels before start moving
	{
    	// If we start dragging the tab, we save its inital position to
    	// restore it later
    	if (DraggingTab != d->DragState)
    	{
    		d->TabDragStartPosition = this->pos();
    	}
        d->DragState = DraggingTab;
		return;
	}

   Super::mouseMoveEvent(ev);
}


//============================================================================
void CDockWidgetTab::contextMenuEvent(QContextMenuEvent* ev)
{
	ev->accept();
	if (d->isDraggingState(DraggingFloatingWidget))
	{
		return;
	}

	d->saveDragStartMousePosition(ev->globalPos());
	QMenu Menu(this);

    const bool isFloatable = d->DockWidget->features().testFlag(CDockWidget::DockWidgetFloatable);
    const bool isNotOnlyTabInContainer =  !d->DockArea->dockContainer()->hasTopLevelDockWidget();

    const bool isDetachable = isFloatable && isNotOnlyTabInContainer;

	auto Action = Menu.addAction(tr("Detach"), this, SLOT(detachDockWidget()));
    Action->setEnabled(isDetachable);
	Menu.addSeparator();
	Action = Menu.addAction(tr("Close"), this, SIGNAL(closeRequested()));
	Action->setEnabled(isClosable());
	Menu.addAction(tr("Close Others"), this, SIGNAL(closeOtherTabsRequested()));
	Menu.exec(ev->globalPos());
}


//============================================================================
bool CDockWidgetTab::isActiveTab() const
{
	return d->IsActiveTab;
}


//============================================================================
void CDockWidgetTab::setActiveTab(bool active)
{
    d->updateCloseButtonVisibility(active);

	// Focus related stuff
	if (CDockManager::testConfigFlag(CDockManager::FocusHighlighting) && !d->DockWidget->dockManager()->isRestoringState())
	{
		bool UpdateFocusStyle = false;
		if (active && !hasFocus())
		{
			//setFocus(Qt::OtherFocusReason);
			d->focusController()->setDockWidgetTabFocused(this);
			UpdateFocusStyle = true;
		}

		if (d->IsActiveTab == active)
		{
			if (UpdateFocusStyle)
			{
				updateStyle();
			}
			return;
		}
	}
	else if (d->IsActiveTab == active)
	{
		return;
	}

	d->IsActiveTab = active;
	updateStyle();
	update();
	updateGeometry();

	Q_EMIT activeTabChanged();
}


//============================================================================
CDockWidget* CDockWidgetTab::dockWidget() const
{
	return d->DockWidget;
}


//============================================================================
void CDockWidgetTab::setDockAreaWidget(CDockAreaWidget* DockArea)
{
	d->DockArea = DockArea;
}


//============================================================================
CDockAreaWidget* CDockWidgetTab::dockAreaWidget() const
{
	return d->DockArea;
}


//============================================================================
void CDockWidgetTab::setIcon(const QIcon& Icon)
{
	QBoxLayout* Layout = qobject_cast<QBoxLayout*>(layout());
	if (!d->IconLabel && Icon.isNull())
	{
		return;
	}

	if (!d->IconLabel)
	{
		d->IconLabel = new QLabel();
		d->IconLabel->setAlignment(Qt::AlignVCenter);
		d->IconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
		internal::setToolTip(d->IconLabel, d->TitleLabel->toolTip());
		Layout->insertWidget(0, d->IconLabel, Qt::AlignVCenter);
		Layout->insertSpacing(1, qRound(1.5 * Layout->contentsMargins().left() / 2.0));
	}
	else if (Icon.isNull())
	{
		// Remove icon label and spacer item
		Layout->removeWidget(d->IconLabel);
		Layout->removeItem(Layout->itemAt(0));
		delete d->IconLabel;
		d->IconLabel = nullptr;
	}

	d->Icon = Icon;
	d->updateIcon();
}


//============================================================================
const QIcon& CDockWidgetTab::icon() const
{
	return d->Icon;
}


//============================================================================
QString CDockWidgetTab::text() const
{
	return d->TitleLabel->text();
}


//============================================================================
void CDockWidgetTab::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) 
	{
		// If this is the last dock area in a dock container it does not make
		// sense to move it to a new floating widget and leave this one
		// empty
		if ((!d->DockArea->dockContainer()->isFloating() || d->DockArea->dockWidgetsCount() > 1)
			&& d->DockWidget->features().testFlag(CDockWidget::DockWidgetFloatable))
		{
			event->accept();
			d->saveDragStartMousePosition(internal::globalPositionOf(event));
			d->startFloating(DraggingInactive);
		}
	}

	Super::mouseDoubleClickEvent(event);
}


//============================================================================
void CDockWidgetTab::setVisible(bool visible)
{
	visible &= !d->DockWidget->features().testFlag(CDockWidget::NoTab);
    Super::setVisible(visible);
}


//============================================================================
void CDockWidgetTab::setText(const QString& title)
{
	d->TitleLabel->setText(title);
}


//============================================================================
bool CDockWidgetTab::isTitleElided() const
{
	return d->TitleLabel->isElided();
}



//============================================================================
bool CDockWidgetTab::isClosable() const
{
	return d->DockWidget && d->DockWidget->features().testFlag(CDockWidget::DockWidgetClosable);
}


//===========================================================================
void CDockWidgetTab::detachDockWidget()
{
	if (!d->DockWidget->features().testFlag(CDockWidget::DockWidgetFloatable))
	{
		return;
	}

	d->saveDragStartMousePosition(QCursor::pos());
	d->startFloating(DraggingInactive);
}


//============================================================================
bool CDockWidgetTab::event(QEvent *e)
{
#ifndef QT_NO_TOOLTIP
	if (e->type() == QEvent::ToolTipChange)
	{
		const auto text = toolTip();
		d->TitleLabel->setToolTip(text);
		if (d->IconLabel) {
			d->IconLabel->setToolTip(text);
		}
	}
#endif
	if (e->type() == QEvent::StyleChange)
	{
		d->updateIcon();
	}
	return Super::event(e);
}


//============================================================================
void CDockWidgetTab::onDockWidgetFeaturesChanged()
{
    d->updateCloseButtonSizePolicy();
    d->updateCloseButtonVisibility(isActiveTab());
}


//============================================================================
void CDockWidgetTab::setElideMode(Qt::TextElideMode mode)
{
	d->TitleLabel->setElideMode(mode);
}


//============================================================================
void CDockWidgetTab::updateStyle()
{
	internal::repolishStyle(this, internal::RepolishDirectChildren);
}


//============================================================================
QSize CDockWidgetTab::iconSize() const
{
	return d->IconSize;
}


//============================================================================
void CDockWidgetTab::setIconSize(const QSize& Size)
{
	d->IconSize = Size;
	d->updateIcon();
}




} // namespace ads
//---------------------------------------------------------------------------
// EOF DockWidgetTab.cpp
