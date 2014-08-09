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

#include "webcamlayerdialog.h"
#include "application.h"
#include "videosource.h"
#include "videosourcemanager.h"
#include "webcamlayer.h"

WebcamLayerDialog::WebcamLayerDialog(WebcamLayer *layer, QWidget *parent)
	: LayerDialog(layer, parent)
	, m_ui()
	, m_curSource(NULL)
	, m_curFramerate(0.0f)
	, m_ignoreSignals(false)
{
	m_ui.setupUi(this);

	// Populate device combo box
	refreshDeviceList();

	// Notify the dialog when settings change
	void (QComboBox:: *fpiCB)(int) = &QComboBox::currentIndexChanged;
	connect(m_ui.deviceCombo, fpiCB,
		this, &LayerDialog::settingModified);
	connect(m_ui.deviceCombo, fpiCB,
		this, &WebcamLayerDialog::deviceComboChanged);
	connect(m_ui.framerateCombo, fpiCB,
		this, &LayerDialog::settingModified);
	connect(m_ui.framerateCombo, fpiCB,
		this, &WebcamLayerDialog::framerateComboChanged);
	connect(m_ui.resolutionCombo, fpiCB,
		this, &LayerDialog::settingModified);

	// Other signals
	connect(m_ui.configBtn, &QAbstractButton::clicked,
		this, &WebcamLayerDialog::configBtnClicked);

	// Watch manager for source changes
	VideoSourceManager *mgr = App->getVideoSourceManager();
	connect(mgr, &VideoSourceManager::sourceAdded,
		this, &WebcamLayerDialog::sourceAdded);
	connect(mgr, &VideoSourceManager::removingSource,
		this, &WebcamLayerDialog::removingSource);
}

WebcamLayerDialog::~WebcamLayerDialog()
{
}

void WebcamLayerDialog::refreshDeviceList()
{
	m_ignoreSignals = true;
	m_ui.deviceCombo->clear();
	const VideoSourceList &srcs =
		App->getVideoSourceManager()->getSourceList();
	for(int i = 0; i < srcs.size(); i++) {
		VideoSource *src = srcs.at(i);
		m_ui.deviceCombo->addItem(src->getFriendlyName());
		m_ui.deviceCombo->setItemData(
			i, qVariantFromValue<quint64>(src->getId()), Qt::UserRole);
	}
	if(srcs.size() <= 0) {
		// Add message saying that no devices are available
		m_ui.deviceCombo->addItem(tr("** No devices found **"));
	}
	m_ignoreSignals = false;

	doEnableDisable();
}

void WebcamLayerDialog::refreshFramerateList()
{
	m_ignoreSignals = true;
	m_ui.framerateCombo->clear();
	if(m_curSource == NULL) {
		m_ignoreSignals = false;
		return;
	}
	bool framerateFound = false;
	QVector<float> framerates = m_curSource->getFramerates();
	for(int i = 0; i < framerates.size(); i++) {
		float framerate = framerates.at(i);
		m_ui.framerateCombo->addItem(tr("%1 Hz").arg(framerate));
		m_ui.framerateCombo->setItemData(
			i, qVariantFromValue<float>(framerate), Qt::UserRole);
		if(framerate == m_curFramerate) {
			m_ui.framerateCombo->setCurrentIndex(i);
			framerateFound = true;
		}
	}
	if(!framerateFound && framerates.size() >= 1) {
		// The specified framerate doesn't exist, use the first one in the list
		m_curFramerate = framerates.at(0);
		m_ui.framerateCombo->setCurrentIndex(0);
	}
	m_ignoreSignals = false;
}

void WebcamLayerDialog::refreshResolutionList()
{
	m_ui.resolutionCombo->clear();
	if(m_curSource == NULL)
		return;
	QSize curSize = m_curSource->getModeSize();
	QVector<QSize> sizes = m_curSource->getSizesForFramerate(m_curFramerate);
	for(int i = 0; i < sizes.size(); i++) {
		const QSize &size = sizes.at(i);
		m_ui.resolutionCombo->addItem(
			tr("%1 x %2").arg(size.width()).arg(size.height()));
		m_ui.resolutionCombo->setItemData(
			i, qVariantFromValue<int>(size.width()), Qt::UserRole + 0);
		m_ui.resolutionCombo->setItemData(
			i, qVariantFromValue<int>(size.height()), Qt::UserRole + 1);
		if(size == curSize)
			m_ui.resolutionCombo->setCurrentIndex(i);
	}
}

