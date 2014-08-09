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

#include "d3dwidget.h"
#include "application.h"
#include "graphicswidget.h"
#include "stylehelper.h"
#include <Libvidgfx/d3dcontext.h>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtGui/5.0.2/QtGui/qpa/qplatformnativeinterface.h>

D3DWidget::D3DWidget(GraphicsWidget *parent)
	: QWidget(parent)
	, m_parent(parent)
	, m_timesToPaint(2)
	, m_initialized(false)
	, m_context(NULL)
	, m_curSize(0, 0)
{
	// Make sure Qt doesn't paint over DirectX
	//setAttribute(Qt::WA_PaintOnScreen); // We set this later
	//setAttribute(Qt::WA_NoSystemBackground);

	// Force creation of a native widget for so that `windowHandle()` doesn't
	// return NULL
	setAttribute(Qt::WA_NativeWindow);

	// Make sure that the generic GraphicsWidget receives all mouse events
	setAttribute(Qt::WA_TransparentForMouseEvents);

	// We finish initializing on the first show event. We do this to keep
	// things consistent as an OpenGL context cannot be initialized until a
	// window has actually been created (At least that's the case on Windows,
	// I honestly don't know if this is the case with DirectX on Windows or
	// OpenGL on OS X)
}

D3DWidget::~D3DWidget()
{
	if(m_context != NULL) {
		App->setGraphicsContext(NULL);
		delete m_context;
		m_context = NULL;
	}
}

/// <summary>
/// Prevent Qt from rendering over DirectX
/// </summary>
QPaintEngine *D3DWidget::paintEngine() const
{
	if(m_timesToPaint > 0)
		return QWidget::paintEngine();
	return NULL;
}

void D3DWidget::showEvent(QShowEvent *ev)
{
	// Finish initializing on the first show event only
	if(m_initialized)
		return;
	m_initialized = true;

	// Get the HWND of this widget
	QPlatformNativeInterface *native =
		QGuiApplication::platformNativeInterface();
	HWND hwnd = static_cast<HWND>(
		native->nativeResourceForWindow("handle", windowHandle()));
	if(hwnd == NULL) {
		appLog(Log::Critical)
			<< "Failed to get HWND for the graphics widget, cannot continue";
		App->exit(1);
		return;
	}

	// Create graphics context
	m_context = new D3DContext();
	App->setGraphicsContext(m_context);

	// Make sure our parent is watching for the context signals
	connect(m_context, &D3DContext::initialized,
		m_parent, &GraphicsWidget::graphicsContextInitialized);
	connect(m_context, &D3DContext::destroying,
		m_parent, &GraphicsWidget::graphicsContextDestroyed);

	// Finish initializing the graphics context
	if(!m_context->initialize(hwnd, size(), StyleHelper::DarkBg1Color)) {
		// The context has already logged the error, no need to do it again
		App->exit(1);
		return;
	}
}

void D3DWidget::resizeEvent(QResizeEvent *ev)
{
	if(m_context == NULL || !m_context->isValid())
		return; // Nothing to do

	// Prevent this event from being called multiple times in a row for the
	// same event
	if(m_curSize == size())
		return;
	m_curSize = size();

	// Forward to the generic graphics widget
	m_parent->screenResized(m_context, ev->oldSize(), size());
}

void D3DWidget::paintEvent(QPaintEvent *ev)
{
	// Force a repaint ASAP instead of waiting for the next frame to begin
	m_parent->setDirty(true);
	//m_parent->repaintScreen();

	// When capturing a window some drivers don't display embedded 3D contexts
	// and instead display the pixel data behind them. As a user can capture
	// the Mishira window itself we need to make it look pretty.
	if(m_timesToPaint <= 0)
		return;

	// Paint the background. TODO: We should also paint a fake canvas
	QPainter p(this);
	p.fillRect(QRect(0, 0, width(), height()), StyleHelper::DarkBg1Color);
	m_timesToPaint--;

	// Make sure Qt doesn't paint over DirectX once we have finished
	if(m_timesToPaint <= 0)
		setAttribute(Qt::WA_PaintOnScreen);
}
