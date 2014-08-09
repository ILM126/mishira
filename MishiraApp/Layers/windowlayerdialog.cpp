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

#include "windowlayerdialog.h"
#include "application.h"
#include "windowlayer.h"
#include "validators.h"
#include <Libdeskcap/capturemanager.h>

WindowLayerDialog::WindowLayerDialog(WindowLayer *layer, QWidget *parent)
	: LayerDialog(layer, parent)
	, m_ui()
	, m_windows()
	, m_ignoreSignals(false)
{
	m_ui.setupUi(this);
	m_windows.reserve(16);

	// Setup custom widgets
	m_ui.leftCropEdit->setUnits(tr("px"));
	m_ui.rightCropEdit->setUnits(tr("px"));
	m_ui.topCropEdit->setUnits(tr("px"));
	m_ui.botCropEdit->setUnits(tr("px"));

	// Populate window select combo box
	refreshClicked();

	//-------------------------------------------------------------------------
	// Capture tab

	// Connect button signals
	connect(m_ui.refreshBtn, &QPushButton::clicked,
		this, &WindowLayerDialog::refreshClicked);
	connect(m_ui.addBtn, &QPushButton::clicked,
		this, &WindowLayerDialog::addWindowClicked);
	connect(m_ui.removeBtn, &QPushButton::clicked,
		this, &WindowLayerDialog::removeWindowClicked);
	connect(m_ui.moveUpBtn, &QPushButton::clicked,
		this, &WindowLayerDialog::moveUpClicked);
	connect(m_ui.moveDownBtn, &QPushButton::clicked,
		this, &WindowLayerDialog::moveDownClicked);

	// Notify the dialog when settings change
	connect(m_ui.showCursorBtn, &QPushButton::clicked,
		this, &LayerDialog::settingModified);
	connect(m_ui.hideCursorBtn, &QPushButton::clicked,
		this, &LayerDialog::settingModified);

	//-------------------------------------------------------------------------
	// Cropping tab

	// Setup validators
	m_ui.leftCropEdit->setValidator(new QIntValidator(0, INT_MAX, this));
	m_ui.rightCropEdit->setValidator(new QIntValidator(0, INT_MAX, this));
	m_ui.topCropEdit->setValidator(new QIntValidator(0, INT_MAX, this));
	m_ui.botCropEdit->setValidator(new QIntValidator(0, INT_MAX, this));
	connect(m_ui.leftCropEdit, &QLineEdit::textChanged,
		this, &WindowLayerDialog::leftCropEditChanged);
	connect(m_ui.rightCropEdit, &QLineEdit::textChanged,
		this, &WindowLayerDialog::rightCropEditChanged);
	connect(m_ui.topCropEdit, &QLineEdit::textChanged,
		this, &WindowLayerDialog::topCropEditChanged);
	connect(m_ui.botCropEdit, &QLineEdit::textChanged,
		this, &WindowLayerDialog::botCropEditChanged);

	// Notify the dialog when settings change
	connect(m_ui.leftCropEdit, &QLineEdit::textChanged,
		this, &LayerDialog::settingModified);
	connect(m_ui.rightCropEdit, &QLineEdit::textChanged,
		this, &LayerDialog::settingModified);
	connect(m_ui.topCropEdit, &QLineEdit::textChanged,
		this, &LayerDialog::settingModified);
	connect(m_ui.botCropEdit, &QLineEdit::textChanged,
		this, &LayerDialog::settingModified);

	//-------------------------------------------------------------------------
	// Colour adjustment tab

	// Connect widget signals
	void (QSpinBox:: *fpiSB)(int) = &QSpinBox::valueChanged;
	void (QDoubleSpinBox:: *fpiDSB)(double) = &QDoubleSpinBox::valueChanged;
	connect(m_ui.gammaSlider, &QAbstractSlider::valueChanged,
		this, &WindowLayerDialog::gammaSliderChanged);
	connect(m_ui.gammaBox, fpiDSB,
		this, &WindowLayerDialog::gammaBoxChanged);
	connect(m_ui.brightnessSlider, &QAbstractSlider::valueChanged,
		this, &WindowLayerDialog::brightnessChanged);
	connect(m_ui.brightnessBox, fpiSB,
		this, &WindowLayerDialog::brightnessChanged);
	connect(m_ui.contrastSlider, &QAbstractSlider::valueChanged,
		this, &WindowLayerDialog::contrastChanged);
	connect(m_ui.contrastBox, fpiSB,
		this, &WindowLayerDialog::contrastChanged);
	connect(m_ui.saturationSlider, &QAbstractSlider::valueChanged,
		this, &WindowLayerDialog::saturationChanged);
	connect(m_ui.saturationBox, fpiSB,
		this, &WindowLayerDialog::saturationChanged);

	// Notify the dialog when settings change
	connect(m_ui.brightnessSlider, &QAbstractSlider::valueChanged,
		this, &LayerDialog::settingModified);
	connect(m_ui.brightnessBox, fpiSB,
		this, &LayerDialog::settingModified);
	connect(m_ui.contrastSlider, &QAbstractSlider::valueChanged,
		this, &LayerDialog::settingModified);
	connect(m_ui.contrastBox, fpiSB,
		this, &LayerDialog::settingModified);
	connect(m_ui.saturationSlider, &QAbstractSlider::valueChanged,
		this, &LayerDialog::settingModified);
	connect(m_ui.saturationBox, fpiSB,
		this, &LayerDialog::settingModified);
}

