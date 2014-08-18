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

#include "monitorlayerdialog.h"
#include "application.h"
#include "monitorlayer.h"
#include "validators.h"
#include <Libdeskcap/capturemanager.h>
#include <Libvidgfx/versionhelpers.h>

MonitorLayerDialog::MonitorLayerDialog(MonitorLayer *layer, QWidget *parent)
	: LayerDialog(layer, parent)
	, m_ui()
	, m_ignoreSignals(false)
{
	m_ui.setupUi(this);

	// Setup custom widgets
	m_ui.leftCropEdit->setUnits(tr("px"));
	m_ui.rightCropEdit->setUnits(tr("px"));
	m_ui.topCropEdit->setUnits(tr("px"));
	m_ui.botCropEdit->setUnits(tr("px"));

	// Populate monitor select combo box
	refreshMonitorList();

	//-------------------------------------------------------------------------
	// Capture tab

	// Notify the dialog when settings change
	connect(m_ui.showCursorBtn, &QAbstractButton::clicked,
		this, &LayerDialog::settingModified);
	connect(m_ui.hideCursorBtn, &QAbstractButton::clicked,
		this, &LayerDialog::settingModified);
	connect(m_ui.ignoreAeroBtn, &QAbstractButton::clicked,
		this, &LayerDialog::settingModified);
	connect(m_ui.disableAeroBtn, &QAbstractButton::clicked,
		this, &LayerDialog::settingModified);

	// Disabling Windows Aero is only required on Windows 7 and earlier as
	// there is no duplication API available. If the user is on Windows 8 or
	// later then the setting is always ignored and the widgets disabled. We
	// cannot hide the widgets as it changes the dialog design too much.
	bool aeroWidgetEnabled = true;
	if(IsWindows8OrGreater()) {
		m_ui.ignoreAeroBtn->setChecked(true);
		aeroWidgetEnabled = false;
	}
	m_ui.aeroLbl->setEnabled(aeroWidgetEnabled);
	m_ui.aeroDescLbl->setEnabled(aeroWidgetEnabled);
	m_ui.ignoreAeroBtn->setEnabled(aeroWidgetEnabled);
	m_ui.disableAeroBtn->setEnabled(aeroWidgetEnabled);

	//-------------------------------------------------------------------------
	// Cropping tab

	// Setup validators
	m_ui.leftCropEdit->setValidator(new QIntValidator(0, INT_MAX, this));
	m_ui.rightCropEdit->setValidator(new QIntValidator(0, INT_MAX, this));
	m_ui.topCropEdit->setValidator(new QIntValidator(0, INT_MAX, this));
	m_ui.botCropEdit->setValidator(new QIntValidator(0, INT_MAX, this));
	connect(m_ui.leftCropEdit, &QLineEdit::textChanged,
		this, &MonitorLayerDialog::leftCropEditChanged);
	connect(m_ui.rightCropEdit, &QLineEdit::textChanged,
		this, &MonitorLayerDialog::rightCropEditChanged);
	connect(m_ui.topCropEdit, &QLineEdit::textChanged,
		this, &MonitorLayerDialog::topCropEditChanged);
	connect(m_ui.botCropEdit, &QLineEdit::textChanged,
		this, &MonitorLayerDialog::botCropEditChanged);

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
		this, &MonitorLayerDialog::gammaSliderChanged);
	connect(m_ui.gammaBox, fpiDSB,
		this, &MonitorLayerDialog::gammaBoxChanged);
	connect(m_ui.brightnessSlider, &QAbstractSlider::valueChanged,
		this, &MonitorLayerDialog::brightnessChanged);
	connect(m_ui.brightnessBox, fpiSB,
		this, &MonitorLayerDialog::brightnessChanged);
	connect(m_ui.contrastSlider, &QAbstractSlider::valueChanged,
		this, &MonitorLayerDialog::contrastChanged);
	connect(m_ui.contrastBox, fpiSB,
		this, &MonitorLayerDialog::contrastChanged);
	connect(m_ui.saturationSlider, &QAbstractSlider::valueChanged,
		this, &MonitorLayerDialog::saturationChanged);
	connect(m_ui.saturationBox, fpiSB,
		this, &MonitorLayerDialog::saturationChanged);

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

