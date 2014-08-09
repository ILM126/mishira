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

#include "imagelayerdialog.h"
#include "imagelayer.h"
#include "validators.h"

ImageLayerDialog::ImageLayerDialog(ImageLayer *layer, QWidget *parent)
	: LayerDialog(layer, parent)
	, m_ui()
	, m_fileDialog(this)
{
	m_ui.setupUi(this);

	// Setup custom widgets
	m_ui.xScrollEdit->setUnits(tr("px/sec"));
	m_ui.yScrollEdit->setUnits(tr("px/sec"));

	// Setup file dialog and connect it to the filename QLineEdit
	//m_fileDialog.setFileMode(FileOpenDialog::ExistingFile);
	QStringList filters;
	filters
		<< "Image files (*.png;*.jpg;*.jpeg;*.gif;*.bmp)"
		<< "All files (*.*)";
	m_fileDialog.setNameFilters(filters);
	//m_fileDialog.setViewMode(FileOpenDialog::List);
	// TODO: Default directory
	connect(&m_fileDialog, &FileOpenDialog::fileSelected,
		m_ui.filenameEdit, &QLineEdit::setText);
	connect(m_ui.filenameBtn, &QPushButton::clicked,
		&m_fileDialog, &FileOpenDialog::open);

	// Setup validators. Target name must start and end with a non-whitespace
	// character and be no longer than 24 characters total
	m_ui.filenameEdit->setValidator(new OpenFileValidator(this));
	m_ui.xScrollEdit->setValidator(new QIntValidator(INT_MIN, INT_MAX, this));
	m_ui.yScrollEdit->setValidator(new QIntValidator(INT_MIN, INT_MAX, this));
	connect(m_ui.filenameEdit, &QLineEdit::textChanged,
		this, &ImageLayerDialog::filenameEditChanged);
	connect(m_ui.xScrollEdit, &QLineEdit::textChanged,
		this, &ImageLayerDialog::xScrollEditChanged);
	connect(m_ui.yScrollEdit, &QLineEdit::textChanged,
		this, &ImageLayerDialog::yScrollEditChanged);

	// Notify the dialog when settings change
	connect(m_ui.filenameEdit, &QLineEdit::textChanged,
		this, &LayerDialog::settingModified);
	connect(m_ui.xScrollEdit, &QLineEdit::textChanged,
		this, &LayerDialog::settingModified);
	connect(m_ui.yScrollEdit, &QLineEdit::textChanged,
		this, &LayerDialog::settingModified);
}

ImageLayerDialog::~ImageLayerDialog()
{
}

void ImageLayerDialog::loadSettings()
{
	ImageLayer *layer = static_cast<ImageLayer *>(m_layer);
	m_ui.filenameEdit->setText(layer->getFilename());
	m_ui.xScrollEdit->setText(QString::number(layer->getScrollSpeed().x()));
	m_ui.yScrollEdit->setText(QString::number(layer->getScrollSpeed().y()));
}

void ImageLayerDialog::applySettings()
{
	ImageLayer *layer = static_cast<ImageLayer *>(m_layer);

	// Filename
	layer->setFilename(m_ui.filenameEdit->text());

	// Scroll speed
	QPoint scrollSpeed;
	if(m_ui.xScrollEdit->hasAcceptableInput())
		scrollSpeed.setX(m_ui.xScrollEdit->text().toInt());
	if(m_ui.yScrollEdit->hasAcceptableInput())
		scrollSpeed.setY(m_ui.yScrollEdit->text().toInt());
	layer->setScrollSpeed(scrollSpeed);
}

void ImageLayerDialog::filenameEditChanged(const QString &text)
{
	m_fileDialog.setInitialFilename(text);
	doQLineEditValidate(m_ui.filenameEdit);
	//emit validityMaybeChanged(isValid());
}

void ImageLayerDialog::xScrollEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.xScrollEdit);
	//emit validityMaybeChanged(isValid());
}

void ImageLayerDialog::yScrollEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.yScrollEdit);
	//emit validityMaybeChanged(isValid());
}
