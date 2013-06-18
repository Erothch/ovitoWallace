///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <core/Core.h>
#include <core/gui/widgets/RolloutContainer.h>

namespace Ovito {

/******************************************************************************
* Constructs the container.
******************************************************************************/
RolloutContainer::RolloutContainer(QWidget* parent) : QScrollArea(parent)
{
	setFrameStyle(QFrame::Panel | QFrame::Sunken);
	setWidgetResizable(true);
	QWidget* widget = new QWidget();
	QVBoxLayout* layout = new QVBoxLayout(widget);
	layout->setContentsMargins(QMargins());
	layout->setSpacing(2);
	layout->addStretch(0);
	widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	setWidget(widget);
}

/******************************************************************************
* Inserts a new rollout into the container.
*		collapsed - Controls whether the new rollout should set to collapsed state.
******************************************************************************/
Rollout* RolloutContainer::addRollout(QWidget* content, const QString& title, const RolloutInsertionParameters& params)
{
	OVITO_CHECK_POINTER(content);
	Rollout* rollout = new Rollout(widget(), content, title, params);
	QBoxLayout* layout = static_cast<QBoxLayout*>(widget()->layout());
	if(params.afterThisRollout != NULL) {
		Rollout* otherRollout = qobject_cast<Rollout*>(params.afterThisRollout->parent());
		for(int i = 0; i < layout->count(); i++) {
			if(layout->itemAt(i)->widget() == otherRollout) {
				layout->insertWidget(i + 1, rollout);
			}
		}
	}
	else if(params.beforeThisRollout != NULL) {
		Rollout* otherRollout = qobject_cast<Rollout*>(params.beforeThisRollout->parent());
		for(int i = 0; i < layout->count(); i++) {
			if(layout->itemAt(i)->widget() == otherRollout) {
				layout->insertWidget(i, rollout);
			}
		}
	}
	else layout->insertWidget(layout->count() - 1, rollout);
	return rollout;
}

/******************************************************************************
* Constructs a rollout widget.
******************************************************************************/
Rollout::Rollout(QWidget* parent, QWidget* content, const QString& title, const RolloutInsertionParameters& params) :
	QWidget(parent), _content(content), _collapseAnimation(this, "visiblePercentage")
{
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	_collapseAnimation.setDuration(350);
	_collapseAnimation.setEasingCurve(QEasingCurve::InOutCubic);

	// Set initial open/collapsed state.
	if(!params.animateFirstOpening && !params.collapsed)
		_visiblePercentage = 100;
	else
		_visiblePercentage = 0;

	// Insert contents.
	_content->setParent(this);
	_content->setVisible(true);
	connect(_content, SIGNAL(destroyed(QObject*)), this, SLOT(deleteLater()));

	// Set up title button.
	_titleButton = new QPushButton(title, this);
	_titleButton->setAutoFillBackground(true);
	_titleButton->setFocusPolicy(Qt::NoFocus);
	_titleButton->setStyleSheet("QPushButton { "
							   "  color: white; "
							   "  border-style: solid; "
							   "  border-width: 1px; "
							   "  border-radius: 0px; "
							   "  border-color: black; "
							   "  background-color: grey; "
							   "  padding: 1px; "
							   "}"
							   "QPushButton:pressed { "
							   "  border-color: white; "
							   "}");
	connect(_titleButton, SIGNAL(clicked(bool)), this, SLOT(toggleCollapsed()));

	if(params.animateFirstOpening && !params.collapsed)
		setCollapsed(false);
}

/******************************************************************************
* Computes the recommended size for the widget.
******************************************************************************/
QSize Rollout::sizeHint() const
{
	QSize titleSize = _titleButton->sizeHint();
	QSize contentSize = _content->sizeHint() * _visiblePercentage / 100;
	return QSize(std::max(titleSize.width(), contentSize.width()), titleSize.height() + contentSize.height());
}

/******************************************************************************
* Handles the resize events of the rollout widget.
******************************************************************************/
void Rollout::resizeEvent(QResizeEvent* event)
{
	int titleHeight = _titleButton->sizeHint().height();
	int contentHeight = _content->sizeHint().height();
	_titleButton->setGeometry(0, 0, width(), titleHeight);
	_content->setGeometry(0, height() - contentHeight, width(), contentHeight);
}

/******************************************************************************
* Paints the border around the rollout contents.
******************************************************************************/
void Rollout::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	int y = _titleButton->height() / 2;
	if(height()-y+1 > 0)
		qDrawShadeRect(&painter, 0, y, width()+1, height()-y+1, palette(), true);
}

};