MonitorLayerDialog::~MonitorLayerDialog()
{
}

void MonitorLayerDialog::refreshMonitorList()
{
	const MonitorInfoList &monitors =
		App->getCaptureManager()->getMonitorInfo();
	m_ui.monitorCombo->clear();
	for(int i = 0; i < monitors.size(); i++) {
		const MonitorInfo &info = monitors.at(i);
		m_ui.monitorCombo->addItem(tr("[%1] %2")
			.arg(info.friendlyId)
			.arg(info.friendlyName));
		m_ui.monitorCombo->setItemData(
			i, qVariantFromValue<int>(info.friendlyId), Qt::UserRole);
	}
}

void MonitorLayerDialog::loadSettings()
{
	MonitorLayer *layer = static_cast<MonitorLayer *>(m_layer);

	// As we assume the monitor list hasn't changed since it was populated just
	// refresh it again to be safe
	refreshMonitorList();

	// Monitor combo
	int monitor = layer->getMonitor();
	for(int i = 0; i < m_ui.monitorCombo->count(); i++) {
		int friendlyId = m_ui.monitorCombo->itemData(i, Qt::UserRole).toInt();
		if(monitor == friendlyId) {
			m_ui.monitorCombo->setCurrentIndex(i);
			break;
		}
	}

	// Mouse cursor
	m_ui.showCursorBtn->setChecked(layer->getCaptureMouse());
	m_ui.hideCursorBtn->setChecked(!layer->getCaptureMouse());

	// Disable Windows Aero
	if(!IsWindows8OrGreater()) {
		m_ui.ignoreAeroBtn->setChecked(!layer->getDisableAero());
		m_ui.disableAeroBtn->setChecked(layer->getDisableAero());
	}

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

void MonitorLayerDialog::applySettings()
{
	MonitorLayer *layer = static_cast<MonitorLayer *>(m_layer);

	// Monitor combo
	int monitor = m_ui.monitorCombo->itemData(
		m_ui.monitorCombo->currentIndex(), Qt::UserRole).toInt();
	layer->setMonitor(monitor);

	// Mouse cursor
	layer->setCaptureMouse(m_ui.showCursorBtn->isChecked());

	// Disable Windows Aero
	layer->setDisableAero(m_ui.disableAeroBtn->isChecked());

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

void MonitorLayerDialog::leftCropEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.leftCropEdit);
	//emit validityMaybeChanged(isValid());
}

void MonitorLayerDialog::rightCropEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.rightCropEdit);
	//emit validityMaybeChanged(isValid());
}

void MonitorLayerDialog::topCropEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.topCropEdit);
	//emit validityMaybeChanged(isValid());
}

void MonitorLayerDialog::botCropEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.botCropEdit);
	//emit validityMaybeChanged(isValid());
}

void MonitorLayerDialog::gammaSliderChanged(int value)
{
	if(m_ignoreSignals)
		return;
	m_ignoreSignals = true;
	//m_ui.gammaSlider->setValue(value);
	m_ui.gammaBox->setValue(pow(10.0f, (float)value / 500.0f));
	m_ignoreSignals = false;
}

void MonitorLayerDialog::gammaBoxChanged(double value)
{
	if(m_ignoreSignals)
		return;
	m_ignoreSignals = true;
	m_ui.gammaSlider->setValue(qRound(log10((float)value) * 500.0f));
	//m_ui.gammaBox->setValue(value);
	m_ignoreSignals = false;
}

void MonitorLayerDialog::brightnessChanged(int value)
{
	if(m_ignoreSignals)
		return;
	m_ignoreSignals = true;
	m_ui.brightnessSlider->setValue(value);
	m_ui.brightnessBox->setValue(value);
	m_ignoreSignals = false;
}

void MonitorLayerDialog::contrastChanged(int value)
{
	if(m_ignoreSignals)
		return;
	m_ignoreSignals = true;
	m_ui.contrastSlider->setValue(value);
	m_ui.contrastBox->setValue(value);
	m_ignoreSignals = false;
}

void MonitorLayerDialog::saturationChanged(int value)
{
	if(m_ignoreSignals)
		return;
	m_ignoreSignals = true;
	m_ui.saturationSlider->setValue(value);
	m_ui.saturationBox->setValue(value);
	m_ignoreSignals = false;
}
