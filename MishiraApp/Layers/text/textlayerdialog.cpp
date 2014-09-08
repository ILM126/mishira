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

#include "textlayerdialog.h"
#include "textlayer.h"

//=============================================================================
// Helpers

typedef QString (*ReplaceCallback)(const QString &);
/// <summary>
/// Does a regular expression "find and replace" operation with a callback on
/// the content to be replaced. Patterns must be in the format
/// "(...)(...)(...)" where the center capture point is passed to be the
/// callback and the other two are written back to the output without
/// modification.
/// </summary>
/// <returns> </returns>
QString doRegexReplace(
	const QString &input, const QString &pattern, ReplaceCallback callback)
{
	QString output;
	QRegExp pat(pattern);
	int pos = 0, prevPos = 0;
	while((pos = pat.indexIn(input, pos)) != -1) {
		QString begin = pat.cap(1);
		QString middle = pat.cap(2);
		QString end = pat.cap(3);
		output += input.mid(prevPos, pos - prevPos);
		output += begin;
		output += callback(middle);
		output += end;
		pos += pat.matchedLength();
		prevPos = pos;
	}
	output += input.mid(prevPos, input.size() - prevPos);
	return output;
}

QString doubleIntCallback(const QString &input)
{
	int size = input.toInt();
	return QStringLiteral("%1").arg(size * 2);
}

QString halfIntCallback(const QString &input)
{
	int size = input.toInt();
	return QStringLiteral("%1").arg(size / 2);
}

QString addColorIfMissingCallback(const QString &input)
{
	if(input.contains(QStringLiteral("color:")))
		return input;
	return QStringLiteral("color:#000000; %1").arg(input);
}

//=============================================================================
// TextLayerDialog class