/// <summary>
/// Enable or disable all widgets when a device is valid or invalid.
/// </summary>
void WebcamLayerDialog::doEnableDisable()
{
	int numDevices = App->getVideoSourceManager()->getSourceList().size();
	bool enable = (numDevices >= 1);

	m_ui.deviceCombo->setEnabled(enable);
	m_ui.orientationCombo->setEnabled(enable);
	m_ui.framerateCombo->setEnabled(enable);
	m_ui.resolutionCombo->setEnabled(enable);
	m_ui.configBtn->setEnabled(enable);
}

void WebcamLayerDialog::loadSettings()
{
	WebcamLayer *layer = static_cast<WebcamLayer *>(m_layer);

	// As we assume the device list hasn't changed since it was populated just
	// refresh it again to be safe
	refreshDeviceList();

	// TODO: What to do if the device is no longer available?
	quint64 deviceId = layer->getDeviceId();
	const VideoSourceList &srcs =
		App->getVideoSourceManager()->getSourceList();
	for(int i = 0; i < srcs.size(); i++) {
		VideoSource *src = srcs.at(i);
		if(src->getId() == deviceId) {
			m_ui.deviceCombo->setCurrentIndex(i);
			m_curSource = src;
			m_curFramerate = m_curSource->getModeFramerate();

			refreshFramerateList();
			refreshResolutionList();
			break;
		}
	}
	if(srcs.size() > 0 && m_curSource == NULL) {
		// Select the first item by default
		m_ui.deviceCombo->setCurrentIndex(0);
		m_curSource = srcs.at(0);
		m_curFramerate = m_curSource->getModeFramerate();

		refreshFramerateList();
		refreshResolutionList();
	}

	m_ui.orientationCombo->setCurrentIndex(layer->getOrientation());
}

void WebcamLayerDialog::applySettings()
{
	WebcamLayer *layer = static_cast<WebcamLayer *>(m_layer);

	quint64 deviceId = m_ui.deviceCombo->itemData(
		m_ui.deviceCombo->currentIndex(), Qt::UserRole).toULongLong();
	GfxOrientation orientation =
		(GfxOrientation)m_ui.orientationCombo->currentIndex();
	layer->setDeviceId(deviceId);
	layer->setOrientation(orientation);

	// Shared device settings
	if(m_curSource == NULL)
		return; // No device selected
	float framerate = m_ui.framerateCombo->itemData(
		m_ui.framerateCombo->currentIndex(), Qt::UserRole).toFloat();
	int sizeWidth = m_ui.resolutionCombo->itemData(
		m_ui.resolutionCombo->currentIndex(), Qt::UserRole + 0).toInt();
	int sizeHeight = m_ui.resolutionCombo->itemData(
		m_ui.resolutionCombo->currentIndex(), Qt::UserRole + 1).toInt();
	QSize size(sizeWidth, sizeHeight);
	m_curSource->setMode(framerate, size);
}

void WebcamLayerDialog::deviceComboChanged(int index)
{
	if(m_ignoreSignals)
		return;

	// Get new device
	quint64 deviceId =
		m_ui.deviceCombo->itemData(index, Qt::UserRole).toULongLong();
	m_curSource = App->getVideoSourceManager()->getSource(deviceId);
	if(m_curSource == NULL)
		return;
	m_curFramerate = m_curSource->getModeFramerate();

	// Refresh settings that depend on the device
	refreshFramerateList();
	refreshResolutionList();
}

void WebcamLayerDialog::framerateComboChanged(int index)
{
	if(m_ignoreSignals)
		return;

	// Get new framerate
	m_curFramerate =
		m_ui.framerateCombo->itemData(index, Qt::UserRole).toFloat();

	// Refresh settings that depend on the device
	refreshResolutionList();
}

void WebcamLayerDialog::configBtnClicked()
{
	if(m_curSource == NULL)
		return;
	m_curSource->showConfigureDialog(this);
}

void WebcamLayerDialog::sourceAdded(VideoSource *source)
{
	// TODO: This isn't the best way to handle it
	refreshDeviceList();
	if(m_curSource == NULL)
		deviceComboChanged(0);
	refreshFramerateList();
	refreshResolutionList();
}

void WebcamLayerDialog::removingSource(VideoSource *source)
{
	// We cannot refresh the device list properly until after we return
	QTimer::singleShot(0, this, SLOT(removingSourceTimeout()));
}

void WebcamLayerDialog::removingSourceTimeout()
{
	// TODO: This isn't the best way to handle it
	refreshDeviceList();
	deviceComboChanged(0); // Annoying to the user
	refreshFramerateList();
	refreshResolutionList();
}