WindowLayerDialog::~WindowLayerDialog()
{
	while(m_windows.count()) {
		delete m_windows.last();
		m_windows.pop_back();
	}
}

void WindowLayerDialog::loadSettings()
{
	WindowLayer *layer = static_cast<WindowLayer *>(m_layer);

	// Reset window list
	while(m_windows.count()) {
		delete m_windows.last();
		m_windows.pop_back();
	}
	QStringList exeList = layer->getExeFilenameList();
	QStringList titleList = layer->getWindowTitleList();
	for(int i = 0; i < exeList.count(); i++) {
		const QString exe = exeList.at(i);
		const QString title = titleList.at(i);
		const QString friendly = QStringLiteral("[%1] %2").arg(exe).arg(title);

		// Create list widget item
		QListWidgetItem *item = new QListWidgetItem(friendly);
		item->setData(Qt::UserRole, exe);
		item->setData(Qt::UserRole + 1, title);

		// Add item to list
		m_windows.append(item);
		m_ui.listWidget->addItem(item);
	}

	// Mouse cursor
	m_ui.showCursorBtn->setChecked(layer->getCaptureMouse());
	m_ui.hideCursorBtn->setChecked(!layer->getCaptureMouse());

	// Cropping information
	const CropInfo &cropInfo = layer->getCropInfo();
	m_ui.leftCropEdit->setText(QString::number(cropInfo.getLeftMargin()));
	m_ui.rightCropEdit->setText(QString::number(cropInfo.getRightMargin()));
	m_ui.topCropEdit->setText(QString::number(cropInfo.getTopMargin()));
	m_ui.botCropEdit->setText(QString::number(cropInfo.getBottomMargin()));
	m_ui.leftCropAnchorBox->setCurrentIndex((int)cropInfo.getLeftAnchor());
	m_ui.rightCropAnchorBox->setCurrentIndex((int)cropInfo.getRightAnchor());
	m_ui.topCropAnchorBox->setCurrentIndex((int)cropInfo.getTopAnchor());
	m_ui.botCropAnchorBox->setCurrentIndex((int)cropInfo.getBottomAnchor());

	// Colour adjustment
	m_ignoreSignals = true;
	m_ui.gammaSlider->setValue(qRound(log10(layer->getGamma()) * 500.0f));
	m_ui.gammaBox->setValue(layer->getGamma());
	m_ui.brightnessSlider->setValue(layer->getBrightness());
	m_ui.brightnessBox->setValue(layer->getBrightness());
	m_ui.contrastSlider->setValue(layer->getContrast());
	m_ui.contrastBox->setValue(layer->getContrast());
	m_ui.saturationSlider->setValue(layer->getSaturation());
	m_ui.saturationBox->setValue(layer->getSaturation());
	m_ignoreSignals = false;
}

