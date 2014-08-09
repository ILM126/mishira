//*****************************************************************************
// Mishira: An audiovisual production tool for broadcasting live video
//
// Copyright (C) 2014 Lucas Murray <lmurray@undefinedfire.com>
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

#ifndef AUDIOVIEW_H
#define AUDIOVIEW_H

#include <QtWidgets/QMenu>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>

class AudioInput;
class AudioPane;

//=============================================================================
class AudioView : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	QVector<AudioPane *>	m_panes;
	QVBoxLayout				m_layout;

	// Right-click context menu
	AudioInput *	m_contextInput;
	QMenu			m_contextMenu;
	//QAction *		m_renameSourceAction;
	QAction *		m_moveSourceUpAction;
	QAction *		m_moveSourceDownAction;

public: // Constructor/destructor ---------------------------------------------
	AudioView(QWidget *parent = NULL);
	~AudioView();

private: // Methods -----------------------------------------------------------
	void	repopulateList();

public:
	void	showInputContextMenu(AudioInput *input);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void	inputAdded(AudioInput *input, int before);
	void	destroyingInput(AudioInput *input);
	void	inputMoved(AudioInput *input, int before);

	void	audioPaneRightClicked(AudioInput *input);
	void	audioPaneContextTriggered(QAction *action);
};
//=============================================================================

#endif // AUDIOVIEW_H
