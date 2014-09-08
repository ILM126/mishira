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

#include "scripttextlayerdialog.h"
#include "scripttextlayer.h"
#include <QtCore/QTimer>
#include <QtWidgets/QScrollBar>

ScriptTextLayerDialog::ScriptTextLayerDialog(
	ScriptTextLayer *layer, QWidget *parent)
	: LayerDialog(layer, parent)
	, m_ui()
	, m_ignoreSignals(false)
{
	m_ui.setupUi(this);

	// Setup custom widgets
	m_ui.strokeSizeEdit->setUnits(tr("px"));
	m_ui.xScrollEdit->setUnits(tr("px/sec"));
	m_ui.yScrollEdit->setUnits(tr("px/sec"));

	// Setup validators
	m_ui.strokeSizeEdit->setValidator(new QIntValidator(0, 20, this));
	m_ui.xScrollEdit->setValidator(new QIntValidator(INT_MIN, INT_MAX, this));
	m_ui.yScrollEdit->setValidator(new QIntValidator(INT_MIN, INT_MAX, this));
	connect(m_ui.strokeSizeEdit, &QLineEdit::textChanged,
		this, &ScriptTextLayerDialog::strokeSizeEditChanged);
	connect(m_ui.xScrollEdit, &QLineEdit::textChanged,
		this, &ScriptTextLayerDialog::xScrollEditChanged);
	connect(m_ui.yScrollEdit, &QLineEdit::textChanged,
		this, &ScriptTextLayerDialog::yScrollEditChanged);

	// Notify the dialog when settings change
	connect(m_ui.scriptEdit, &QPlainTextEdit::textChanged,
		this, &LayerDialog::settingModified);
	connect(m_ui.strokeSizeEdit, &QLineEdit::textChanged,
		this, &LayerDialog::settingModified);
	connect(m_ui.strokeColorBtn, &ColorButton::colorChanged,
		this, &LayerDialog::settingModified);
	connect(m_ui.wordWrapBtn, &QPushButton::clicked,
		this, &LayerDialog::settingModified);
	connect(m_ui.noWordWrapBtn, &QPushButton::clicked,
		this, &LayerDialog::settingModified);
	connect(m_ui.xScrollEdit, &QLineEdit::textChanged,
		this, &LayerDialog::settingModified);
	connect(m_ui.yScrollEdit, &QLineEdit::textChanged,
		this, &LayerDialog::settingModified);
}

ScriptTextLayerDialog::~ScriptTextLayerDialog()
{
}

void ScriptTextLayerDialog::loadSettings()
{
	ScriptTextLayer *layer = static_cast<ScriptTextLayer *>(m_layer);

	m_ui.scriptEdit->document()->setPlainText(layer->getScript());
	resetEditorScrollPos();

	m_ui.fontEdit->setCurrentFont(layer->getFont());
	m_ui.fontSizeEdit->setValue(layer->getFontSize());
	m_ui.boldBtn->setChecked(layer->getFontBold());
	m_ui.italicBtn->setChecked(layer->getFontItalic());
	m_ui.underlineBtn->setChecked(layer->getFontUnderline());
	m_ui.colorBtn->setColor(layer->getFontColor());

	m_ui.strokeSizeEdit->setText(QString::number(layer->getStrokeSize()));
	m_ui.strokeColorBtn->setColor(layer->getStrokeColor());
	m_ui.wordWrapBtn->setChecked(layer->getWordWrap());
	m_ui.noWordWrapBtn->setChecked(!layer->getWordWrap());
	m_ui.xScrollEdit->setText(QString::number(layer->getScrollSpeed().x()));
	m_ui.yScrollEdit->setText(QString::number(layer->getScrollSpeed().y()));
}

void ScriptTextLayerDialog::applySettings()
{
	ScriptTextLayer *layer = static_cast<ScriptTextLayer *>(m_layer);

	layer->setScript(m_ui.scriptEdit->document()->toPlainText());

	layer->setFont(m_ui.fontEdit->currentFont());
	layer->setFontSize(m_ui.fontSizeEdit->value());
	layer->setFontBold(m_ui.boldBtn->isChecked());
	layer->setFontItalic(m_ui.italicBtn->isChecked());
	layer->setFontUnderline(m_ui.underlineBtn->isChecked());
	layer->setFontColor(m_ui.colorBtn->getColor());

	if(m_ui.strokeSizeEdit->hasAcceptableInput())
		layer->setStrokeSize(m_ui.strokeSizeEdit->text().toInt());
	layer->setStrokeColor(m_ui.strokeColorBtn->getColor());
	layer->setWordWrap(m_ui.wordWrapBtn->isChecked());

	// Scroll speed
	QPoint scrollSpeed;
	if(m_ui.xScrollEdit->hasAcceptableInput())
		scrollSpeed.setX(m_ui.xScrollEdit->text().toInt());
	if(m_ui.yScrollEdit->hasAcceptableInput())
		scrollSpeed.setY(m_ui.yScrollEdit->text().toInt());
	layer->setScrollSpeed(scrollSpeed);

	layer->resetScript();
}

void ScriptTextLayerDialog::strokeSizeEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.strokeSizeEdit);
	//emit validityMaybeChanged(isValid());
}

void ScriptTextLayerDialog::xScrollEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.xScrollEdit);
	//emit validityMaybeChanged(isValid());
}

void ScriptTextLayerDialog::yScrollEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.yScrollEdit);
	//emit validityMaybeChanged(isValid());
}

void ScriptTextLayerDialog::resetEditorScrollPos()
{
	QTextCursor cursor = m_ui.scriptEdit->textCursor();
	cursor.setPosition(0);
	m_ui.scriptEdit->setTextCursor(cursor);
	//m_ui.scriptEdit->verticalScrollBar()->setValue(0);
}