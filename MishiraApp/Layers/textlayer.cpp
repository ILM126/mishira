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

#include "textlayer.h"
#include "application.h"
#include "layergroup.h"
#include "profile.h"
#include "textlayerdialog.h"
#include <Libvidgfx/graphicscontext.h>
#include <QtGui/QPainter>
#include <QtGui/QTextBlock>
#include <QtGui/QTextLayout>

const QString LOG_CAT = QStringLiteral("Scene");

//=============================================================================
// Helpers

/// <summary>
/// Calculates the distance from the specified pixel to the closest opaque
/// pixel. Takes into account semi-transparent pixels to produce a more
/// accurate result.
/// </summary>
float getDistance(const QImage &img, int x, int y, int maxDist)
{
	// Fast out if the pixel is fully covered
	if(qAlpha(img.pixel(x, y)) == 255)
		return 0.0f;

	int startX = qMax(0, x - maxDist);
	int startY = qMax(0, y - maxDist);
	int endX = qMin(img.width(), x + maxDist + 1); // Just outside
	int endY = qMin(img.height(), y + maxDist + 1); // Just outside
	float dist = (float)maxDist;
	float distSq = dist * dist; // Minimize the amount of sqrt() calls

	// TODO: This loop could be optimized by moving outwards from the center
	for(int j = startY; j < endY; j++) {
		for(int i = startX; i < endX; i++) {
			int alpha = qAlpha(img.pixel(i, j));
			if(alpha == 0)
				continue;
			int dx = qAbs(x - i);
			int dy = qAbs(y - j);
			int dsq = dx * dx + dy * dy;
			if(dsq >= distSq)
				continue;

			// Ignore semi-transparent pixels
			distSq = dsq;
			dist = sqrtf(dsq);
			continue;

			// Take into account semi-transparent pixels
			// TODO
		}
	}

	return dist;
}

//=============================================================================
// TextLayer class

TextLayer::TextLayer(LayerGroup *parent)
	: Layer(parent)
	, m_vertBuf()
	, m_texture(NULL)
	, m_isTexDirty(true)
	, m_document(this)
	, m_strokeSize(0)
	, m_strokeColor(0, 0, 0)
	, m_wordWrap(true)
	, m_dialogBgColor(255, 255, 255)
	, m_scrollSpeed()
{
	//-------------------------------------------------------------------------
	// Setup document rendering settings

	m_document.setDocumentMargin(0.0);

	// As video is rarely played at its exact size we would prefer to not hint
	// our text. Unfortunately when text has an outline it isn't correctly
	// centered when we enable "use design metrics" mode.
	//m_document.setUseDesignMetrics(true);

	// This is duplicated in `setWordWrap()`
	QTextOption options = m_document.defaultTextOption();
	if(m_wordWrap)
		options.setWrapMode(QTextOption::WordWrap);
	else
		options.setWrapMode(QTextOption::NoWrap);
	//options.setUseDesignMetrics(true);
	m_document.setDefaultTextOption(options);

	setDefaultFontSettings(&m_document);

	//-------------------------------------------------------------------------

	// As `render()` can be called multiple times on the same layer (Switching
	// scenes with a shared layer) we must do time-based calculations
	// separately
	connect(App, &Application::queuedFrameEvent,
		this, &TextLayer::queuedFrameEvent);
}

TextLayer::~TextLayer()
{
}

