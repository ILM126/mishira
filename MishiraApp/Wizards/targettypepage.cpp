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

#include "targettypepage.h"
#include "wizardwindow.h"
#include "Widgets/targettypewidget.h"

TargetTypePage::TargetTypePage(QWidget *parent)
	: QWidget(parent)
	, m_ui()
	, m_typeWidgets()
	, m_typeBtnGroup()
{
	m_ui.setupUi(this);
}

TargetTypePage::~TargetTypePage()
{
	cleanUpTypePage();
}

void TargetTypePage::cleanUpTypePage()
{
	// Remove all our target radio buttons from the group
	for(int i = 0; i < m_typeWidgets.count(); i++)
		m_typeBtnGroup.removeButton(m_typeWidgets.at(i)->getButton());

	// Deleting our widgets removes them from the UI layout
	for(int i = 0; i < m_typeWidgets.count(); i++)
		delete m_typeWidgets.at(i);
	m_typeWidgets.clear();
}

void TargetTypePage::addTargetType(
	QGridLayout *layout, TrgtType type, const QString &tooltip,
	const QPixmap &icon, int row, int column)
{
	TargetTypeWidget *widget = new TargetTypeWidget(type, icon);
	widget->setToolTip(tooltip);
	m_typeWidgets.append(widget);
	layout->addWidget(widget, row, column);
	m_typeBtnGroup.addButton(widget->getButton());
}

/// <summary>
/// Reset button code for the page that is shared between multiple controllers.
/// </summary>
void TargetTypePage::sharedReset(
	WizardWindow *wizWin, WizTargetSettings *defaults, bool isNewProfile)
{
	setUpdatesEnabled(false);

	// Clean up if we've already loaded this page
	cleanUpTypePage();

	// Set the page title depending on what wizard this is
	if(isNewProfile)
		m_ui.titleLbl->setText(tr("Where do you want to broadcast to?"));
	else
		m_ui.titleLbl->setText(tr("Select target type"));

	// Fill target type list
	QGridLayout *layout = m_ui.typeListLayout;
	addTargetType(
		layout, TrgtFileType, tr("Local file"),
		QPixmap(":/Resources/target-200x100-file.png"), 0, 0);
	//addTargetType(
	//	layout, TrgtFileType, tr("YouTube"),
	//	QPixmap(":/Resources/target-200x100-youtube.png"), 0, 1);
	addTargetType(
		layout, TrgtTwitchType, tr("Twitch"),
		QPixmap(":/Resources/target-200x100-twitch.png"), 0, 1);
	//addTargetType(
	//	layout, TrgtFileType, tr("Livestream"),
	//	QPixmap(":/Resources/target-200x100-livestream.png"), 1, 0);
	addTargetType(
		layout, TrgtUstreamType, tr("Ustream"),
		QPixmap(":/Resources/target-200x100-ustream.png"), 0, 2);
	addTargetType(
		layout, TrgtHitboxType, tr("Hitbox"),
		QPixmap(":/Resources/target-200x100-hitbox.png"), 1, 0);
	addTargetType(
		layout, TrgtRtmpType, tr("Other RTMP"),
		QPixmap(":/Resources/target-200x100-rtmp.png"), 1, 1);

	// Spacer to make sure all targets are at the top of the scroll area
	layout->addItem(
		new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding),
		3, 0);

	// Select the first target type
	m_typeWidgets.at(0)->select();

	setUpdatesEnabled(true);

	// Page is always valid
	wizWin->setCanContinue(true);
}

/// <summary>
/// Next button code for the page that is shared between multiple controllers.
/// </summary>
void TargetTypePage::sharedNext(WizTargetSettings *settings)
{
	// Save selected target type
	for(int i = 0; i < m_typeWidgets.count(); i++) {
		TargetTypeWidget *widget = m_typeWidgets.at(i);
		if(!widget->isSelected())
			continue;
		settings->type = widget->getType();
		break;
	}
}
