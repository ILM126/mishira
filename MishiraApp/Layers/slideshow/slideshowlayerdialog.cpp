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

#include "slideshowlayerdialog.h"
#include "slideshowlayer.h"
#include "validators.h"

SlideshowLayerDialog::SlideshowLayerDialog(
	SlideshowLayer *layer, QWidget *parent)
	: LayerDialog(layer, parent)
	, m_ui()
	, m_fileDialog(this)
{
	m_ui.setupUi(this);

	// Setup custom widgets
	m_ui.delayEdit->setUnits(tr("secs"));
	m_ui.transitionEdit->setUnits(tr("secs"));

	// Setup file dialog and connect it to the add image button
	//m_fileDialog.setFileMode(FileOpenDialog::ExistingFile);
	QStringList filters;
	filters
		<< "Image files (*.png;*.jpg;*.jpeg;*.gif;*.bmp)"
		<< "All files (*.*)";
	m_fileDialog.setNameFilters(filters);
	//m_fileDialog.setViewMode(FileOpenDialog::List);
	// TODO: Default directory
	connect(&m_fileDialog, &FileOpenDialog::fileSelected,
		this, &SlideshowLayerDialog::fileSelected);
	connect(m_ui.addBtn, &QPushButton::clicked,
		&m_fileDialog, &FileOpenDialog::open);

	// Connect other buttons
	connect(m_ui.removeBtn, &QPushButton::clicked,
		this, &SlideshowLayerDialog::removeImageClicked);
	connect(m_ui.moveUpBtn, &QPushButton::clicked,
		this, &SlideshowLayerDialog::moveUpClicked);
	connect(m_ui.moveDownBtn, &QPushButton::clicked,
		this, &SlideshowLayerDialog::moveDownClicked);

	// Setup validators
	m_ui.delayEdit->setValidator(
		new QDoubleValidator(0.0, 60.0 * 60.0 * 24.0, 3, this));
	m_ui.transitionEdit->setValidator(
		new QDoubleValidator(0.0, 60.0, 3, this));
	connect(m_ui.delayEdit, &QLineEdit::textChanged,
		this, &SlideshowLayerDialog::delayEditChanged);
	connect(m_ui.transitionEdit, &QLineEdit::textChanged,
		this, &SlideshowLayerDialog::transitionEditChanged);

	// Notify the dialog when settings change
	connect(m_ui.delayEdit, &QLineEdit::textChanged,
		this, &LayerDialog::settingModified);
	connect(m_ui.transitionEdit, &QLineEdit::textChanged,
		this, &LayerDialog::settingModified);
	connect(m_ui.inThenOutBtn, &QAbstractButton::clicked,
		this, &LayerDialog::settingModified);
	connect(m_ui.inAndOutBtn, &QAbstractButton::clicked,
		this, &LayerDialog::settingModified);
	connect(m_ui.seqBtn, &QAbstractButton::clicked,
		this, &LayerDialog::settingModified);
	connect(m_ui.randomBtn, &QAbstractButton::clicked,
		this, &LayerDialog::settingModified);
}

SlideshowLayerDialog::~SlideshowLayerDialog()
{
}

void SlideshowLayerDialog::loadSettings()
{
	SlideshowLayer *layer = static_cast<SlideshowLayer *>(m_layer);

	// Filename list
	m_ui.listWidget->clear();
	const QStringList &filenames = layer->getFilenames();
	for(int i = 0; i < filenames.count(); i++) {
		const QString filename = filenames.at(i);
		QListWidgetItem *item = new QListWidgetItem(filename);
		m_ui.listWidget->addItem(item);
	}

	// Text fields
	m_ui.delayEdit->setText(QString::number(layer->getDelayTime()));
	m_ui.transitionEdit->setText(QString::number(layer->getTransitionTime()));

	// Radio buttons
	switch(layer->getTransitionStyle()) {
	default:
	case InThenOutStyle:
		m_ui.inThenOutBtn->setChecked(true);
		m_ui.inAndOutBtn->setChecked(false);
		break;
	case InAndOutStyle:
		m_ui.inThenOutBtn->setChecked(false);
		m_ui.inAndOutBtn->setChecked(true);
		break;
	}
	switch(layer->getOrder()) {
	default:
	case SequentialOrder:
		m_ui.seqBtn->setChecked(true);
		m_ui.randomBtn->setChecked(false);
		break;
	case RandomOrder:
		m_ui.seqBtn->setChecked(false);
		m_ui.randomBtn->setChecked(true);
		break;
	}
}

void SlideshowLayerDialog::applySettings()
{
	SlideshowLayer *layer = static_cast<SlideshowLayer *>(m_layer);

	// Filename list
	QStringList filenames;
	for(int i = 0; i < m_ui.listWidget->count(); i++) {
		QListWidgetItem *item = m_ui.listWidget->item(i);
		filenames.append(item->text());
	}
	layer->setFilenames(filenames);

	// Text fields
	if(m_ui.delayEdit->hasAcceptableInput())
		layer->setDelayTime(m_ui.delayEdit->text().toFloat());
	if(m_ui.transitionEdit->hasAcceptableInput())
		layer->setTransitionTime(m_ui.transitionEdit->text().toFloat());

	// Radio buttons
	if(m_ui.inThenOutBtn->isChecked())
		layer->setTransitionStyle(InThenOutStyle);
	else if(m_ui.inAndOutBtn->isChecked())
		layer->setTransitionStyle(InAndOutStyle);
	if(m_ui.seqBtn->isChecked())
		layer->setOrder(SequentialOrder);
	else if(m_ui.randomBtn->isChecked())
		layer->setOrder(RandomOrder);
}

void SlideshowLayerDialog::fileSelected(const QString &filename)
{
	// Add the filename to the bottom of the list
	QListWidgetItem *item = new QListWidgetItem(filename);
	m_ui.listWidget->addItem(item);
	settingModified();
}

void SlideshowLayerDialog::removeImageClicked()
{
	QListWidgetItem *item = m_ui.listWidget->currentItem();
	if(item == NULL)
		return; // Nothing selected
	delete item; // Removes from the list widget as well
	settingModified();
}

void SlideshowLayerDialog::moveUpClicked()
{
	QListWidgetItem *item = m_ui.listWidget->currentItem();
	if(item == NULL)
		return; // Nothing selected
	int id = m_ui.listWidget->row(item);
	int newId = qMax(id - 1, 0);
	if(id < 0 || id == newId)
		return;
	item = m_ui.listWidget->takeItem(id); // Should return the same item
	m_ui.listWidget->insertItem(newId, item);
	m_ui.listWidget->setCurrentItem(item);
	settingModified();
}

void SlideshowLayerDialog::moveDownClicked()
{
	QListWidgetItem *item = m_ui.listWidget->currentItem();
	if(item == NULL)
		return; // Nothing selected
	int id = m_ui.listWidget->row(item);
	int newId = qMin(id + 1, m_ui.listWidget->count() - 1);
	if(id < 0 || id == newId)
		return;
	item = m_ui.listWidget->takeItem(id); // Should return the same item
	m_ui.listWidget->insertItem(newId, item);
	m_ui.listWidget->setCurrentItem(item);
	settingModified();
}

void SlideshowLayerDialog::delayEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.delayEdit);
	//emit validityMaybeChanged(isValid());
}

void SlideshowLayerDialog::transitionEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.transitionEdit);
	//emit validityMaybeChanged(isValid());
}