TextLayerDialog::TextLayerDialog(TextLayer *layer, QWidget *parent)
	: LayerDialog(layer, parent)
	, m_ui()
	, m_ignoreSignals(false)
{
	m_ui.setupUi(this);

	// Setup custom widgets
	m_ui.strokeSizeEdit->setUnits(tr("px"));
	m_ui.bgColorBtn->setColors(QColor(255, 255, 255), QColor(0, 0, 0));
	m_ui.xScrollEdit->setUnits(tr("px/sec"));
	m_ui.yScrollEdit->setUnits(tr("px/sec"));

	// Setup document text box
	layer->setDefaultFontSettings(m_ui.documentEdit->document());
	m_ui.documentEdit->setAcceptRichText(false); // Prevent pasting tables
	connect(m_ui.documentEdit, &QTextEdit::currentCharFormatChanged,
		this, &TextLayerDialog::charFormatChanged);

	// Connect WYSIWYG button signals
	void (QFontComboBox:: *fpfCB)(const QFont &) =
		&QFontComboBox::currentFontChanged;
	void (QSpinBox:: *fpiSB)(int) = &QSpinBox::valueChanged;
	void (QAbstractButton:: *fpbAB)(bool) = &QAbstractButton::toggled;
	void (ColorButton:: *fpcCB)(const QColor &) = &ColorButton::colorChanged;
	void (ColorToggleButton:: *fpcCTB)(const QColor &) =
		&ColorToggleButton::currentColorChanged;
	connect(m_ui.fontEdit, fpfCB,
		this, &TextLayerDialog::fontChanged);
	connect(m_ui.fontSizeEdit, fpiSB,
		this, &TextLayerDialog::fontSizeChanged);
	connect(m_ui.boldBtn, fpbAB,
		this, &TextLayerDialog::boldToggled);
	connect(m_ui.italicBtn, fpbAB,
		this, &TextLayerDialog::italicToggled);
	connect(m_ui.underlineBtn, fpbAB,
		this, &TextLayerDialog::underlineToggled);
	connect(m_ui.leftBtn, fpbAB,
		this, &TextLayerDialog::leftAlignToggled);
	connect(m_ui.centerBtn, fpbAB,
		this, &TextLayerDialog::centerAlignToggled);
	connect(m_ui.rightBtn, fpbAB,
		this, &TextLayerDialog::rightAlignToggled);
	connect(m_ui.justifyBtn, fpbAB,
		this, &TextLayerDialog::justifyAlignToggled);
	connect(m_ui.colorBtn, fpcCB,
		this, &TextLayerDialog::fontColorChanged);
	connect(m_ui.bgColorBtn, fpcCTB,
		this, &TextLayerDialog::bgColorChanged);

	// Setup validators
	m_ui.strokeSizeEdit->setValidator(new QIntValidator(0, 20, this));
	m_ui.xScrollEdit->setValidator(new QIntValidator(INT_MIN, INT_MAX, this));
	m_ui.yScrollEdit->setValidator(new QIntValidator(INT_MIN, INT_MAX, this));
	connect(m_ui.strokeSizeEdit, &QLineEdit::textChanged,
		this, &TextLayerDialog::strokeSizeEditChanged);
	connect(m_ui.xScrollEdit, &QLineEdit::textChanged,
		this, &TextLayerDialog::xScrollEditChanged);
	connect(m_ui.yScrollEdit, &QLineEdit::textChanged,
		this, &TextLayerDialog::yScrollEditChanged);

	// Notify the dialog when settings change
	connect(m_ui.documentEdit, &QTextEdit::textChanged,
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

TextLayerDialog::~TextLayerDialog()
{
}

/// <summary>
/// Called when loading the HTML from the layer. We change the font size.
/// </summary>
QString TextLayerDialog::prepareForEdit(const QString &input)
{
	QString output;

	// Half the font sizes
	output = doRegexReplace(
		input, "(<[^>]*font-size:)(\\d+)(pt[^>]*>)", halfIntCallback);

	// Make sure that text has an explicit colour set so that Qt doesn't
	// automatically determine it itself. We do this by adding a colour to the
	// `<body>` tag.
	output = doRegexReplace(
		output, "(<body[^>]*style=\")([^\"]*)(\"[^>]*>)",
		addColorIfMissingCallback);

#if 0
	// Debug conversion
	appLog() << "------------- Input";
	appLog() << input;
	appLog() << "------------- Output";
	appLog() << output;
	return input;
#endif // 0

	return output;
}

/// <summary>
/// Called when saving the HTML to the layer. We change the font size.
/// </summary>
QString TextLayerDialog::prepareForDisplay(const QString &input)
{
	QString output;

	// Double the font sizes
	output = doRegexReplace(
		input, "(<[^>]*font-size:)(\\d+)(pt[^>]*>)", doubleIntCallback);

#if 0
	// Debug conversion
	appLog() << "------------- Input";
	appLog() << input;
	appLog() << "------------- Output";
	appLog() << output;
	return input;
#endif // 0

	return output;
}

/// <summary>
/// Makes sure that the specified cursor is always selecting something,
/// inserting text as a last resort.
/// </summary>
/// <returns>True if the cursor already had a selection</returns>
bool TextLayerDialog::ensureSelectionNotEmpty(QTextCursor &cursor)
{
	if(cursor.position() == cursor.anchor()) {
		cursor.select(QTextCursor::BlockUnderCursor);
		if(cursor.position() == cursor.anchor()) {
			cursor.insertText(tr("Placeholder text"));
			cursor.select(QTextCursor::BlockUnderCursor);
		}
		return false;
	}
	return true;
}

void TextLayerDialog::loadSettings()
{
	TextLayer *layer = static_cast<TextLayer *>(m_layer);
	QString html = layer->getDocumentHtml();
	html = prepareForEdit(html);
	m_ui.documentEdit->document()->setHtml(html);
	m_ui.strokeSizeEdit->setText(QString::number(layer->getStrokeSize()));
	m_ui.strokeColorBtn->setColor(layer->getStrokeColor());
	m_ui.wordWrapBtn->setChecked(layer->getWordWrap());
	m_ui.noWordWrapBtn->setChecked(!layer->getWordWrap());
	m_ui.bgColorBtn->setIsA(
		layer->getDialogBgColor() != m_ui.bgColorBtn->getColorB());
	bgColorChanged(m_ui.bgColorBtn->getCurrentColor());
	m_ui.xScrollEdit->setText(QString::number(layer->getScrollSpeed().x()));
	m_ui.yScrollEdit->setText(QString::number(layer->getScrollSpeed().y()));
}

void TextLayerDialog::applySettings()
{
	TextLayer *layer = static_cast<TextLayer *>(m_layer);
	QString html = m_ui.documentEdit->document()->toHtml();
	html = prepareForDisplay(html);
	layer->setDocumentHtml(html);
	if(m_ui.strokeSizeEdit->hasAcceptableInput())
		layer->setStrokeSize(m_ui.strokeSizeEdit->text().toInt());
	layer->setStrokeColor(m_ui.strokeColorBtn->getColor());
	layer->setWordWrap(m_ui.wordWrapBtn->isChecked());
	layer->setDialogBgColor(m_ui.bgColorBtn->getCurrentColor());

	// Scroll speed
	QPoint scrollSpeed;
	if(m_ui.xScrollEdit->hasAcceptableInput())
		scrollSpeed.setX(m_ui.xScrollEdit->text().toInt());
	if(m_ui.yScrollEdit->hasAcceptableInput())
		scrollSpeed.setY(m_ui.yScrollEdit->text().toInt());
	layer->setScrollSpeed(scrollSpeed);
}

void TextLayerDialog::charFormatChanged(const QTextCharFormat &format)
{
	m_ignoreSignals = true;

	// Update all font-related toolbar buttons
	m_ui.fontEdit->setCurrentFont(format.font());
	m_ui.fontSizeEdit->setValue(format.fontPointSize());
	m_ui.boldBtn->setChecked(format.fontWeight() >= QFont::Bold);
	m_ui.italicBtn->setChecked(format.fontItalic());
	m_ui.underlineBtn->setChecked(format.fontUnderline());
	switch(m_ui.documentEdit->alignment() & Qt::AlignHorizontal_Mask) {
	default:
	case Qt::AlignAbsolute:
	case Qt::AlignLeft:
		m_ui.leftBtn->setChecked(true);
		break;
	case Qt::AlignHCenter:
		m_ui.centerBtn->setChecked(true);
		break;
	case Qt::AlignRight:
		m_ui.rightBtn->setChecked(true);
		break;
	case Qt::AlignJustify:
		m_ui.justifyBtn->setChecked(true);
		break;
	}
	m_ui.colorBtn->setColor(m_ui.documentEdit->textColor());

	m_ignoreSignals = false;
}

void TextLayerDialog::fontChanged(const QFont &font)
{
	if(m_ignoreSignals)
		return;

	QTextCharFormat format;
	QFont newFont = font;
	newFont.setPointSize(m_ui.fontSizeEdit->value());
	format.setFont(newFont);

	QTextCursor cursor = m_ui.documentEdit->textCursor();
	ensureSelectionNotEmpty(cursor);
	cursor.mergeCharFormat(format);

	// Return focus to the text edit widget
	m_ui.documentEdit->setFocus();
}

void TextLayerDialog::fontSizeChanged(int i)
{
	if(m_ignoreSignals)
		return;

	QTextCharFormat format;
	QFont newFont = m_ui.fontEdit->currentFont();
	newFont.setPointSize(i);
	format.setFont(newFont);

	QTextCursor cursor = m_ui.documentEdit->textCursor();
	ensureSelectionNotEmpty(cursor);
	cursor.mergeCharFormat(format);

	// Return focus to the text edit widget
	//m_ui.documentEdit->setFocus(); // TODO: Doesn't work when typing size
}

void TextLayerDialog::boldToggled(bool checked)
{
	if(m_ignoreSignals)
		return;

	QTextCharFormat format;
	format.setFontWeight(checked ? QFont::Bold : QFont::Normal);

	QTextCursor cursor = m_ui.documentEdit->textCursor();
	ensureSelectionNotEmpty(cursor);
	cursor.mergeCharFormat(format);
}

void TextLayerDialog::italicToggled(bool checked)
{
	if(m_ignoreSignals)
		return;

	QTextCharFormat format;
	format.setFontItalic(checked);

	QTextCursor cursor = m_ui.documentEdit->textCursor();
	ensureSelectionNotEmpty(cursor);
	cursor.mergeCharFormat(format);
}

void TextLayerDialog::underlineToggled(bool checked)
{
	if(m_ignoreSignals)
		return;

	QTextCharFormat format;
	format.setFontUnderline(checked);

	QTextCursor cursor = m_ui.documentEdit->textCursor();
	ensureSelectionNotEmpty(cursor);
	cursor.mergeCharFormat(format);
}

void TextLayerDialog::leftAlignToggled(bool checked)
{
	if(m_ignoreSignals)
		return;
	if(!checked)
		return;

	m_ui.documentEdit->setAlignment(Qt::AlignLeft);
}

void TextLayerDialog::centerAlignToggled(bool checked)
{
	if(m_ignoreSignals)
		return;
	if(!checked)
		return;

	m_ui.documentEdit->setAlignment(Qt::AlignHCenter);
}

void TextLayerDialog::rightAlignToggled(bool checked)
{
	if(m_ignoreSignals)
		return;
	if(!checked)
		return;

	m_ui.documentEdit->setAlignment(Qt::AlignRight);
}

void TextLayerDialog::justifyAlignToggled(bool checked)
{
	if(m_ignoreSignals)
		return;
	if(!checked)
		return;

	m_ui.documentEdit->setAlignment(Qt::AlignJustify);
}

void TextLayerDialog::fontColorChanged(const QColor &color)
{
	if(m_ignoreSignals)
		return;

	QTextCursor cursor = m_ui.documentEdit->textCursor();
	if(ensureSelectionNotEmpty(cursor)) {
		// Already had a selection
		m_ui.documentEdit->setTextColor(color);
	} else {
		// Apply the colour to the entire block
		QTextCursor origCursor = m_ui.documentEdit->textCursor();
		m_ui.documentEdit->setTextCursor(cursor);
		m_ui.documentEdit->setTextColor(color);
		m_ui.documentEdit->setTextCursor(origCursor);
	}

	// Return focus to the text edit widget
	m_ui.documentEdit->setFocus();
}

void TextLayerDialog::bgColorChanged(const QColor &color)
{
	// Update background
	QWidget *viewport = m_ui.documentEdit->viewport();
	QPalette pal = viewport->palette();
	pal.setColor(viewport->backgroundRole(), color);
	viewport->setPalette(pal);

	// Update text cursor
	pal = m_ui.documentEdit->palette();
	pal.setColor(QPalette::Text,
		QColor(255 - color.red(), 255 - color.green(), 255 - color.blue()));
	m_ui.documentEdit->setPalette(pal);
}

void TextLayerDialog::strokeSizeEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.strokeSizeEdit);
	//emit validityMaybeChanged(isValid());
}

void TextLayerDialog::xScrollEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.xScrollEdit);
	//emit validityMaybeChanged(isValid());
}

void TextLayerDialog::yScrollEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.yScrollEdit);
	//emit validityMaybeChanged(isValid());
}
