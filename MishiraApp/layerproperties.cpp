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

#include "layerproperties.h"
#include "application.h"
#include "darkstyle.h"
#include "layer.h"
#include "profile.h"
#include "scene.h"
#include "sceneitem.h"
#include "stylehelper.h"
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGraphicsDropShadowEffect>

LayerProperties::LayerProperties(Profile *profile, QWidget *parent)
	: QWidget(parent)
	, m_ui()
	, m_profile(profile)
	, m_alignGroup(this)
	, m_ignoreSignals(false)
{
	m_ui.setupUi(this);

	// Setup label colours and shadows
	QPalette pal = palette();
	pal.setColor(QPalette::WindowText, StyleHelper::DarkFg2Color);
	setPalette(pal); // Must be done before applying the dark widget style!
	addShadowEffect(m_ui.xLbl);
	addShadowEffect(m_ui.yLbl);
	addShadowEffect(m_ui.widthLbl);
	addShadowEffect(m_ui.heightLbl);
	addShadowEffect(m_ui.opacityLbl);
	addShadowEffect(m_ui.scalingLbl);
	addShadowEffect(m_ui.alignmentLbl);

	// Apply dark widget style
	App->applyDarkStyle(m_ui.xEdit);
	App->applyDarkStyle(m_ui.yEdit);
	App->applyDarkStyle(m_ui.widthEdit);
	App->applyDarkStyle(m_ui.heightEdit);
	App->applyDarkStyle(m_ui.opacityEdit);
	App->applyDarkStyle(m_ui.scalingCombo);
	App->applyDarkStyle(m_ui.moreBtn);
	App->applyDarkStyle(m_ui.alignTLBtn);
	App->applyDarkStyle(m_ui.alignTCBtn);
	App->applyDarkStyle(m_ui.alignTRBtn);
	App->applyDarkStyle(m_ui.alignMLBtn);
	App->applyDarkStyle(m_ui.alignMCBtn);
	App->applyDarkStyle(m_ui.alignMRBtn);
	App->applyDarkStyle(m_ui.alignBLBtn);
	App->applyDarkStyle(m_ui.alignBCBtn);
	App->applyDarkStyle(m_ui.alignBRBtn);

	// Hide unimplemented widgets
	//m_ui.rotLbl->hide();
	//m_ui.rotEdit->hide();

	// Setup custom widgets
	m_ui.xEdit->setUnits(tr("px"));
	m_ui.yEdit->setUnits(tr("px"));
	m_ui.widthEdit->setUnits(tr("px"));
	m_ui.heightEdit->setUnits(tr("px"));
	//m_ui.rotEdit->setUnits(tr("deg"));
	m_ui.opacityEdit->setUnits(tr("%"));

	// Setup alignment radio button group
	m_alignGroup.addButton(m_ui.alignTLBtn, LyrTopLeftAlign);
	m_alignGroup.addButton(m_ui.alignTCBtn, LyrTopCenterAlign);
	m_alignGroup.addButton(m_ui.alignTRBtn, LyrTopRightAlign);
	m_alignGroup.addButton(m_ui.alignMLBtn, LyrMiddleLeftAlign);
	m_alignGroup.addButton(m_ui.alignMCBtn, LyrMiddleCenterAlign);
	m_alignGroup.addButton(m_ui.alignMRBtn, LyrMiddleRightAlign);
	m_alignGroup.addButton(m_ui.alignBLBtn, LyrBottomLeftAlign);
	m_alignGroup.addButton(m_ui.alignBCBtn, LyrBottomCenterAlign);
	m_alignGroup.addButton(m_ui.alignBRBtn, LyrBottomRightAlign);

	// Setup the "more" button
	QFont font = m_ui.moreBtn->font();
	font.setBold(true);
	m_ui.moreBtn->setFont(font);
	pal = m_ui.moreBtn->palette();
	pal.setColor(QPalette::Button, StyleHelper::BlueBaseColor);
	pal.setColor(QPalette::ButtonText, Qt::white);
	m_ui.moreBtn->setPalette(pal);
	connect(m_ui.moreBtn, &QAbstractButton::clicked,
		this, &LayerProperties::morePropertiesClicked);

	// Setup validators
	m_ui.xEdit->setValidator(new QIntValidator(INT_MIN, INT_MAX, this));
	m_ui.yEdit->setValidator(new QIntValidator(INT_MIN, INT_MAX, this));
	m_ui.widthEdit->setValidator(new QIntValidator(1, INT_MAX, this));
	m_ui.heightEdit->setValidator(new QIntValidator(1, INT_MAX, this));
	m_ui.opacityEdit->setValidator(new QIntValidator(0, 100, this));
	connect(m_ui.xEdit, &UnitLineEdit::textChanged,
		this, &LayerProperties::lineEditChanged);
	connect(m_ui.yEdit, &UnitLineEdit::textChanged,
		this, &LayerProperties::lineEditChanged);
	connect(m_ui.widthEdit, &UnitLineEdit::textChanged,
		this, &LayerProperties::lineEditChanged);
	connect(m_ui.heightEdit, &UnitLineEdit::textChanged,
		this, &LayerProperties::lineEditChanged);
	connect(m_ui.opacityEdit, &UnitLineEdit::textChanged,
		this, &LayerProperties::lineEditChanged);

	// Connect other widget signals
	void (QComboBox:: *fpiCB)(int) = &QComboBox::currentIndexChanged;
	connect(m_ui.scalingCombo, fpiCB,
		this, &LayerProperties::scalingChanged);
	void (QButtonGroup:: *fpiBG)(int) = &QButtonGroup::buttonClicked;
	connect(&m_alignGroup, fpiBG,
		this, &LayerProperties::alignmentChanged);

	// Watch active scene for changes
	activeSceneChanged(m_profile->getActiveScene(), NULL);
	connect(m_profile, &Profile::activeSceneChanged,
		this, &LayerProperties::activeSceneChanged);

	// Disable everything by default
	setAllEnabled(false);
}