void WindowLayerDialog::applySettings()
{
	WindowLayer *layer = static_cast<WindowLayer *>(m_layer);

	// Window list
	layer->beginAddingWindows();
	for(int i = 0; i < m_windows.count(); i++) {
		QListWidgetItem *item = m_windows.at(i);
		const QString exe = item->data(Qt::UserRole).toString();
		const QString title = item->data(Qt::UserRole + 1).toString();
		layer->addWindow(exe, title);
	}
	layer->finishAddingWindows();

	// Mouse cursor
	layer->setCaptureMouse(m_ui.showCursorBtn->isChecked());

	// Capture method
	CptrMethod method = CptrAutoMethod; // Hard-coded
	layer->setCaptureMethod(method);

	// Cropping information
	CropInfo cropInfo;
	if(m_ui.leftCropEdit->hasAcceptableInput())
		cropInfo.setLeftMargin(m_ui.leftCropEdit->text().toInt());
	if(m_ui.rightCropEdit->hasAcceptableInput())
		cropInfo.setRightMargin(m_ui.rightCropEdit->text().toInt());
	if(m_ui.topCropEdit->hasAcceptableInput())
		cropInfo.setTopMargin(m_ui.topCropEdit->text().toInt());
	if(m_ui.botCropEdit->hasAcceptableInput())
		cropInfo.setBottomMargin(m_ui.botCropEdit->text().toInt());
	cropInfo.setLeftAnchor(
		(CropInfo::Anchor)m_ui.leftCropAnchorBox->currentIndex());
	cropInfo.setRightAnchor(
		(CropInfo::Anchor)m_ui.rightCropAnchorBox->currentIndex());
	cropInfo.setTopAnchor(
		(CropInfo::Anchor)m_ui.topCropAnchorBox->currentIndex());
	cropInfo.setBottomAnchor(
		(CropInfo::Anchor)m_ui.botCropAnchorBox->currentIndex());
	layer->setCropInfo(cropInfo);

	// Colour adjustment
	layer->setGamma(m_ui.gammaBox->value());
	layer->setBrightness(m_ui.brightnessSlider->value());
	layer->setContrast(m_ui.contrastSlider->value());
	layer->setSaturation(m_ui.saturationSlider->value());
}

void WindowLayerDialog::leftCropEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.leftCropEdit);
	//emit validityMaybeChanged(isValid());
}

void WindowLayerDialog::rightCropEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.rightCropEdit);
	//emit validityMaybeChanged(isValid());
}

void WindowLayerDialog::topCropEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.topCropEdit);
	//emit validityMaybeChanged(isValid());
}

void WindowLayerDialog::botCropEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.botCropEdit);
	//emit validityMaybeChanged(isValid());
}

/// <summary>
/// Refreshes the available window list
/// </summary>
void WindowLayerDialog::refreshClicked()
{
	// Generate the list of available windows and sort them. We cannot add them
	// directly to the combo box as the user data doesn't move when we reorder
	// the items.
	CaptureManager *mgr = App->getCaptureManager();
	mgr->cacheWindowList();
	QVector<WinId> windows = mgr->getWindowList();
	QVector<WindowSortData> sortData;
	sortData.reserve(windows.size());
	for(int i = 0; i < windows.count(); i++) {
		WinId winId = windows.at(i);
		const QString exe = mgr->getWindowExeFilename(winId);
		const QString title = mgr->getWindowTitle(winId);
		const QString itemName = QStringLiteral("[%1] %2").arg(exe).arg(title);

		// Alphabetically sort items as they are processed
		int j = 0;
		if(sortData.size()) {
			for(; j < sortData.size(); j++) {
				const WindowSortData &data = sortData.at(j);
				if(data.name.compare(itemName, Qt::CaseInsensitive) > 0) {
					j--;
					break;
				}
			}
			j++;
		}

		// Insert the item to our list
		WindowSortData data;
		data.name = itemName;
		data.exe = exe;
		data.title = title;
		if(j <= sortData.size())
			sortData.insert(j, data);
		else
			sortData.append(data);
	}
	mgr->uncacheWindowList();

	// Add the items to the actual combo box
	m_ui.windowCombo->clear();
	for(int i = 0; i < sortData.size(); i++) {
		const WindowSortData &data = sortData.at(i);
		m_ui.windowCombo->addItem(data.name);
		m_ui.windowCombo->setItemData(
			i, qVariantFromValue<QString>(data.exe), Qt::UserRole);
		m_ui.windowCombo->setItemData(
			i, qVariantFromValue<QString>(data.title), Qt::UserRole + 1);
	}
}