void TextLayer::setDocumentHtml(const QString &html)
{
	if(m_document.toHtml() == html)
		return; // Nothing to do
	//appLog() << html;
	m_document.setHtml(html);
	m_isTexDirty = true;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void TextLayer::setStrokeSize(int size)
{
	if(m_strokeSize == size)
		return; // Nothing to do
	m_strokeSize = size;
	m_isTexDirty = true;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void TextLayer::setStrokeColor(const QColor &color)
{
	if(m_strokeColor == color)
		return; // Nothing to do
	m_strokeColor = color;
	m_isTexDirty = true;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void TextLayer::setWordWrap(bool wordWrap)
{
	if(m_wordWrap == wordWrap)
		return; // Nothing to do
	m_wordWrap = wordWrap;
	m_isTexDirty = true;

	// This is duplicated in the constructor
	QTextOption options = m_document.defaultTextOption();
	if(m_wordWrap)
		options.setWrapMode(QTextOption::WordWrap);
	else
		options.setWrapMode(QTextOption::NoWrap);
	//options.setUseDesignMetrics(true);
	m_document.setDefaultTextOption(options);

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void TextLayer::setScrollSpeed(const QPoint &speed)
{
	if(m_scrollSpeed == speed)
		return; // Nothing to do
	m_scrollSpeed = speed;
	if(m_scrollSpeed.x() == 0 || m_scrollSpeed.y() == 0) {
		// Make changing the speed nicer visually by only resetting it when
		// required.
		m_vertBuf.resetScrolling();
	}

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

QFont TextLayer::getDefaultFont() const
{
#ifdef Q_OS_WIN
	QFont font("Calibri", 32);
	//font.setHintingPreference(QFont::PreferNoHinting);
	//font.setHintingPreference(QFont::PreferFullHinting);
	//font.setStyleStrategy(QFont::PreferAntialias);
#endif
	return font;
}

void TextLayer::setDefaultFontSettings(QTextDocument *document)
{
	document->setDefaultFont(getDefaultFont());
}

void TextLayer::initializeResources(GraphicsContext *gfx)
{
	appLog(LOG_CAT)
		<< "Creating hardware resources for layer " << getIdString();

	m_vertBuf.setContext(gfx);
	m_vertBuf.setRect(QRectF());
	m_vertBuf.setTextureUv(QRectF());
	m_isTexDirty = true;
	updateResources(gfx);
}

void TextLayer::recreateTexture(GraphicsContext *gfx)
{
	if(!m_isTexDirty)
		return; // Don't waste any time if it hasn't changed
	m_isTexDirty = false;

	// Delete existing texture if one exists
	if(m_texture != NULL)
		gfx->deleteTexture(m_texture);
	m_texture = NULL;

	// Determine texture size. We need to keep in mind that the text in the
	// document might extend outside of the layer's bounds.
	m_document.setTextWidth(m_rect.width());
	QSize size(
		(int)ceilf(m_document.size().width()),
		(int)ceilf(m_document.size().height()));

	if(m_document.isEmpty() || size.isEmpty()) {
		// Nothing to display
		return;
	}

	// Create temporary canvas. We need to be careful here as text is rendered
	// differently on premultiplied vs non-premultiplied pixel formats. On a
	// premultiplied format text is rendered with subpixel rendering enabled
	// while on a non-premultiplied format it is not. As we don't want subpixel
	// rendering we use the standard ARGB32 format.
	QSize imgSize(
		size.width() + m_strokeSize * 2, size.height() + m_strokeSize * 2);
	QImage img(imgSize, QImage::Format_ARGB32);
	img.fill(Qt::transparent);
	QPainter p(&img);
	p.setRenderHint(QPainter::Antialiasing, true);

	// Render text
	//m_document.drawContents(&p);

	// Render stroke
	if(m_strokeSize > 0) {
#define STROKE_TECHNIQUE 0
#if STROKE_TECHNIQUE == 0
		// Technique 0: Use QTextDocument's built-in text outliner
		//quint64 timeStart = App->getUsecSinceExec();

		QTextDocument *outlineDoc = m_document.clone(this);

		QTextCharFormat format;
		QPen pen(m_strokeColor, (double)(m_strokeSize * 2));
		pen.setJoinStyle(Qt::RoundJoin);
		format.setTextOutline(pen);
		QTextCursor cursor(outlineDoc);
		cursor.select(QTextCursor::Document);
		cursor.mergeCharFormat(format);

		// Take into account the stroke offset
		p.translate(m_strokeSize, m_strokeSize);

		//quint64 timePath = App->getUsecSinceExec();
		outlineDoc->drawContents(&p);
		delete outlineDoc;

		//quint64 timeEnd = App->getUsecSinceExec();
		//appLog() << "Path time = " << (timePath - timeStart) << " usec";
		//appLog() << "Render time = " << (timeEnd - timePath) << " usec";
		//appLog() << "Full time = " << (timeEnd - timeStart) << " usec";
#elif STROKE_TECHNIQUE == 1
		// Technique 1: Create a text QPainterPath and stroke it
		quint64 timeStart = App->getUsecSinceExec();

		// Create the path for the text's stroke
		QPainterPath path;
		QTextBlock &block = m_document.firstBlock();
		int numBlocks = m_document.blockCount();
		for(int i = 0; i < numBlocks; i++) {
			QTextLayout *layout = block.layout();
			for(int j = 0; j < layout->lineCount(); j++) {
				QTextLine &line = layout->lineAt(j);
				const QString text = block.text().mid(
					line.textStart(), line.textLength());
				QPointF pos = layout->position() + line.position();
				pos.ry() += line.ascent();
				//appLog() << pos << ": " << text;
				path.addText(pos, block.charFormat().font(), text);
			}
			block = block.next();
		}

		quint64 timePath = App->getUsecSinceExec();
		path = path.simplified(); // Fixes gaps with large stroke sizes
		quint64 timeSimplify = App->getUsecSinceExec();

		// Render the path
		//p.strokePath(path, QPen(m_strokeColor, m_strokeSize));

		// Convert it to a stroke
		QPainterPathStroker stroker;
		stroker.setWidth(m_strokeSize);
		//stroker.setCurveThreshold(2.0);
		stroker.setJoinStyle(Qt::RoundJoin);
		path = stroker.createStroke(path);

		// Render the path
		p.fillPath(path, m_strokeColor);

		quint64 timeEnd = App->getUsecSinceExec();
		appLog() << "Path time = " << (timePath - timeStart) << " usec";
		appLog() << "Simplify time = " << (timeSimplify - timePath) << " usec";
		appLog() << "Render time = " << (timeEnd - timeSimplify) << " usec";
		appLog() << "Full time = " << (timeEnd - timeStart) << " usec";
#elif STROKE_TECHNIQUE == 2
		// Technique 2: Similar to technique 1 but do each block separately
		quint64 timeStart = App->getUsecSinceExec();
		quint64 timeTotalSimplify = 0;
		quint64 timeTotalRender = 0;

		// Create the path for the text's stroke
		QTextBlock &block = m_document.firstBlock();
		int numBlocks = m_document.blockCount();
		for(int i = 0; i < numBlocks; i++) {
			// Convert this block to a painter path
			QPainterPath path;
			QTextLayout *layout = block.layout();
			for(int j = 0; j < layout->lineCount(); j++) {
				QTextLine &line = layout->lineAt(j);
				const QString text = block.text().mid(
					line.textStart(), line.textLength());
				QPointF pos = layout->position() + line.position() +
					QPointF(m_strokeSize, m_strokeSize);
				pos.ry() += line.ascent();
				//appLog() << pos << ": " << text;
				path.addText(pos, block.charFormat().font(), text);
			}

			// Prevent gaps appearing at larger stroke sizes
			quint64 timeA = App->getUsecSinceExec();
			path = path.simplified();
			quint64 timeB = App->getUsecSinceExec();
			timeTotalSimplify += timeB - timeA;

			// Render the path
			QPen pen(m_strokeColor, m_strokeSize * 2);
			pen.setJoinStyle(Qt::RoundJoin);
			p.strokePath(path, pen);
			timeA = App->getUsecSinceExec();
			timeTotalRender += timeA - timeB;

			// Iterate
			block = block.next();
		}

		// Make the final draw take into account the stroke offset
		p.translate(m_strokeSize, m_strokeSize);

		quint64 timeEnd = App->getUsecSinceExec();
		appLog() << "Simplify time = " << timeTotalSimplify << " usec";
		appLog() << "Render time = " << timeTotalRender << " usec";
		appLog() << "Full time = " << (timeEnd - timeStart) << " usec";
#elif STROKE_TECHNIQUE == 3
		// Technique 3: Raster brute-force where for each destination pixel
		// we measure the distance to the closest opaque source pixel
		quint64 timeStart = App->getUsecSinceExec();

		// Get bounding region based on text line bounding rects
		QRegion region;
		QTextBlock &block = m_document.firstBlock();
		int numBlocks = m_document.blockCount();
		for(int i = 0; i < numBlocks; i++) {
			QTextLayout *layout = block.layout();
			for(int j = 0; j < layout->lineCount(); j++) {
				QTextLine &line = layout->lineAt(j);
				const QString text = block.text().mid(
					line.textStart(), line.textLength());
				QRect rect = line.naturalTextRect()
					.translated(layout->position()).toAlignedRect();
				if(rect.isEmpty())
					continue; // Don't add empty rectangles
				rect.adjust(0, 0, 1, 0); // QTextLine is incorrect?
				rect.adjust(
					-m_strokeSize, -m_strokeSize,
					m_strokeSize, m_strokeSize);
				//appLog() << rect;
				region += rect;
			}

			// Iterate
			block = block.next();
		}
		quint64 timeRegion = App->getUsecSinceExec();

#if 0
		// Debug bounding region
		QPainterPath regionPath;
		regionPath.addRegion(region);
		regionPath.setFillRule(Qt::WindingFill);
		p.fillPath(regionPath, QColor(255, 0, 0, 128));
#endif // 0

		// We cannot read and write to the same image at the same time so
		// create a second one. Note that this is not premultiplied.
		QImage imgOut(size, QImage::Format_ARGB32);
		imgOut.fill(Qt::transparent);

		// Do distance calculation. We assume that non-fully transparent
		// pixels are always next to a fully opaque one so if the closest
		// "covered" pixel is not fully opaque then we can use that pixel's
		// opacity to determine the distance to the shape's edge.
		for(int y = 0; y < img.height(); y++) {
			for(int x = 0; x < img.width(); x++) {
				if(!region.contains(QPoint(x, y)))
					continue;
				float dist = getDistance(img, x, y, m_strokeSize);

				// We fake antialiasing by blurring the edge by 1px
				float outEdge = (float)m_strokeSize;
				if(dist >= outEdge)
					continue; // Outside stroke completely
				float opacity = qMin(1.0f, outEdge - dist);
				QColor col = m_strokeColor;
				col.setAlphaF(col.alphaF() * opacity);

				// Blend the stroke so that it appears under the existing
				// pixel data
				QRgb origRgb = img.pixel(x, y);
				QColor origCol(origRgb);
				origCol.setAlpha(qAlpha(origRgb));
				col = blendColors(col, origCol, 1.0f);
				imgOut.setPixel(x, y, col.rgba());
			}
		}
		quint64 timeRender = App->getUsecSinceExec();

		// Swap image data
		p.end();
		img = imgOut;
		p.begin(&img);

		quint64 timeEnd = App->getUsecSinceExec();
		appLog() << "Region time = " << (timeRegion - timeStart) << " usec";
		appLog() << "Render time = " << (timeRender - timeRegion) << " usec";
		appLog() << "Swap time = " << (timeEnd - timeRender) << " usec";
		appLog() << "Full time = " << (timeEnd - timeStart) << " usec";
#endif // STROKE_TECHNIQUE
	}

	// Render text
	m_document.drawContents(&p);

	// Convert the image to a GPU texture
	m_texture = gfx->createTexture(img);

	// Preview texture for debugging
	//img.save(App->getDataDirectory().filePath("Preview.png"));
}

void TextLayer::updateResources(GraphicsContext *gfx)
{
	// Completely recreate texture if needed
	recreateTexture(gfx);

	//-------------------------------------------------------------------------
	// Update vertex buffer

	setVisibleRect(QRect()); // Invisible by default
	if(m_texture == NULL)
		return;

	// Never scale our texture and always align it to the left (The user
	// needs to set the alignment in the text document itself)
	LyrAlignment alignment;
	switch(m_alignment) {
	default:
	case LyrTopLeftAlign:
	case LyrTopCenterAlign:
	case LyrTopRightAlign:
		alignment = LyrTopLeftAlign;
		break;
	case LyrMiddleLeftAlign:
	case LyrMiddleCenterAlign:
	case LyrMiddleRightAlign:
		alignment = LyrMiddleLeftAlign;
		break;
	case LyrBottomLeftAlign:
	case LyrBottomCenterAlign:
	case LyrBottomRightAlign:
		alignment = LyrBottomLeftAlign;
		break;
	}
	QSize exStrokeSize = m_texture->getSize();
	exStrokeSize.rwidth() -= m_strokeSize * 2;
	exStrokeSize.rheight() -= m_strokeSize * 2;
	QRectF rect = createScaledRectInBounds(
		exStrokeSize, m_rect, LyrActualScale, alignment);
	rect.adjust(-m_strokeSize, -m_strokeSize, m_strokeSize, m_strokeSize);

	m_vertBuf.setRect(rect);
	m_vertBuf.setTextureUv(QRectF(0.0, 0.0, 1.0, 1.0), GfxUnchangedOrient);
	setVisibleRect(rect.toAlignedRect());
}

void TextLayer::destroyResources(GraphicsContext *gfx)
{
	appLog(LOG_CAT)
		<< "Destroying hardware resources for layer " << getIdString();

	m_vertBuf.deleteVertBuf();
	m_vertBuf.setContext(NULL);
	gfx->deleteTexture(m_texture);
	m_texture = NULL;
}

void TextLayer::render(
	GraphicsContext *gfx, Scene *scene, uint frameNum, int numDropped)
{
	// Has the layer width changed since we rendered the document texture?
	if(m_rect.width() != m_document.textWidth()) {
		// It has, we need to repaint the document texture
		m_isTexDirty = true;
		updateResources(gfx);
	}

	VertexBuffer *vertBuf = m_vertBuf.getVertBuf();
	if(m_texture == NULL || vertBuf == NULL)
		return; // Nothing to render

	gfx->setShader(GfxTexDecalShader);
	gfx->setTopology(m_vertBuf.getTopology());
	gfx->setBlending(GfxAlphaBlending);
	QColor prevCol = gfx->getTexDecalModColor();
	gfx->setTexDecalModColor(
		QColor(255, 255, 255, (int)(getOpacity() * 255.0f)));
	gfx->setTexture(m_texture);
	gfx->setTextureFilter(GfxBilinearFilter);
	gfx->drawBuffer(vertBuf);
	gfx->setTexDecalModColor(prevCol);
}

LyrType TextLayer::getType() const
{
	return LyrTextLayerType;
}

bool TextLayer::hasSettingsDialog()
{
	return true;
}

LayerDialog *TextLayer::createSettingsDialog(QWidget *parent)
{
	return new TextLayerDialog(this, parent);
}

void TextLayer::serialize(QDataStream *stream) const
{
	Layer::serialize(stream);

	// Write data version number
	*stream << (quint32)2;

	// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
	//-------------------------------------------------------------------------
	// If this method is modified then make sure that
	// `ScriptTextLayer::serialize()` has also been updated!
	//-------------------------------------------------------------------------
	// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

	// Save our data
	*stream << m_document.toHtml();
	*stream << m_strokeSize;
	*stream << m_strokeColor;
	*stream << m_wordWrap;
	*stream << m_dialogBgColor;
	*stream << m_scrollSpeed;
}

bool TextLayer::unserialize(QDataStream *stream)
{
	if(!Layer::unserialize(stream))
		return false;

	// Read data version number
	quint32 version;
	*stream >> version;

	// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
	//-------------------------------------------------------------------------
	// If this method is modified then make sure that
	// `ScriptTextLayer::unserialize()` has also been updated!
	//-------------------------------------------------------------------------
	// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

	// Read our data
	if(version >= 0 && version <= 2) {
		bool boolData;
		QString stringData;

		*stream >> stringData;
		//appLog() << stringData;
		m_document.setHtml(stringData);
		*stream >> m_strokeSize;
		*stream >> m_strokeColor;
		if(version >= 1) {
			*stream >> boolData;
			setWordWrap(boolData);
		}
		*stream >> m_dialogBgColor;
		if(version >= 2)
			*stream >> m_scrollSpeed;
		else
			m_scrollSpeed = QPoint(0, 0);
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in text layer serialized data, "
			<< "cannot load settings";
		return false;
	}

	return true;
}

void TextLayer::queuedFrameEvent(uint frameNum, int numDropped)
{
	if(m_scrollSpeed.isNull() || m_texture == NULL ||
		getVisibleRect().isEmpty())
	{
		return; // Nothing to do
	}

	// Calculate amount to scroll in UV coordinates per frame
	Fraction framerate = App->getProfile()->getVideoFramerate();
	QPointF scrollPerFrame(
		(qreal)m_scrollSpeed.x() / framerate.asFloat() /
		(qreal)getVisibleRect().width(),
		(qreal)m_scrollSpeed.y() / framerate.asFloat() /
		(qreal)getVisibleRect().height());

	// Apply scroll
	QPointF scroll = scrollPerFrame * (qreal)(numDropped + 1);
	m_vertBuf.scrollBy(scroll);
}