LayerProperties::~LayerProperties()
{
}

void LayerProperties::setAllEnabled(bool enabled)
{
	m_ui.xLbl->setEnabled(enabled);
	m_ui.yLbl->setEnabled(enabled);
	m_ui.widthLbl->setEnabled(enabled);
	m_ui.heightLbl->setEnabled(enabled);
	m_ui.opacityLbl->setEnabled(enabled);
	m_ui.scalingLbl->setEnabled(enabled);
	m_ui.alignmentLbl->setEnabled(enabled);

	m_ui.xEdit->setEnabled(enabled);
	m_ui.yEdit->setEnabled(enabled);
	m_ui.widthEdit->setEnabled(enabled);
	m_ui.heightEdit->setEnabled(enabled);
	m_ui.opacityEdit->setEnabled(enabled);
	m_ui.scalingCombo->setEnabled(enabled);
	QList<QAbstractButton *> btns = m_alignGroup.buttons();
	for(int i = 0; i < btns.count(); i++)
		btns.at(i)->setEnabled(enabled);
}

void LayerProperties::addShadowEffect(QWidget *widget)
{
	// Add a drop shadow effect to the label. The label takes ownership of the
	// effect and will delete it when the label is deleted or when we assign a
	// new effect to the label.
	QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
	effect->setBlurRadius(1.0f);
	effect->setColor(StyleHelper::TextShadowColor);
	effect->setOffset(1.0f, 1.0f);
	widget->setGraphicsEffect(effect);
}

void LayerProperties::lineEditChanged(const QString &text)
{
	if(m_ignoreSignals)
		return;

	// Validate everything
	doQLineEditValidate(m_ui.xEdit);
	doQLineEditValidate(m_ui.yEdit);
	doQLineEditValidate(m_ui.widthEdit);
	doQLineEditValidate(m_ui.heightEdit);
	doQLineEditValidate(m_ui.opacityEdit);

	// Get layer object
	SceneItem *item = App->getActiveItem();
	if(item == NULL || item->isGroup())
		return; // We can only modify layers
	Layer *layer = item->getLayer();

	// Update layer transform
	if(m_ui.xEdit->hasAcceptableInput() && m_ui.yEdit->hasAcceptableInput() &&
		m_ui.widthEdit->hasAcceptableInput() &&
		m_ui.heightEdit->hasAcceptableInput())
	{
		layer->setRect(QRect(
			m_ui.xEdit->text().toInt(), m_ui.yEdit->text().toInt(),
			m_ui.widthEdit->text().toInt(), m_ui.heightEdit->text().toInt()));
	}
	//if(m_ui.rotEdit->hasAcceptableInput())
	//	layer->setRotationDeg(m_ui.rotEdit->text().toInt());
	if(m_ui.opacityEdit->hasAcceptableInput())
		layer->setOpacity((float)m_ui.opacityEdit->text().toInt() / 100.0f);
}

void LayerProperties::scalingChanged(int id)
{
	if(m_ignoreSignals)
		return;

	SceneItem *item = App->getActiveItem();
	if(item == NULL || item->isGroup())
		return; // We can only modify layers
	Layer *layer = item->getLayer();

	layer->setScaling((LyrScalingMode)id);
}

void LayerProperties::alignmentChanged(int id)
{
	if(m_ignoreSignals)
		return;

	SceneItem *item = App->getActiveItem();
	if(item == NULL || item->isGroup())
		return; // We can only modify layers
	Layer *layer = item->getLayer();

	layer->setAlignment((LyrAlignment)id);
}

void LayerProperties::morePropertiesClicked()
{
	SceneItem *item = App->getActiveItem();
	if(item == NULL || item->isGroup())
		return; // We can only modify layers
	Layer *layer = item->getLayer();

	if(layer->hasSettingsDialog())
		layer->showSettingsDialog();
}

