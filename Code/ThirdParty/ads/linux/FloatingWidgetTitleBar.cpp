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
/// \file   FloatingWidgetTitleBar.cpp
/// \author Uwe Kindler
/// \date   13.05.2019
/// \brief  Implementation of CFloatingWidgetTitleBar class
//============================================================================

//============================================================================
//                                   INCLUDES
//============================================================================
#include "FloatingWidgetTitleBar.h"

#include <iostream>

#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QPixmap>
#include <QStyle>
#include <QMouseEvent>

#include "ads_globals.h"
#include "ElidingLabel.h"
#include "FloatingDockContainer.h"

namespace ads
{

using tTabLabel = CElidingLabel;
using tCloseButton = QPushButton;

/**
 * @brief Private data class of public interface CFloatingWidgetTitleBar
 */
struct FloatingWidgetTitleBarPrivate
{
	CFloatingWidgetTitleBar *_this; ///< public interface class
	QLabel *IconLabel = nullptr;
	tTabLabel *TitleLabel;
	tCloseButton *CloseButton = nullptr;
	CFloatingDockContainer *FloatingWidget = nullptr;
	eDragState DragState = DraggingInactive;

	FloatingWidgetTitleBarPrivate(CFloatingWidgetTitleBar *_public) :
		_this(_public)
	{
	}

	/**
	 * Creates the complete layout including all controls
	 */
	void createLayout();
};

//============================================================================
void FloatingWidgetTitleBarPrivate::createLayout()
{
	TitleLabel = new tTabLabel();
	TitleLabel->setElideMode(Qt::ElideRight);
	TitleLabel->setText("DockWidget->windowTitle()");
	TitleLabel->setObjectName("floatingTitleLabel");
    TitleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	CloseButton = new tCloseButton();
	CloseButton->setObjectName("floatingTitleCloseButton");
	CloseButton->setFlat(true);

	// The standard icons do does not look good on high DPI screens
	QIcon CloseIcon;
	QPixmap normalPixmap = _this->style()->standardPixmap(
	    QStyle::SP_TitleBarCloseButton, 0, CloseButton);
	CloseIcon.addPixmap(normalPixmap, QIcon::Normal);
	CloseIcon.addPixmap(internal::createTransparentPixmap(normalPixmap, 0.25),
	    QIcon::Disabled);
	CloseButton->setIcon(
	    _this->style()->standardIcon(QStyle::SP_TitleBarCloseButton));
	CloseButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	CloseButton->setVisible(true);
	CloseButton->setFocusPolicy(Qt::NoFocus);
	_this->connect(CloseButton, SIGNAL(clicked()), SIGNAL(closeRequested()));

	QFontMetrics fm(TitleLabel->font());
	int Spacing = qRound(fm.height() / 4.0);

	// Fill the layout
	QBoxLayout *Layout = new QBoxLayout(QBoxLayout::LeftToRight);
	Layout->setContentsMargins(6, 0, 0, 0);
	Layout->setSpacing(0);
	_this->setLayout(Layout);
	Layout->addWidget(TitleLabel, 1);
	Layout->addSpacing(Spacing);
	Layout->addWidget(CloseButton);
	Layout->setAlignment(Qt::AlignCenter);

	TitleLabel->setVisible(true);
}

//============================================================================
CFloatingWidgetTitleBar::CFloatingWidgetTitleBar(CFloatingDockContainer *parent) :
	QWidget(parent),
	d(new FloatingWidgetTitleBarPrivate(this))
{
	d->FloatingWidget = parent;
	d->createLayout();
}

//============================================================================
CFloatingWidgetTitleBar::~CFloatingWidgetTitleBar()
{
	delete d;
}

//============================================================================
void CFloatingWidgetTitleBar::mousePressEvent(QMouseEvent *ev)
{
	if (ev->button() == Qt::LeftButton)
	{
		d->DragState = DraggingFloatingWidget;
		d->FloatingWidget->startDragging(ev->pos(), d->FloatingWidget->size(),
		    this);
		return;
	}
	Super::mousePressEvent(ev);
}

//============================================================================
void CFloatingWidgetTitleBar::mouseReleaseEvent(QMouseEvent *ev)
{
	d->DragState = DraggingInactive;
    if (d->FloatingWidget)
    {
        d->FloatingWidget->finishDragging();
    }
	Super::mouseReleaseEvent(ev);
}

//============================================================================
void CFloatingWidgetTitleBar::mouseMoveEvent(QMouseEvent *ev)
{
	if (!(ev->buttons() & Qt::LeftButton) || DraggingInactive == d->DragState)
	{
		d->DragState = DraggingInactive;
		Super::mouseMoveEvent(ev);
		return;
	}

	// move floating window
	if (DraggingFloatingWidget == d->DragState)
	{
		d->FloatingWidget->moveFloating();
		Super::mouseMoveEvent(ev);
		return;
	}
	Super::mouseMoveEvent(ev);
}

//============================================================================
void CFloatingWidgetTitleBar::enableCloseButton(bool Enable)
{
	d->CloseButton->setEnabled(Enable);
}

//============================================================================
void CFloatingWidgetTitleBar::setTitle(const QString &Text)
{
	d->TitleLabel->setText(Text);
}

} // namespace ads
