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

#include "mainpage.h"
#include "application.h"
#include "constants.h"
#include "layer.h"
#include "layergroup.h"
#include "layerproperties.h"
#include "profile.h"
#include "sceneitem.h"
#include "stylehelper.h"
#include "target.h"
#include "Widgets/audioview.h"
#include "Widgets/borderspacer.h"
#include "Widgets/embosswidget.h"
#include "Widgets/patternwidgets.h"
#include "Widgets/sceneitemview.h"
#include "Widgets/sceneview.h"
#include "Widgets/sortableview.h"
#include "Widgets/targetpane.h"
#include "Widgets/targetview.h"
#include <QtCore/QTime>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenuBar>

//=============================================================================
// MainPage class

// Prevent checking for updates multiple times per execution by only checking
// a maximum of one times.
bool MainPage::s_updaterOnce = false;
bool MainPage::s_updatesAvail = false;

MainPage::MainPage(QWidget *parent)
	: QWidget(parent)
	, m_statusTimer(this)
	, m_updaterProc(NULL)

	// Toolboxes
	, m_leftToolbox(this)
	, m_rightToolbox(this)
	, m_topToolbox(this)
	, m_bottomToolbox(this)
	, m_pageLayout(this)
	, m_leftLayout(&m_leftToolbox)
	, m_rightLayout(&m_rightToolbox)
	, m_topLayout(&m_topToolbox)
	, m_bottomLayout(&m_bottomToolbox)

	// Toolbox bars
	, m_layersBar(
	StyleHelper::DarkFg2Color, StyleHelper::DarkBg3Color,
	StyleHelper::TextShadowColor, tr("Scene layers"), &m_leftToolbox)
	, m_targetsBar(
	StyleHelper::DarkFg2Color, StyleHelper::DarkBg3Color,
	StyleHelper::TextShadowColor, tr("Targets"), &m_rightToolbox)
	, m_mixerBar(
	StyleHelper::DarkFg2Color, StyleHelper::DarkBg3Color,
	StyleHelper::TextShadowColor, tr("Audio mixer"), &m_rightToolbox)
	, m_scenesBar(
	StyleHelper::DarkFg2Color, StyleHelper::DarkBg3Color,
	StyleHelper::TextShadowColor, tr("Scenes"), &m_rightToolbox)

	// Transition selector bar
	, m_transitionBar(this)
	, m_transitionLayout(&m_transitionBar)

	// Transition popup menu
	, m_transitionMenuId(0)
	, m_transitionMenu(this)
	, m_transitionDurAction(NULL)
	, m_transitionDurChangeAction(NULL)
	, m_transitionInstantAction(NULL)
	, m_transitionFadeAction(NULL)
	, m_transitionFadeBlackAction(NULL)
	, m_transitionFadeWhiteAction(NULL)
	, m_transitionDurDialog(NULL)

	// Widgets
	, m_graphicsWidget(this)
	, m_broadcastButton(StyleHelper::GreenSet, true, this)
	, m_addGroupButton(StyleHelper::BlueSet, true, this)
	, m_updatesAvailableButton(StyleHelper::YellowSet, false, this)
	, m_modifyTargetsButton(StyleHelper::BlueSet, true, this)
	, m_modifyMixerButton(StyleHelper::BlueSet, true, this)
	, m_addSceneButton(StyleHelper::BlueSet, true, this)
	, m_transALeftButton(StyleHelper::ButtonSet, true, this)
	, m_transARightButton(StyleHelper::ButtonSet, true, this)
	, m_transBLeftButton(StyleHelper::ButtonSet, true, this)
	, m_transBRightButton(StyleHelper::ButtonSet, true, this)
	, m_transCLeftButton(StyleHelper::ButtonSet, true, this)
	, m_transCRightButton(StyleHelper::ButtonSet, true, this)
	, m_zoomSlider(&m_bottomToolbox)
	, m_autoZoomCheckBox(&m_bottomToolbox)
	, m_statusLbl(&m_bottomToolbox)
	, m_itemView(NULL)
	, m_sceneView(NULL)
	, m_layerProperties(NULL)
	, m_targetView(NULL)
	, m_audioView(NULL)
{
#ifdef Q_OS_WIN
	// Add the global menu bar if we're on windows. This reparents the menu bar
	// so we have to unset the parent during our destructor so that it doesn't
	// get destroyed as well.
	QMenuBar *menuBar = App->getMenuBar();
	m_pageLayout.setMenuBar(menuBar);
#endif

	// We want a different font for the main window
	QFont font;
#ifdef Q_OS_WIN
	font.setFamily(QStringLiteral("Segoe UI"));
	setFont(font);
#endif

	// Set the base window colour
	QPalette pal = palette();
	pal.setColor(backgroundRole(), StyleHelper::DarkBg5Color);
	setPalette(pal);
	setAutoFillBackground(true);

	// Remove layout margins so that the child widgets fill the entire space
	m_pageLayout.setMargin(0);
	m_pageLayout.setSpacing(0);
	m_leftLayout.setMargin(0);
	m_leftLayout.setSpacing(0);
	m_rightLayout.setMargin(0);
	m_rightLayout.setSpacing(0);
	m_topLayout.setMargin(0);
	m_topLayout.setSpacing(0);
	m_bottomLayout.setMargin(0);
	m_bottomLayout.setSpacing(0);
	m_transitionLayout.setMargin(0);
	m_transitionLayout.setSpacing(0);

	//-------------------------------------------------------------------------
	// Initialize widgets

	// Main graphics widget
	m_graphicsWidget.setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Expanding);

	// Begin/stop broadcasting button
	m_broadcastButton.setSizePolicy(
		QSizePolicy::Minimum, QSizePolicy::Fixed);
	m_broadcastButton.setFixedHeight(40);
	font = m_broadcastButton.font();
	//font.setBold(true);
	font.setPixelSize(15);