void LayerProperties::activeSceneChanged(Scene *scene, Scene *oldScene)
{
	// Watch scene for changes
	if(oldScene != NULL) {
		disconnect(oldScene, &Scene::itemChanged,
			this, &LayerProperties::itemChanged);
		disconnect(oldScene, &Scene::activeItemChanged,
			this, &LayerProperties::activeItemChanged);
	}
	if(scene != NULL) {
		connect(scene, &Scene::itemChanged,
			this, &LayerProperties::itemChanged);
		connect(scene, &Scene::activeItemChanged,
			this, &LayerProperties::activeItemChanged);
		activeItemChanged(scene->getActiveItem());
	} else
		activeItemChanged(NULL);
}

void LayerProperties::itemChanged(SceneItem *item)
{
	if(item != App->getActiveItem())
		return; // Not the displayed item
	if(item == NULL || item->isGroup())
		return;

	//-------------------------------------------------------------------------
	// Update items. All line edits need to have their cursor position
	// remembered as this method is called WHILE the user is modifying the
	// field.

	int curPos = 0;
	m_ignoreSignals = true;
	Layer *layer = item->getLayer();

	if(!m_ui.xEdit->hasFocus() ||
		(m_ui.xEdit->hasFocus() && m_ui.xEdit->hasAcceptableInput()))
	{
		QString newText = QString::number(layer->getRect().x());
		if(m_ui.xEdit->text() != newText) {
			curPos = m_ui.xEdit->cursorPosition();
			m_ui.xEdit->setText(newText);
			m_ui.xEdit->setCursorPosition(curPos);
		}
	}

	if(!m_ui.yEdit->hasFocus() ||
		(m_ui.yEdit->hasFocus() && m_ui.yEdit->hasAcceptableInput()))
	{
		QString newText = QString::number(layer->getRect().y());
		if(m_ui.yEdit->text() != newText) {
			curPos = m_ui.yEdit->cursorPosition();
			m_ui.yEdit->setText(newText);
			m_ui.yEdit->setCursorPosition(curPos);
		}
	}

	if(!m_ui.widthEdit->hasFocus() ||
		(m_ui.widthEdit->hasFocus() && m_ui.widthEdit->hasAcceptableInput()))
	{
		QString newText = QString::number(layer->getRect().width());
		if(m_ui.widthEdit->text() != newText) {
			curPos = m_ui.widthEdit->cursorPosition();
			m_ui.widthEdit->setText(newText);
			m_ui.widthEdit->setCursorPosition(curPos);
		}
	}

	if(!m_ui.heightEdit->hasFocus() ||
		(m_ui.heightEdit->hasFocus() && m_ui.heightEdit->hasAcceptableInput()))
	{
		QString newText = QString::number(layer->getRect().height());
		if(m_ui.heightEdit->text() != newText) {
			curPos = m_ui.heightEdit->cursorPosition();
			m_ui.heightEdit->setText(newText);
			m_ui.heightEdit->setCursorPosition(curPos);
		}
	}

	//if(!m_ui.rotEdit->hasFocus() ||
	//	(m_ui.rotEdit->hasFocus() && m_ui.rotEdit->hasAcceptableInput()))
	//{
	//	QString newText = QString::number(layer->getRotationDeg());
	//	if(m_ui.rotEdit->text() != newText) {
	//		curPos = m_ui.rotEdit->cursorPosition();
	//		m_ui.rotEdit->setText(newText);
	//		m_ui.rotEdit->setCursorPosition(curPos);
	//	}
	//}

	if(!m_ui.opacityEdit->hasFocus() || (m_ui.opacityEdit->hasFocus() &&
		m_ui.opacityEdit->hasAcceptableInput()))
	{
		QString newText =
			QString::number(qRound(layer->getOpacity() * 100.0f));
		if(m_ui.opacityEdit->text() != newText) {
			curPos = m_ui.opacityEdit->cursorPosition();
			m_ui.opacityEdit->setText(newText);
			m_ui.opacityEdit->setCursorPosition(curPos);
		}
	}

	m_ui.scalingCombo->setCurrentIndex(layer->getScaling());
	m_alignGroup.button(layer->getAlignment())->setChecked(true);
	m_ignoreSignals = false;

	//-------------------------------------------------------------------------

	// Reset validity
	doQLineEditValidate(m_ui.xEdit);
	doQLineEditValidate(m_ui.yEdit);
	doQLineEditValidate(m_ui.widthEdit);
	doQLineEditValidate(m_ui.heightEdit);
	doQLineEditValidate(m_ui.opacityEdit);
}

void LayerProperties::activeItemChanged(SceneItem *item)
{
	if(item == NULL) {
		// The main page hides us so we don't need to disable our widgets
		//setAllEnabled(false);
	} else if(item->isGroup()) {
		// The main page hides us so we don't need to disable our widgets
		//setAllEnabled(false);
	} else {
		setAllEnabled(true);
	}
	itemChanged(item);
}
