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

#ifndef MAINPAGE_H
#define MAINPAGE_H

#include "scene.h"
#include "Widgets/elidedlabel.h"
#include "Widgets/graphicswidget.h"
#include "Widgets/styledbutton.h"
#include "Widgets/styledbar.h"
#include "Widgets/zoomslider.h"
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStackedLayout>
#include <QtWidgets/QWidget>

class AudioView;
class LayerProperties;
class SceneItemView;
class SceneView;
class TargetView;
class QInputDialog;

//=============================================================================
class MainPage : public QWidget
{
	Q_OBJECT

private: // Static members ----------------------------------------------------
	static bool		s_updaterOnce; // False = Not yet checked
	static bool		s_updatesAvail;

private: // Members -----------------------------------------------------------
	QTimer			m_statusTimer;
	QProcess *		m_updaterProc;

	// Toolboxes
	QWidget			m_leftToolbox;
	QWidget			m_rightToolbox;
	QWidget			m_topToolbox;
	QWidget			m_bottomToolbox;
	QGridLayout		m_pageLayout;
	QVBoxLayout		m_leftLayout;
	QVBoxLayout		m_rightLayout;
	QStackedLayout	m_topLayout;
	QHBoxLayout		m_bottomLayout;

	// Toolbox bars
	StyledBar		m_layersBar;
	StyledBar		m_targetsBar;
	StyledBar		m_mixerBar;
	StyledBar		m_scenesBar;

	// Transition selector bar
	QWidget			m_transitionBar;
	QHBoxLayout		m_transitionLayout;

	// Transition popup menu
	uint			m_transitionMenuId;
	QMenu			m_transitionMenu;
	QAction *		m_transitionDurAction;
	QAction *		m_transitionDurChangeAction;
	QAction *		m_transitionInstantAction;
	QAction *		m_transitionFadeAction;
	QAction *		m_transitionFadeBlackAction;
	QAction *		m_transitionFadeWhiteAction;
	QInputDialog *	m_transitionDurDialog;

	// Widgets
	GraphicsWidget		m_graphicsWidget;
	StyledButton		m_broadcastButton;
	StyledButton		m_addGroupButton;
	StyledButton		m_updatesAvailableButton;
	StyledButton		m_modifyTargetsButton;
	StyledButton		m_modifyMixerButton;
	StyledButton		m_addSceneButton;
	StyledButton		m_transALeftButton;
	StyledButton		m_transARightButton;
	StyledButton		m_transBLeftButton;
	StyledButton		m_transBRightButton;
	StyledButton		m_transCLeftButton;
	StyledButton		m_transCRightButton;
	ZoomSlider			m_zoomSlider;
	QCheckBox			m_autoZoomCheckBox;
	ElidedLabel			m_statusLbl;
	SceneItemView *		m_itemView;
	SceneView *			m_sceneView;
	LayerProperties *	m_layerProperties;
	TargetView *		m_targetView;
	AudioView *			m_audioView;

public: // Constructor/destructor ---------------------------------------------
	MainPage(QWidget *parent = 0);
	~MainPage();

public: // Methods ------------------------------------------------------------
	void			showPropertiesToolbox(bool visible);
	void			setStatusLabel(const QString &msg);

private:
	void			startUpdaterProcess();
	void			updateBroadcastButton();
	void			showTransitionMenu(uint transitionId, QWidget *btn);
	void			showTransitionDurDialog(uint durationMsec);

protected: // Events ----------------------------------------------------------
	virtual void	keyPressEvent(QKeyEvent *ev);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void		updaterError(QProcess::ProcessError error);
	void		updaterResult(int exitCode, QProcess::ExitStatus exitStatus);
	void		broadcastClicked();
	void		broadcastingChanged();
	void		addGroupClicked();
	void		addSceneClicked();
	void		transALeftClicked();
	void		transARightClicked();
	void		transBLeftClicked();
	void		transBRightClicked();
	void		transCLeftClicked();
	void		transCRightClicked();
	void		updatesAvailableClicked();
	void		modifyTargetsClicked();
	void		modifyMixerClicked();
	void		zoomSliderChanged(float zoom, bool byUser);
	void		itemChanged(SceneItem *item);
	void		activeSceneChanged(Scene *scene, Scene *oldScene);
	void		transitionsChanged();
	void		transitionMenuTriggered(QAction *action);
	void		transitionDurDialogClosed(int result);
	void		statusTimeout();
};
//=============================================================================

#endif // MAINPAGE_H
