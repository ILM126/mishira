//*****************************************************************************
// Mishira: An audiovisual production tool for broadcasting live video
//
// Copyright (C) 2014 Lucas Murray <lucas@polyflare.com>
// All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//*****************************************************************************

#ifndef TARGETVIEW_H
#define TARGETVIEW_H

#include <QtWidgets/QMenu>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>

class Target;
class TargetPane;

//=============================================================================
class TargetView : public QScrollArea
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	QVector<TargetPane *>	m_panes;
	QWidget					m_layoutWidget;
	QVBoxLayout				m_layout;

	// Right-click context menu
	Target *		m_contextTarget;
	QMenu			m_contextMenu;
	QAction *		m_modifyTargetAction;
	QAction *		m_moveTargetUpAction;
	QAction *		m_moveTargetDownAction;

public: // Constructor/destructor ---------------------------------------------
	TargetView(QWidget *parent = NULL);
	~TargetView();

private: // Methods -----------------------------------------------------------
	void			repopulateList();

	virtual bool	eventFilter(QObject *watched, QEvent *ev);

public:
	void			showTargetContextMenu(Target *target);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			targetAdded(Target *target, int before);
	void			destroyingTarget(Target *target);
	void			targetChanged(Target *target);
	void			targetMoved(Target *target, int before);

	void			targetPaneRightClicked(Target *target);
	void			targetContextTriggered(QAction *action);

	void			enableUpdatesTimeout();
};
//=============================================================================

#endif // TARGETVIEW_H