void WindowLayerDialog::addWindowClicked()
{
	// Get the currently selected window
	int i = m_ui.windowCombo->currentIndex();
	if(i < 0)
		return;
	const QString exe = m_ui.windowCombo->itemData(i, Qt::UserRole).toString();
	const QString title =
		m_ui.windowCombo->itemData(i, Qt::UserRole + 1).toString();
	const QString friendly = QStringLiteral("[%1] %2").arg(exe).arg(title);

	// TODO: Test to see if it's a duplicate (Fuzzy or exact match?)

	// Create list widget item
	QListWidgetItem *item = new QListWidgetItem(friendly);
	item->setData(Qt::UserRole, exe);
	item->setData(Qt::UserRole + 1, title);

	// Add item to list
	m_windows.insert(0, item);
	m_ui.listWidget->insertItem(0, item);
	settingModified();
}

void WindowLayerDialog::removeWindowClicked()
{
	QListWidgetItem *item = m_ui.listWidget->currentItem();
	if(item == NULL)
		return; // Nothing selected
	int id = m_windows.indexOf(item);
	if(id >= 0)
		m_windows.remove(id);
	delete item; // Removes from the list widget as well
	settingModified();
}

void WindowLayerDialog::moveUpClicked()
{
	QListWidgetItem *item = m_ui.listWidget->currentItem();
	if(item == NULL)
		return; // Nothing selected
	int id = m_windows.indexOf(item);
	int newId = qMax(id - 1, 0);
	if(id < 0 || id == newId)
		return;
	m_windows.remove(id);
	m_windows.insert(newId, item);
	item = m_ui.listWidget->takeItem(id); // Should return the same item
	m_ui.listWidget->insertItem(newId, item);
	m_ui.listWidget->setCurrentItem(item);
	settingModified();
}

void WindowLayerDialog::moveDownClicked()
{
	QListWidgetItem *item = m_ui.listWidget->currentItem();
	if(item == NULL)
		return; // Nothing selected
	int id = m_windows.indexOf(item);
	int newId = qMin(id + 1, m_windows.count() - 1);
	if(id < 0 || id == newId)
		return;
	m_windows.remove(id);
	m_windows.insert(newId, item);
	item = m_ui.listWidget->takeItem(id); // Should return the same item
	m_ui.listWidget->insertItem(newId, item);
	m_ui.listWidget->setCurrentItem(item);
	settingModified();
}

void WindowLayerDialog::gammaSliderChanged(int value)
{
	if(m_ignoreSignals)
		return;
	m_ignoreSignals = true;
	//m_ui.gammaSlider->setValue(value);
	m_ui.gammaBox->setValue(pow(10.0f, (float)value / 500.0f));
	m_ignoreSignals = false;
}

void WindowLayerDialog::gammaBoxChanged(double value)
{
	if(m_ignoreSignals)
		return;
	m_ignoreSignals = true;
	m_ui.gammaSlider->setValue(qRound(log10((float)value) * 500.0f));
	//m_ui.gammaBox->setValue(value);
	m_ignoreSignals = false;
}

void WindowLayerDialog::brightnessChanged(int value)
{
	if(m_ignoreSignals)
		return;
	m_ignoreSignals = true;
	m_ui.brightnessSlider->setValue(value);
	m_ui.brightnessBox->setValue(value);
	m_ignoreSignals = false;
}

void WindowLayerDialog::contrastChanged(int value)
{
	if(m_ignoreSignals)
		return;
	m_ignoreSignals = true;
	m_ui.contrastSlider->setValue(value);
	m_ui.contrastBox->setValue(value);
	m_ignoreSignals = false;
}

void WindowLayerDialog::saturationChanged(int value)
{
	if(m_ignoreSignals)
		return;
	m_ignoreSignals = true;
	m_ui.saturationSlider->setValue(value);
	m_ui.saturationBox->setValue(value);
	m_ignoreSignals = false;
}