#ifdef Q_OS_WIN
	font.setFamily(QStringLiteral("Calibri"));
#endif
	m_broadcastButton.setFont(font);
	updateBroadcastButton();
	connect(&m_broadcastButton, &StyledButton::clicked,
		this, &MainPage::broadcastClicked);
	connect(App, &Application::broadcastingChanged,
		this, &MainPage::broadcastingChanged);

	// Add scene layer group button
	m_addGroupButton.setSizePolicy(
		QSizePolicy::Minimum, QSizePolicy::Fixed);
	m_addGroupButton.setFixedHeight(STYLED_BAR_HEIGHT);
	m_addGroupButton.setIcon(QIcon(":/Resources/white-plus.png"));
	m_addGroupButton.setText(tr("Group"));
	font = m_addGroupButton.font();
	font.setBold(true);
	m_addGroupButton.setFont(font);
	m_addGroupButton.setToolTip(tr("Add a new scene layer group"));
	QMargins margins = m_addGroupButton.contentsMargins();
	margins.setLeft(4);
	margins.setRight(5);
	m_addGroupButton.setContentsMargins(margins);
	m_layersBar.addRightWidget(&m_addGroupButton);
	connect(&m_addGroupButton, &StyledButton::clicked,
		this, &MainPage::addGroupClicked);

	// Updates available button
	m_updatesAvailableButton.setSizePolicy(
		QSizePolicy::Maximum, QSizePolicy::Fixed);
	m_updatesAvailableButton.setFixedHeight(STYLED_BAR_HEIGHT);
	m_updatesAvailableButton.setText(tr("Updates available"));
	m_updatesAvailableButton.setFont(font); // Reuse font from above
	m_updatesAvailableButton.hide();
	connect(&m_updatesAvailableButton, &StyledButton::clicked,
		this, &MainPage::updatesAvailableClicked);

	// Modify targets button
	m_modifyTargetsButton.setSizePolicy(
		QSizePolicy::Maximum, QSizePolicy::Fixed);
	m_modifyTargetsButton.setFixedHeight(STYLED_BAR_HEIGHT);
	m_modifyTargetsButton.setText(tr("Modify"));
	m_modifyTargetsButton.setFont(font); // Reuse font from above
	m_targetsBar.addRightWidget(&m_modifyTargetsButton);
	connect(&m_modifyTargetsButton, &StyledButton::clicked,
		this, &MainPage::modifyTargetsClicked);

	// Modify audio mixer button
	m_modifyMixerButton.setSizePolicy(
		QSizePolicy::Maximum, QSizePolicy::Fixed);
	m_modifyMixerButton.setFixedHeight(STYLED_BAR_HEIGHT);
	m_modifyMixerButton.setText(tr("Modify"));
	m_modifyMixerButton.setFont(font); // Reuse font from above
	m_mixerBar.addRightWidget(&m_modifyMixerButton);
	connect(&m_modifyMixerButton, &StyledButton::clicked,
		this, &MainPage::modifyMixerClicked);

	// Add scene button
	m_addSceneButton.setSizePolicy(
		QSizePolicy::Minimum, QSizePolicy::Fixed);
	m_addSceneButton.setFixedHeight(STYLED_BAR_HEIGHT);
	m_addSceneButton.setIcon(QIcon(":/Resources/white-plus.png"));
	m_addSceneButton.setText(tr("Scene"));
	m_addSceneButton.setFont(font); // Reuse font from above
	m_addSceneButton.setToolTip(tr("Add a new scene"));
	m_addSceneButton.setContentsMargins(margins); // Reuse margins from above
	m_scenesBar.addRightWidget(&m_addSceneButton);
	connect(&m_addSceneButton, &StyledButton::clicked,
		this, &MainPage::addSceneClicked);

	// Zoom slider
	m_zoomSlider.setSizePolicy(
		QSizePolicy::Maximum, QSizePolicy::Fixed);
	m_zoomSlider.setFixedHeight(STYLED_BAR_HEIGHT);
	m_zoomSlider.setToolTip(tr("Preview area zoom")); // TODO: Display amount
	void (ZoomSlider:: *fpfZS)(float) = &ZoomSlider::setZoom;
	connect(&m_graphicsWidget, &GraphicsWidget::zoomChanged,
		&m_zoomSlider, fpfZS);
	connect(&m_zoomSlider, &ZoomSlider::zoomChanged,
		this, &MainPage::zoomSliderChanged);

	// Automatic zoom check box
	App->applyDarkStyle(&m_autoZoomCheckBox);
	m_autoZoomCheckBox.setFixedHeight(21); // HACK to move the box down 1px
	m_autoZoomCheckBox.setIcon(QIcon(":/Resources/zoom-fill-screen.png"));
	m_autoZoomCheckBox.setToolTip(tr("Automatically zoom to fill window"));
	m_autoZoomCheckBox.setChecked(m_graphicsWidget.isAutoZoomEnabled());
	connect(&m_graphicsWidget, &GraphicsWidget::autoZoomEnabledChanged,
		&m_autoZoomCheckBox, &QCheckBox::setChecked);
	connect(&m_autoZoomCheckBox, &QCheckBox::clicked,
		&m_graphicsWidget, &GraphicsWidget::setAutoZoomEnabled);

	// Status bar message
	App->applyDarkStyle(&m_statusLbl);
	m_statusLbl.setFixedHeight(21); // HACK to move the box down 1px
	m_statusLbl.setAlignment(Qt::AlignCenter);

	// Scene item list
	Profile *profile = App->getProfile();
	Scene *scene = App->getActiveScene();
	m_itemView = new SceneItemView(scene, this);
	m_itemView->setFrameStyle(QFrame::NoFrame);
	m_itemView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	m_itemView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	connect(profile, &Profile::activeSceneChanged,
		this, &MainPage::activeSceneChanged);

	// Scene list
	m_sceneView = new SceneView(profile, this);
	m_sceneView->setFrameStyle(QFrame::NoFrame);
	m_sceneView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	m_sceneView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

	// Scene layer properties
	m_layerProperties = new LayerProperties(profile, this);

	// Target list
	m_targetView = new TargetView(this);
	m_targetView->setFrameStyle(QFrame::NoFrame);
	m_targetView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_targetView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_targetView->setSizePolicy(
		QSizePolicy::Preferred, QSizePolicy::Maximum);

	// Audio list
	m_audioView = new AudioView(this);
	m_audioView->setSizePolicy(
		QSizePolicy::Preferred, QSizePolicy::Fixed);

	//-------------------------------------------------------------------------
	// Transition selector bar

	m_transALeftButton.setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_transALeftButton.setFixedHeight(STYLED_BAR_HEIGHT);
	m_transALeftButton.setJoin(false, true);
	margins = m_transALeftButton.contentsMargins();
	margins.setLeft(2);
	margins.setRight(2);
	m_transALeftButton.setContentsMargins(margins);
	m_transitionLayout.addWidget(&m_transALeftButton);
	connect(&m_transALeftButton, &StyledButton::clicked,
		this, &MainPage::transALeftClicked);

	m_transARightButton.setSizePolicy(
		QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_transARightButton.setFixedSize(15, STYLED_BAR_HEIGHT);
	m_transARightButton.setToolTip(tr("Change transition"));
	m_transARightButton.setJoin(true, false);
	margins = m_transARightButton.contentsMargins();
	margins.setLeft(0);
	margins.setRight(0);
	m_transARightButton.setContentsMargins(margins);
	m_transitionLayout.addWidget(&m_transARightButton);
	connect(&m_transARightButton, &StyledButton::clicked,
		this, &MainPage::transARightClicked);

	m_transBLeftButton.setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_transBLeftButton.setFixedHeight(STYLED_BAR_HEIGHT);
	m_transBLeftButton.setJoin(false, true);
	margins = m_transBLeftButton.contentsMargins();
	margins.setLeft(2);
	margins.setRight(2);
	m_transBLeftButton.setContentsMargins(margins);
	m_transitionLayout.addWidget(&m_transBLeftButton);
	connect(&m_transBLeftButton, &StyledButton::clicked,
		this, &MainPage::transBLeftClicked);

	m_transBRightButton.setSizePolicy(
		QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_transBRightButton.setFixedSize(15, STYLED_BAR_HEIGHT);
	m_transBRightButton.setToolTip(tr("Change transition"));
	m_transBRightButton.setJoin(true, false);
	margins = m_transBRightButton.contentsMargins();
	margins.setLeft(0);
	margins.setRight(0);
	m_transBRightButton.setContentsMargins(margins);
	m_transitionLayout.addWidget(&m_transBRightButton);
	connect(&m_transBRightButton, &StyledButton::clicked,
		this, &MainPage::transBRightClicked);

	m_transCLeftButton.setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_transCLeftButton.setFixedHeight(STYLED_BAR_HEIGHT);
	m_transCLeftButton.setJoin(false, true);
	margins = m_transCLeftButton.contentsMargins();
	margins.setLeft(2);
	margins.setRight(2);
	m_transCLeftButton.setContentsMargins(margins);
	m_transitionLayout.addWidget(&m_transCLeftButton);
	connect(&m_transCLeftButton, &StyledButton::clicked,
		this, &MainPage::transCLeftClicked);

	m_transCRightButton.setSizePolicy(
		QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_transCRightButton.setFixedSize(15, STYLED_BAR_HEIGHT);
	m_transCRightButton.setToolTip(tr("Change transition"));
	m_transCRightButton.setJoin(true, false);
	margins = m_transCRightButton.contentsMargins();
	margins.setLeft(0);
	margins.setRight(0);
	m_transCRightButton.setContentsMargins(margins);
	m_transitionLayout.addWidget(&m_transCRightButton);
	connect(&m_transCRightButton, &StyledButton::clicked,
		this, &MainPage::transCRightClicked);

	// Evenly space buttons
	m_transitionLayout.setStretch(0, 1);
	m_transitionLayout.setStretch(2, 1);
	m_transitionLayout.setStretch(4, 1);

	// Watch profile for changes and update immediately
	connect(profile, &Profile::transitionsChanged,
		this, &MainPage::transitionsChanged);
	transitionsChanged();

	// Populate transition popup menu
	m_transitionDurAction = m_transitionMenu.addAction(tr(
		"Current duration: 250 msec"));
	m_transitionDurAction->setEnabled(false);
	m_transitionDurChangeAction = m_transitionMenu.addAction(tr(
		"Change duration..."));
	m_transitionMenu.addSeparator();
	m_transitionInstantAction = m_transitionMenu.addAction(tr(
		PrflTransitionStrings[PrflInstantTransition]));
	m_transitionInstantAction->setCheckable(true);
	m_transitionFadeAction = m_transitionMenu.addAction(tr(
		PrflTransitionStrings[PrflFadeTransition]));
	m_transitionFadeAction->setCheckable(true);
	m_transitionFadeBlackAction = m_transitionMenu.addAction(tr(
		PrflTransitionStrings[PrflFadeBlackTransition]));
	m_transitionFadeBlackAction->setCheckable(true);
	m_transitionFadeWhiteAction = m_transitionMenu.addAction(tr(
		PrflTransitionStrings[PrflFadeWhiteTransition]));
	m_transitionFadeWhiteAction->setCheckable(true);
	connect(&m_transitionMenu, &QMenu::triggered,
		this, &MainPage::transitionMenuTriggered);

	//-------------------------------------------------------------------------
	// Populate toolboxes

	m_leftLayout.addWidget(&m_layersBar);
	m_leftLayout.addWidget(new VBorderSpacer(this));
	m_leftLayout.addWidget(m_itemView);
	m_leftLayout.addWidget(new VBorderSpacer(this));
	m_leftLayout.addWidget(&m_scenesBar);
	m_leftLayout.addWidget(new VBorderSpacer(this));
	m_leftLayout.addWidget(m_sceneView);
	m_leftLayout.addWidget(new VBorderSpacer(this));
	m_leftLayout.addWidget(&m_transitionBar);
	m_leftLayout.setStretch(2, 2);
	m_leftLayout.setStretch(6, 1);

	m_rightLayout.addWidget(&m_broadcastButton);
	m_rightLayout.addWidget(new VBorderSpacer(this));
	m_rightLayout.addWidget(&m_targetsBar);
	m_rightLayout.addWidget(new VBorderSpacer(this));
	m_rightLayout.addWidget(m_targetView);
	//m_rightLayout.addWidget(new VBorderSpacer(this));
	m_rightLayout.addWidget(&m_mixerBar);
	m_rightLayout.addWidget(new VBorderSpacer(this));
	m_rightLayout.addWidget(m_audioView);
	m_rightLayout.addStretch(0);
	m_rightLayout.setStretch(4, 1);

	m_topLayout.addWidget(new EmbossWidgetSmallDark(&m_topToolbox));
	m_topLayout.addWidget(m_layerProperties);

	m_bottomLayout.addWidget(&m_zoomSlider);
	HeaderPattern *padding = new HeaderPattern(&m_bottomToolbox);
	padding->setFixedSize(15, 12);
	m_bottomLayout.addWidget(padding);
	m_bottomLayout.addSpacing(6);
	m_bottomLayout.addWidget(&m_autoZoomCheckBox, 0, Qt::AlignBottom);
	m_bottomLayout.addWidget(&m_statusLbl, 1); // Stretches
	m_bottomLayout.addSpacing(6);
	m_bottomLayout.addWidget(&m_updatesAvailableButton);

	//-------------------------------------------------------------------------

	// Set title bar style
	m_layersBar.setLabelBold(true);
	m_targetsBar.setLabelBold(true);
	m_mixerBar.setLabelBold(true);
	m_scenesBar.setLabelBold(true);

	// Add toolboxes and the graphics widget to the layout
	m_pageLayout.addWidget(&m_leftToolbox, 0, 0, 5, 1);
	m_pageLayout.addWidget(new HBorderSpacer(this), 0, 1, 5, 1);
	m_pageLayout.addWidget(&m_topToolbox, 0, 2, 1, 1);
	m_pageLayout.addWidget(new VBorderSpacer(this), 1, 2, 1, 1);
	m_pageLayout.addWidget(&m_graphicsWidget, 2, 2, 1, 1);
	m_pageLayout.addWidget(new VBorderSpacer(this), 3, 2, 1, 1);
	m_pageLayout.addWidget(&m_bottomToolbox, 4, 2, 1, 1);
	m_pageLayout.addWidget(new HBorderSpacer(this), 0, 3, 5, 1);
	m_pageLayout.addWidget(&m_rightToolbox, 0, 4, 5, 1);

	// Make the graphics widget use as much space as possible
	m_pageLayout.setRowStretch(2, 1);

	// Setup toolbox sizes
	m_leftToolbox.setFixedWidth(240);
	m_rightToolbox.setFixedWidth(200);
	m_bottomToolbox.setSizePolicy(
		QSizePolicy::Ignored, QSizePolicy::MinimumExpanding);

	// Watch scene for changes
	activeSceneChanged(App->getActiveScene(), NULL);

	// Connect timer signals
	connect(&m_statusTimer, &QTimer::timeout,
		this, &MainPage::statusTimeout);

	// Check if there's a new update available
	startUpdaterProcess();
}

MainPage::~MainPage()
{
	// Exit updater process if it's still running
	if(m_updaterProc != NULL) {
		if(m_updaterProc->state() != QProcess::NotRunning)
			m_updaterProc->kill();
		delete m_updaterProc;
		m_updaterProc = NULL;
	}

#ifdef Q_OS_WIN
	// Unset the menu bar parent so that it doesn't get destroyed
	m_pageLayout.menuBar()->setParent(NULL);
#endif

	delete m_itemView;
	delete m_sceneView;
	delete m_layerProperties;
}

void MainPage::showPropertiesToolbox(bool visible)
{
	m_topLayout.setCurrentIndex(visible ? 1 : 0);
}

void MainPage::setStatusLabel(const QString &msg)
{
	QString curTime = QTime::currentTime().toString(QStringLiteral("h:mma"));
	m_statusLbl.setText(QStringLiteral("[ %1 ] %2").arg(curTime).arg(msg));

	// Clear status message after X seconds
	m_statusTimer.setSingleShot(true);
	m_statusTimer.start(2 * 60 * 1000); // 2 minutes
}

void MainPage::statusTimeout()
{
	m_statusLbl.setText(QString());
}

void MainPage::updateBroadcastButton()
{
	if(App->isBroadcasting()) {
		m_broadcastButton.setText(tr("Stop broadcasting"));
		m_broadcastButton.setColorSet(StyleHelper::RedSet);
	} else {
		m_broadcastButton.setText(tr("Begin broadcasting"));
		m_broadcastButton.setColorSet(StyleHelper::GreenSet);
	}
}

void MainPage::keyPressEvent(QKeyEvent *ev)
{
	switch(ev->key()) {
	default:
		break;
	case Qt::Key_Escape: {
		Scene *scene = App->getActiveScene();
		if(scene != NULL)
			scene->setActiveItem(NULL);
		ev->accept();
		break; }
	case Qt::Key_Delete: {
		SceneItem *item = App->getActiveItem();
		if(item != NULL)
			App->showDeleteLayerDialog(item);
		ev->accept();
		break; }
	case Qt::Key_F2: {
		SceneItem *item = App->getActiveItem();
		if(item != NULL)
			m_itemView->editLayerName(item);
		ev->accept();
		break; }
	}
}

void MainPage::startUpdaterProcess()
{
#if !IS_UPDATER_ENABLED
	// If the updater is disabled then return immediately
	return;
#endif // IS_UPDATER_ENABLED

	// Only check for updates once per execution. If we detected that there is
	// a new version available then keep displaying the "updates available"
	// button in the status bar
	if(s_updaterOnce) {
		if(s_updatesAvail)
			m_updatesAvailableButton.show();
		return;
	}
	s_updaterOnce = true;

	// Configure program executable and arguments
#ifdef Q_OS_WIN
	QString prog =
		QStringLiteral("%1/wyUpdate.exe").arg(App->applicationDirPath());
	QStringList args;
	args << "/quickcheck" << "/justcheck" << "/noerr";
#else
#error Unsupported platform
#endif // Q_OS_WIN

	// Create and start the process
	m_updaterProc = new QProcess(this);
	void (QProcess:: *fpisP)(int, QProcess::ExitStatus) = &QProcess::finished;
	void (QProcess:: *fpeP)(QProcess::ProcessError) = &QProcess::error;
	connect(m_updaterProc, fpisP,
		this, &MainPage::updaterResult);
	connect(m_updaterProc, fpeP,
		this, &MainPage::updaterError);
	m_updaterProc->start(prog, args);
}

void MainPage::updaterError(QProcess::ProcessError error)
{
	switch(error) {
	case QProcess::FailedToStart:
		appLog(Log::Warning) << QStringLiteral(
			"Updater process failed to start");
		setStatusLabel(tr(
			"Error checking for updates - Updater failed to start"));
		break;
	case QProcess::Crashed:
		appLog(Log::Warning) << QStringLiteral(
			"Updater process crashed");
		setStatusLabel(tr(
			"Error checking for updates - Updater crashed"));
		break;
	case QProcess::Timedout:
		appLog(Log::Warning) << QStringLiteral(
			"Updater process timed out");
		setStatusLabel(tr(
			"Error checking for updates - Updater timed out"));
		break;
	case QProcess::ReadError:
		appLog(Log::Warning) << QStringLiteral(
			"Updater process had a read error");
		setStatusLabel(tr(
			"Error checking for updates - Updater read error"));
		break;
	case QProcess::WriteError:
		appLog(Log::Warning) << QStringLiteral(
			"Updater process had a write error");
		setStatusLabel(tr(
			"Error checking for updates - Updater write error"));
		break;
	default:
	case QProcess::UnknownError:
		appLog(Log::Warning) << QStringLiteral(
			"Updater process had an unknown error");
		setStatusLabel(tr(
			"Error checking for updates - Updater had an unknown error"));
		break;
	}
}

void MainPage::updaterResult(int exitCode, QProcess::ExitStatus exitStatus)
{
	// If it wasn't a clean exit then the exit code is invalid
	if(exitStatus != QProcess::NormalExit) {
		// Already logged reason
		return;
	}

	// Check the return code
#ifdef Q_OS_WIN
	switch(exitCode) {
	case 0: // Up-to-date
		appLog() << QStringLiteral("%1 is up-to-date")
			.arg(APP_NAME);
		break;
	default:
	case 1: // Updater general error
	case 3: // User cancelled update
	case 4: // Updater already running (Focuses the existing window)
		appLog(Log::Warning) << QStringLiteral(
			"Internal updater error (Return code = %1)")
			.arg(exitCode);
		setStatusLabel(tr(
			"Error checking for updates - Internal updater error (%1)")
			.arg(exitCode));
		break;
	case 2: // Updates available
		appLog(Log::Warning) << QStringLiteral(
			"A new version of %1 is available")
			.arg(APP_NAME);
		m_updatesAvailableButton.show();
		s_updatesAvail = true;
		break;
	}
#else
#error Unsupported platform
#endif // Q_OS_WIN
}

void MainPage::broadcastClicked()
{
	App->showStartStopBroadcastDialog(!App->isBroadcasting());
}

void MainPage::broadcastingChanged()
{
	updateBroadcastButton();
}

void MainPage::addGroupClicked()
{
	// Determine position to insert group at
	int before;
	SceneItem *item = App->getActiveItem();
	if(item != NULL) {
		LayerGroup *group;
		if(item->isGroup()) {
			group = item->getGroup();
		} else {
			Layer *layer = item->getLayer();
			group = layer->getParent();
		}
		before = item->getScene()->indexOfGroup(group);
	} else
		before = 0; // Insert at top of list

	// Actually add the group
	Profile *profile = App->getProfile();
	Scene *scene = App->getActiveScene();
	LayerGroup *group = profile->createLayerGroup(QString());
	scene->addGroup(group, true, before); // Visible by default
}

void MainPage::addSceneClicked()
{
	// Determine position to insert scene at
	int before;
	Profile *profile = App->getProfile();
	Scene *scene = App->getActiveScene();
	if(scene != NULL)
		before = profile->indexOfScene(scene);
	else
		before = 0; // Insert at top of list

	// Actually add the scene
	scene = profile->createScene(QString(), before);
	profile->setActiveScene(scene); // Active by default
}

void MainPage::showTransitionMenu(uint transitionId, QWidget *btn)
{
	// Get transition information
	PrflTransition transition;
	uint durationMsec;
	Profile *profile = App->getProfile();
	profile->getTransition(transitionId, transition, durationMsec);

	// Update menu contents
	m_transitionMenuId = transitionId;
	m_transitionDurAction->setText(tr("Current duration: %L1 msec")
		.arg(durationMsec));
	m_transitionInstantAction->setChecked(
		transition == PrflInstantTransition);
	m_transitionFadeAction->setChecked(
		transition == PrflFadeTransition);
	m_transitionFadeBlackAction->setChecked(
		transition == PrflFadeBlackTransition);
	m_transitionFadeWhiteAction->setChecked(
		transition == PrflFadeWhiteTransition);

	// Popup above the button
	QPoint pos = btn->mapToGlobal(
		QPoint(0, -m_transitionMenu.sizeHint().height()));
	m_transitionMenu.popup(pos);
}

void MainPage::transALeftClicked()
{
	Profile *profile = App->getProfile();
	profile->setActiveTransition(0);
}

void MainPage::transARightClicked()
{
	showTransitionMenu(0, &m_transARightButton);
}

void MainPage::transBLeftClicked()
{
	Profile *profile = App->getProfile();
	profile->setActiveTransition(1);
}

void MainPage::transBRightClicked()
{
	showTransitionMenu(1, &m_transBRightButton);
}

void MainPage::transCLeftClicked()
{
	Profile *profile = App->getProfile();
	profile->setActiveTransition(2);
}

void MainPage::transCRightClicked()
{
	showTransitionMenu(2, &m_transCRightButton);
}

void MainPage::updatesAvailableClicked()
{
	App->showRunUpdaterDialog();
}

void MainPage::modifyTargetsClicked()
{
	if(App->isBroadcasting()) {
		App->showBasicWarningDialog(
			tr("Cannot modify targets while broadcasting."));
		return;
	}
	App->showWizard(WizEditTargetsController);
}

void MainPage::modifyMixerClicked()
{
	if(App->isBroadcasting()) {
		App->showBasicWarningDialog(
			tr("Cannot modify audio mixer while broadcasting."));
		return;
	}
	App->showWizard(WizAudioMixerController);
}

void MainPage::zoomSliderChanged(float zoom, bool byUser)
{
	// Zoom towards the center of the screen
	m_graphicsWidget.setZoom(zoom,
		m_graphicsWidget.mapWidgetToCanvasPos(
		m_graphicsWidget.rect().center()));
	if(byUser)
		m_graphicsWidget.setAutoZoomEnabled(false);
}

void MainPage::itemChanged(SceneItem *item)
{
	if(item == NULL || item->isGroup())
		showPropertiesToolbox(false);
	else
		showPropertiesToolbox(true);
}

void MainPage::activeSceneChanged(Scene *scene, Scene *oldScene)
{
	if(scene == oldScene)
		return; // Should never happen

	// Prevent ugly repaints. The main window will automatically reenable
	// updates in a few milliseconds.
	App->disableMainWindowUpdates();

	if(oldScene != NULL) {
		disconnect(oldScene, &Scene::activeItemChanged,
			this, &MainPage::itemChanged);
	}
	if(scene != NULL) {
		connect(scene, &Scene::activeItemChanged,
			this, &MainPage::itemChanged);
		itemChanged(scene->getActiveItem());
	} else
		showPropertiesToolbox(false);
	m_itemView->setScene(scene);
}

/// <summary>
/// Called whenever something relating to transitions changed.
/// </summary>
void MainPage::transitionsChanged()
{
	bool isActive;
	PrflTransition transition;
	uint durationMsec;
	Profile *profile = App->getProfile();

	// Configure fonts
	QFont font = m_transALeftButton.font();
	QFont fontAct = font; // Active font
	font.setBold(false);
	fontAct.setBold(true);

	profile->getTransition(0, transition, durationMsec);
	isActive = (profile->getActiveTransition() == 0);
	m_transALeftButton.setText(tr(PrflTransitionShortStrings[transition]));
	m_transALeftButton.setToolTip(
		tr("%1 transition").arg(tr(PrflTransitionStrings[transition])));
	if(isActive) {
		m_transALeftButton.setColorSet(StyleHelper::BlueSet);
		m_transARightButton.setColorSet(StyleHelper::BlueSet);
		m_transALeftButton.setFont(fontAct);
		m_transARightButton.setIcon(
			QIcon(":/Resources/dark-indicator-up-white.png"));
	} else {
		m_transALeftButton.setColorSet(StyleHelper::ButtonSet);
		m_transARightButton.setColorSet(StyleHelper::ButtonSet);
		m_transALeftButton.setFont(font);
		m_transARightButton.setIcon(
			QIcon(":/Resources/dark-indicator-up.png"));
	}

	profile->getTransition(1, transition, durationMsec);
	isActive = (profile->getActiveTransition() == 1);
	m_transBLeftButton.setText(tr(PrflTransitionShortStrings[transition]));
	m_transBLeftButton.setToolTip(
		tr("%1 transition").arg(tr(PrflTransitionStrings[transition])));
	if(isActive) {
		m_transBLeftButton.setColorSet(StyleHelper::BlueSet);
		m_transBRightButton.setColorSet(StyleHelper::BlueSet);
		m_transBLeftButton.setFont(fontAct);
		m_transBRightButton.setIcon(
			QIcon(":/Resources/dark-indicator-up-white.png"));
	} else {
		m_transBLeftButton.setColorSet(StyleHelper::ButtonSet);
		m_transBRightButton.setColorSet(StyleHelper::ButtonSet);
		m_transBLeftButton.setFont(font);
		m_transBRightButton.setIcon(
			QIcon(":/Resources/dark-indicator-up.png"));
	}

	profile->getTransition(2, transition, durationMsec);
	isActive = (profile->getActiveTransition() == 2);
	m_transCLeftButton.setText(tr(PrflTransitionShortStrings[transition]));
	m_transCLeftButton.setToolTip(
		tr("%1 transition").arg(tr(PrflTransitionStrings[transition])));
	if(isActive) {
		m_transCLeftButton.setColorSet(StyleHelper::BlueSet);
		m_transCRightButton.setColorSet(StyleHelper::BlueSet);
		m_transCLeftButton.setFont(fontAct);
		m_transCRightButton.setIcon(
			QIcon(":/Resources/dark-indicator-up-white.png"));
	} else {
		m_transCLeftButton.setColorSet(StyleHelper::ButtonSet);
		m_transCRightButton.setColorSet(StyleHelper::ButtonSet);
		m_transCLeftButton.setFont(font);
		m_transCRightButton.setIcon(
			QIcon(":/Resources/dark-indicator-up.png"));
	}
}

void MainPage::transitionMenuTriggered(QAction *action)
{
	// Get transition information
	PrflTransition transition;
	uint durationMsec;
	Profile *profile = App->getProfile();
	profile->getTransition(m_transitionMenuId, transition, durationMsec);

	if(action == m_transitionDurChangeAction) {
		showTransitionDurDialog(durationMsec);
	} else if(action == m_transitionInstantAction) {
		// Change transition to instant
		profile->setTransition(
			m_transitionMenuId, PrflInstantTransition, durationMsec);
	} else if(action == m_transitionFadeAction) {
		// Change transition to fade
		profile->setTransition(
			m_transitionMenuId, PrflFadeTransition, durationMsec);
	} else if(action == m_transitionFadeBlackAction) {
		// Change transition to fade through black
		profile->setTransition(
			m_transitionMenuId, PrflFadeBlackTransition, durationMsec);
	} else if(action == m_transitionFadeWhiteAction) {
		// Change transition to fade through white
		profile->setTransition(
			m_transitionMenuId, PrflFadeWhiteTransition, durationMsec);
	}
}

void MainPage::showTransitionDurDialog(uint durationMsec)
{
	// Create delay dialog
	m_transitionDurDialog = new QInputDialog(this);
	connect(m_transitionDurDialog, &QInputDialog::finished,
		this, &MainPage::transitionDurDialogClosed);
	m_transitionDurDialog->setInputMode(QInputDialog::IntInput);
	m_transitionDurDialog->setIntMinimum(0 * 1000); // 0 seconds
	m_transitionDurDialog->setIntMaximum(10 * 1000); // 10 seconds
	m_transitionDurDialog->setLabelText(
		tr("Transition duration (Milliseconds):"));
	m_transitionDurDialog->setIntValue(durationMsec);
	m_transitionDurDialog->setModal(true);
	m_transitionDurDialog->show();
}

void MainPage::transitionDurDialogClosed(int result)
{
	if(result == QDialog::Accepted) {
		// Get transition information
		PrflTransition transition;
		uint durationMsec;
		Profile *profile = App->getProfile();
		profile->getTransition(m_transitionMenuId, transition, durationMsec);
		profile->setTransition(
			m_transitionMenuId, transition, m_transitionDurDialog->intValue());
	}

	// Destroy delay dialog
	m_transitionDurDialog->deleteLater();
	m_transitionDurDialog = NULL;
}
