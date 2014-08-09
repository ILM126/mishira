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

#include "darkstyle.h"
#include "log.h"
#include "stylehelper.h"
#include <QtGui/QPainter>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QStyleOptionFrameV3>

const bool ENABLE_STYLE_DEBUG = true;
const QString LOG_CAT = QStringLiteral("Style");

//=============================================================================
// Helpers

void translatePointList(QVector<QPoint> &points, const QPoint &offset)
{
	for(int i = 0; i < points.count(); i++)
		points[i] += offset;
}

//=============================================================================
// DarkStyle

DarkStyle::DarkStyle()
	: QCommonStyle()
	, m_nextCheckStateIsTri()
{
	m_nextCheckStateIsTri.reserve(32);
}

DarkStyle::~DarkStyle()
{
}

void DarkStyle::addNextCheckStateIsTri(QCheckBox *w)
{
	if(m_nextCheckStateIsTri.contains(w))
		return;
	m_nextCheckStateIsTri.append(w);
}

void DarkStyle::removeNextCheckStateIsTri(QCheckBox *w)
{
	int index = m_nextCheckStateIsTri.indexOf(w);
	if(index < 0)
		return;
	m_nextCheckStateIsTri.remove(index);
}

//=============================================================================
// QStyle methods

void DarkStyle::polish(QWidget *widget)
{
	QCommonStyle::polish(widget);

	// We want to repaint some widgets when the user hovers over them with the
	// mouse cursor
	if(qobject_cast<QAbstractButton *>(widget) ||
		qobject_cast<QComboBox *>(widget) ||
		qobject_cast<QScrollBar *>(widget))
	{
		widget->setAttribute(Qt::WA_Hover);
	}

	// We want the outline of combo box popups to be a custom colour instead of
	// the foreground colour. We can't just reimplement `PE_Frame` as the popup
	// is a separate widget and it defaults to the original QStyle and not this
	// one.
	QComboBox *combo = qobject_cast<QComboBox *>(widget);
	if(combo != NULL) {
		QPalette pal = widget->palette();
		pal.setColor(widget->foregroundRole(), StyleHelper::DarkBg4Color);
		widget->setPalette(pal);
	}
}

void DarkStyle::unpolish(QWidget *widget)
{
	QCommonStyle::unpolish(widget);

	// We want to repaint some widgets when the user hovers over them with the
	// mouse cursor
	if(qobject_cast<QAbstractButton *>(widget) ||
		qobject_cast<QComboBox *>(widget) ||
		qobject_cast<QScrollBar *>(widget))
	{
		widget->setAttribute(Qt::WA_Hover, false);
	}

	// TODO: Undo combo box popup outline colour change
}

QRect DarkStyle::itemTextRect(
	const QFontMetrics &fm, const QRect &r, int flags, bool enabled,
	const QString &text) const
{
	return QCommonStyle::itemTextRect(fm, r, flags, enabled, text);
}

void DarkStyle::drawItemText(
	QPainter *painter, const QRect &rect, int flags, const QPalette &pal,
	bool enabled, const QString &text, QPalette::ColorRole textRole) const
{
	QCommonStyle::drawItemText(
		painter, rect, flags, pal, enabled, text, textRole);
}

void DarkStyle::drawPrimitive(
	PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
	const QWidget *w) const
{
	switch(pe) {
	case PE_Widget:
		break; // Invisible primitive
	case PE_PanelLineEdit:
		if(const QStyleOptionFrame *panel =
			qstyleoption_cast<const QStyleOptionFrame *>(opt))
		{
			// Draw line edits darker when they have keyboard focus
			QPalette pal = opt->palette;
			if(w->hasFocus()) {
				pal.setColor(QPalette::Active, QPalette::Base,
					pal.base().color().darker(LINE_EDIT_FOCUS_DARKEN_AMOUNT));
			}
			if(panel->lineWidth > 0)
				StyleHelper::drawDarkPanel(p, pal, opt->rect);
			else
				p->fillRect(opt->rect, pal.color(QPalette::Base));
		}
		break;
	case PE_IndicatorCheckBox:
	case PE_IndicatorRadioButton: {
		bool isRadio = (pe == PE_IndicatorRadioButton);

		// Draw the background
		if(opt->state & State_Sunken) {
			// Pressed
			QPalette pal = opt->palette;
			pal.setColor(QPalette::Base,
				pal.base().color().darker(CHECKBOX_PRESSED_DARKEN_AMOUNT));
			StyleHelper::drawDarkPanel(p, pal, opt->rect, isRadio);
		} else if(opt->state & State_MouseOver) {
			// Hovered
			QPalette pal = opt->palette;
			pal.setColor(QPalette::Base,
				pal.base().color().darker(CHECKBOX_HOVER_DARKEN_AMOUNT));
			StyleHelper::drawDarkPanel(p, pal, opt->rect, isRadio);
		} else {
			// Normal
			StyleHelper::drawDarkPanel(p, opt->palette, opt->rect, isRadio);
		}

		// Draw the check mark. HACK: Displaying a check mark when in the
		// sunken state causes visual issues for tri-state check boxes. We want
		// to show the next state on mouse down but we don't actually know what
		// the next state will be. We cheat by manually forwarding the required
		// information to this style instance with `m_nextCheckStateIsTri`.
		if(opt->state & (State_Sunken | State_On | State_NoChange)) {
			// Indicator is checked
			p->save();
			p->setRenderHint(QPainter::Antialiasing, true);

			int dx = opt->rect.x();
			int dy = opt->rect.y();

			bool showDot = (isRadio || opt->state & State_NoChange);
			if(opt->state & State_Sunken) {
				if(m_nextCheckStateIsTri.contains(w))
					showDot = true;
			}
			if(showDot) {
				// Create brush
				QBrush brush(StyleHelper::DarkFg1Color);
				if(opt->state & State_Sunken) // Darken on press
					brush.setColor(StyleHelper::DarkFg3Color);

				// Render the check mark
				p->setPen(Qt::NoPen);
				p->setBrush(brush);
				if(isRadio) {
					// Radio button check mark
					p->drawEllipse(4 + dx, 4 + dy, 5, 5);
				} else {
					// Tri-state check mark
					p->drawEllipse(5 + dx, 5 + dy, 3, 3);
				}
			} else {
				// Create pen
				QPen pen(StyleHelper::DarkFg1Color, 1.75);
				pen.setJoinStyle(Qt::MiterJoin); // Sharp corner
				if(opt->state & State_Sunken) // Darken on press
					pen.setColor(StyleHelper::DarkFg3Color);

				// Render the check mark
				p->setPen(pen);
				p->setBrush(Qt::NoBrush);
				QPolygonF poly;
				poly << QPointF(3.5, 7.5);
				poly << QPointF(5.25, 9.5);
				poly << QPointF(9.5, 3.5);
				poly.translate((double)dx, (double)dy);
				p->drawPolyline(poly);
			}

			p->restore();
		}
		break; }
	case PE_IndicatorArrowDown: {
		// Be consistent with the text colour
		QColor fg = opt->palette.color(QPalette::ButtonText);
		QColor shadow = StyleHelper::TextShadowColor;

		QPoint cen = opt->rect.center() + QPoint(1, 1);
		QVector<QPoint> points;
		points << QPoint(cen.x() - 3, cen.y() - 1);
		points << QPoint(cen.x() + 3, cen.y() - 1);
		points << QPoint(cen.x() - 2, cen.y() + 0);
		points << QPoint(cen.x() + 2, cen.y() + 0);
		points << QPoint(cen.x() - 1, cen.y() + 1);
		points << QPoint(cen.x() + 1, cen.y() + 1);
		points << QPoint(cen.x() - 0, cen.y() + 2);
		points << QPoint(cen.x() - 0, cen.y() + 2);
		p->setPen(shadow);
		p->drawLines(points);
		translatePointList(points, QPoint(-1, -1));
		p->setPen(fg);
		p->drawLines(points);
		break; }
	case PE_FrameFocusRect:
		// Based on QWindowStyle's focus rectangle
		if(const QStyleOptionFocusRect *fropt =
			qstyleoption_cast<const QStyleOptionFocusRect *>(opt))
		{
			// Only display the focus rectangle when the alt key has been
			// pressed
			if(!(fropt->state & State_KeyboardFocusChange) &&
				!styleHint(SH_UnderlineShortcut, opt))
				return;

			// Push painter state
			p->save();

			// Setup the brush with a XOR pattern
			p->setBackgroundMode(Qt::TransparentMode);
			QRect r = opt->rect;
			QColor bgCol = fropt->backgroundColor;
			if(!bgCol.isValid())
				bgCol = p->background().color();
			QColor patternCol((bgCol.red() ^ 0xff) & 0xff,
				(bgCol.green() ^ 0xff) & 0xff,
				(bgCol.blue() ^ 0xff) & 0xff);
			p->setBrush(QBrush(patternCol, Qt::Dense4Pattern));
			p->setBrushOrigin(r.topLeft());
			p->setPen(Qt::NoPen);

			// Draw the actual rectangle
			p->drawRect(r.left(), r.top(), r.width(), 1); // Top
			p->drawRect(r.left(), r.bottom(), r.width(), 1); // Bottom
			p->drawRect(r.left(), r.top(), 1, r.height()); // Left
			p->drawRect(r.right(), r.top(), 1, r.height()); // Right

			// Pop painter state
			p->restore();
		}
		break;
	default:
		if(ENABLE_STYLE_DEBUG) {
			appLog(LOG_CAT, Log::Warning)
				<< "Unhandled drawPrimitive() "
				<< primitiveElementToString(pe);
		}
		QCommonStyle::drawPrimitive(pe, opt, p, w);
		break;
	}
}

void DarkStyle::drawControl(
	ControlElement element, const QStyleOption *opt, QPainter *p,
	const QWidget *w) const
{
	switch(element) {
	case CE_PushButton:
		// QCommonStyle doesn't properly set the label colour
		if(const QStyleOptionButton *btn =
			qstyleoption_cast<const QStyleOptionButton *>(opt))
		{
			// Draw the background
			drawControl(CE_PushButtonBevel, btn, p, w);

			// Draw the label
			QStyleOptionButton subopt = *btn;
			p->setPen(opt->palette.color(QPalette::ButtonText));
			subopt.rect = subElementRect(SE_PushButtonContents, btn, w);
			drawControl(CE_PushButtonLabel, &subopt, p, w);

			// Draw the focus rectangle
			if(btn->state & State_HasFocus) {
				QStyleOptionFocusRect fropt;
				fropt.QStyleOption::operator=(*btn);
				fropt.rect = subElementRect(SE_PushButtonFocusRect, btn, w);
				drawPrimitive(PE_FrameFocusRect, &fropt, p, w);
			}
		}
		break;
	case CE_PushButtonBevel:
		if(const QStyleOptionButton *btn =
			qstyleoption_cast<const QStyleOptionButton *>(opt))
		{
			QPoint offset(0, 0);
			//if(!(opt->state & State_Enabled)) {
			//	// Disabled
			//} else
			if(opt->state & State_Sunken) {
				// Pressed
				StyleHelper::drawDarkButton(p, opt->palette, opt->rect, true);
				offset = QPoint(1, 1);
			} else if(opt->state & State_MouseOver) {
				// Hovered
				QPalette pal = opt->palette;
				pal.setColor(QPalette::Button,
					pal.button().color().darker(BUTTON_HOVER_DARKEN_AMOUNT));
				StyleHelper::drawDarkButton(p, pal, opt->rect, false);
			} else {
				// Normal
				StyleHelper::drawDarkButton(p, opt->palette, opt->rect, false);
			}

			if(btn->features & QStyleOptionButton::HasMenu) {
				int size = pixelMetric(PM_MenuButtonIndicator, btn, w);
				QRect r = btn->rect;
				QStyleOptionButton newBtn = *btn;
				newBtn.rect = QRect(
					r.right() - size - 4 + offset.x(),
					r.height() / 2 - size / 2 + offset.y(),
					size, size);
				drawPrimitive(PE_IndicatorArrowDown, &newBtn, p, w);
			}
		}
		break;
	case CE_PushButtonLabel:
		if(const QStyleOptionButton *btn =
			qstyleoption_cast<const QStyleOptionButton *>(opt))
		{
			// The code below is based off QCommonStyle (CE_PushButtonLabel)
			// Differences include:
			//     - Added text shadow with hard-coded colour
			//     - Improved text/icon positioning
			// This code is mostly duplicated in `StyledButton::paintEvent()`

			QRect textRect = btn->rect;
			uint tf = Qt::AlignVCenter | Qt::TextShowMnemonic;
			if(!styleHint(QStyle::SH_UnderlineShortcut, btn, w))
				tf |= Qt::TextHideMnemonic;

			// HACK: Move the text up 1px so that it is vertically centered
			textRect.translate(0, -1);

			// Draw the icon if one exists. We need to modify the text
			// rectangle as well as we want both the icon and the text centered
			// on the button
			QRect iconRect;
			if(!btn->icon.isNull()) {
				// Determine icon state
				QIcon::Mode mode = (btn->state & QStyle::State_Enabled) ?
					QIcon::Normal : QIcon::Disabled;
				if(mode == QIcon::Normal &&
					btn->state & QStyle::State_HasFocus)
				{
					mode = QIcon::Active;
				}
				QIcon::State state = QIcon::Off;
				if(btn->state & QStyle::State_On)
					state = QIcon::On;

				// Determine metrics
				QPixmap pixmap = btn->icon.pixmap(btn->iconSize, mode, state);
				int labelWidth = pixmap.width();
				int labelHeight = pixmap.height();
				int iconSpacing = 4; // See `sizeHint()`
				int textWidth = btn->fontMetrics.boundingRect(
					btn->rect, tf, btn->text).width();
				if(!btn->text.isEmpty())
					labelWidth += textWidth + iconSpacing;

				// Determine icon rectangle
				iconRect = QRect(
					textRect.x() + (textRect.width() - labelWidth) / 2,
					textRect.y() + (textRect.height() - labelHeight) / 2,
					pixmap.width(), pixmap.height());
				iconRect =
					QStyle::visualRect(btn->direction, textRect, iconRect);

				// Change where the text will be displayed. We force a left
				// align as we adjust the text rectangle instead
				tf |= Qt::AlignLeft;
				if(btn->direction == Qt::RightToLeft)
					textRect.setRight(iconRect.left() - iconSpacing);
				else {
					textRect.setLeft(
						iconRect.left() + iconRect.width() + iconSpacing);
				}

				// Translate the contents slightly when the button is pressed
				if(btn->state & QStyle::State_Sunken) {
					iconRect.translate(
						pixelMetric(PM_ButtonShiftHorizontal, opt, w),
						pixelMetric(PM_ButtonShiftHorizontal, opt, w));
				}

				// Draw pixmap
				p->drawPixmap(iconRect, pixmap);
			} else
				tf |= Qt::AlignHCenter;

			// Translate the contents slightly when the button is pressed
			if(btn->state & QStyle::State_Sunken) {
				textRect.translate(
					pixelMetric(PM_ButtonShiftHorizontal, opt, w),
					pixelMetric(PM_ButtonShiftHorizontal, opt, w));
			}

			// Draw text shadow only if the button is enabled
			if(btn->state & QStyle::State_Enabled) {
				QPalette pal(btn->palette);
				pal.setColor(
					QPalette::ButtonText, StyleHelper::TextShadowColor);
				drawItemText(
					p, textRect.translated(2, 1).adjusted(-1, -1, 1, 1), tf,
					pal, true, btn->text, QPalette::ButtonText);
			}

			// Draw text. HACK: We offset the text by one pixel to the right as
			// the font metrics include the character spacing to the right of
			// the last character making the text appear slightly to the left.
			// We also increase the rectangle by 1px in every direction in
			// order for the text to not get clipped with some fonts.
			drawItemText(
				p, textRect.translated(1, 0).adjusted(-1, -1, 1, 1), tf,
				btn->palette, (btn->state & QStyle::State_Enabled), btn->text,
				QPalette::ButtonText);
		}
		break;
	case CE_ComboBoxLabel:
		if(const QStyleOptionComboBox *cb =
			qstyleoption_cast<const QStyleOptionComboBox *>(opt))
		{
			QRect editRect =
				subControlRect(CC_ComboBox, cb, SC_ComboBoxEditField, w);
			if(!cb->currentIcon.isNull()) {
				// TODO
			}
			if(!cb->currentText.isEmpty() && !cb->editable) {
				uint tf = visualAlignment(
					cb->direction, Qt::AlignLeft | Qt::AlignVCenter);

				// HACK: Move the text up 1px so that it is vertically centered
				editRect.translate(0, -1);

				// Draw text shadow only if the combo box is enabled
				if(cb->state & QStyle::State_Enabled) {
					QPalette pal(cb->palette);
					pal.setColor(
						QPalette::ButtonText, StyleHelper::TextShadowColor);
					drawItemText(
						p, editRect.translated(2, 1).adjusted(-1, -1, 1, 1),
						tf, pal, true, cb->currentText, QPalette::ButtonText);
				}

				// Draw text. HACK: We offset the text by one pixel to the
				// right as the font metrics include the character spacing to
				// the right of the last character making the text appear
				// slightly to the left. We also increase the rectangle by 1px
				// in every direction in order for the text to not get clipped
				// with some fonts.
				drawItemText(
					p, editRect.translated(1, 0).adjusted(-1, -1, 1, 1), tf,
					cb->palette, (cb->state & State_Enabled), cb->currentText,
					QPalette::ButtonText);
			}
		}
		break;
	case CE_ScrollBarSubPage:
	case CE_ScrollBarAddPage:
		break; // Invisible
	case CE_ScrollBarSlider:
		if(const QStyleOptionSlider *sb =
			qstyleoption_cast<const QStyleOptionSlider *>(opt))
		{
			// Is the scroll bar the full length of the groove?
			bool isFull = false;
			if(sb->orientation == Qt::Horizontal)
				isFull = sb->rect.width() == w->width();
			else
				isFull = sb->rect.height() == w->height();

			// Determine slider colour
			QColor col = StyleHelper::DarkBg6Color;
			if(!isFull) {
				if(opt->state & State_Sunken) {
					// Pressed
					col = col.darker(SCROLLBAR_PRESSED_DARKEN_AMOUNT);
				} else if(opt->state & State_MouseOver) {
					// Hovered
					col = col.darker(SCROLLBAR_HOVER_DARKEN_AMOUNT);
				}
			}

			// Create painter path
			QPainterPath path;
			if(sb->orientation == Qt::Horizontal) {
				// 1px extra padding on the top for the border
				QRect rect = sb->rect.adjusted(2, 3, -2, -2);
				path = StyleHelper::createRoundedRect(
					rect, (float)rect.height() * 0.5f);
			} else {
				// 1px extra padding on the left for the border
				QRect rect = sb->rect.adjusted(3, 2, -2, -2);
				path = StyleHelper::createRoundedRect(
					rect, (float)rect.width() * 0.5f);
			}

			// Render the slider
			p->save();
			p->setRenderHint(QPainter::Antialiasing, true);
			p->setPen(Qt::NoPen);
			p->setBrush(col);
			p->drawPath(path);
			p->restore();
		}
		break;
	case CE_RadioButton:
	case CE_CheckBox:
	case CE_CheckBoxLabel:
	case CE_RadioButtonLabel:
	case CE_ShapedFrame:
		// Use default
		QCommonStyle::drawControl(element, opt, p, w);
		break;
	default:
		if(ENABLE_STYLE_DEBUG) {
			appLog(LOG_CAT, Log::Warning)
				<< "Unhandled drawControl() "
				<< controlElementToString(element);
		}
		QCommonStyle::drawControl(element, opt, p, w);
		break;
	}
}

QRect DarkStyle::subElementRect(
	SubElement subElement, const QStyleOption *option,
	const QWidget *widget) const
{
	QRect rect;

	// Sub-elements that ignore QCommonStyle
#if 0
	switch(subElement) {
	case SE_PushButtonFocusRect:
		// We want the focus rectangle to go around the contents of the button
		// as closely as possible. QCommonStyle's algorithm doesn't correctly
		// handle all possible inputs so we use our own algorithm instead. This
		// code is based on our StyledButton class.
		if(const QStyleOptionButton *btn =
			qstyleoption_cast<const QStyleOptionButton *>(option))
		{
			int pad =
				pixelMetric(QStyle::PM_DefaultFrameWidth, option, widget) + 1;

			// Get contents rectangle
			QMargins margins = widget->contentsMargins();
			QRect textRect = btn->rect.adjusted(
				margins.left(), margins.top(),
				-margins.right(), -margins.bottom());
			if(btn->state & QStyle::State_Sunken)
				textRect.translate(1, 1);

			// Does the button have an icon, text or both?
			if(!btn->icon.isNull()) {
				QRegion region(btn->fontMetrics.boundingRect(
					textRect, tf, btn->text));
				region += iconRect;
				rect = region.boundingRect();
				rect = rect.adjusted(-pad, -pad, pad, pad);
			} else if(!btn->text.isEmpty()) {
				rect = btn->fontMetrics.boundingRect(
					textRect, tf, btn->text);
				rect = rect.adjusted(-pad, -pad, pad, pad);
			} else {
				rect = btn->rect;

				// Translate the contents slightly when the button is pressed
				if(btn->state & QStyle::State_Sunken)
					rect.translate(1, 1);
			}
			return rect;
		}
		break;
	default:
		break;
	}
#endif // 0

	// Sub-elements that modify QCommonStyle
	rect = QCommonStyle::subElementRect(subElement, option, widget);
	switch(subElement) {
	case SE_LineEditContents:
		// Extra horizontal padding
		rect.adjust(1, 0, -1, 0);
		break;
	case SE_CheckBoxFocusRect:
	case SE_RadioButtonFocusRect:
		rect.adjust(1, 1, -1, -1);
		break;
	case SE_ComboBoxLayoutItem:
	case SE_PushButtonContents:
	case SE_PushButtonFocusRect:
	case SE_PushButtonLayoutItem:
	case SE_CheckBoxIndicator:
	case SE_CheckBoxContents:
	case SE_CheckBoxClickRect:
	case SE_CheckBoxLayoutItem:
	case SE_RadioButtonIndicator:
	case SE_RadioButtonContents:
	case SE_RadioButtonClickRect:
	case SE_RadioButtonLayoutItem:
	case SE_FrameLayoutItem:
	case SE_FrameContents:
	case SE_ShapedFrameContents:
	case SE_LabelLayoutItem:
		// Use default value
		break;
	default:
		if(ENABLE_STYLE_DEBUG) {
			appLog(LOG_CAT, Log::Warning)
				<< "Unhandled subElementRect() "
				<< subElementToString(subElement);
		}
		break;
	}

	return rect;
}

void DarkStyle::drawComplexControl(
	ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
	const QWidget *widget) const
{
	SubControls sub = opt->subControls;
	switch(cc) {
	case CC_ComboBox:
		if(const QStyleOptionComboBox *cmb =
			qstyleoption_cast<const QStyleOptionComboBox *>(opt))
		{
			if(sub & SC_ComboBoxFrame) {
				QStyleOptionButton btn;
				btn.QStyleOption::operator=(*opt);
				if(sub & SC_ComboBoxArrow)
					btn.features = QStyleOptionButton::HasMenu;
				drawControl(QStyle::CE_PushButton, &btn, p, widget);
			}
		}
		break;
	case CC_ScrollBar:
		// We need to render the groove here as QCommonStyle doesn't do it for
		// us at all
		if(const QStyleOptionSlider *sb =
			qstyleoption_cast<const QStyleOptionSlider *>(opt))
		{
			p->save();
			p->setRenderHint(QPainter::Antialiasing, false);

			// Draw background
			if(sb->orientation == Qt::Horizontal) {
				p->fillRect(
					opt->rect.adjusted(0, 1, 0, 0), StyleHelper::DarkBg2Color);
			} else {
				p->fillRect(
					opt->rect.adjusted(1, 0, 0, 0), StyleHelper::DarkBg2Color);
			}

			// Draw left/top border
			p->setPen(StyleHelper::DarkBg1Color);
			p->setBrush(Qt::NoBrush);
			if(sb->orientation == Qt::Horizontal)
				p->drawLine(opt->rect.topLeft(), opt->rect.topRight());
			else
				p->drawLine(opt->rect.topLeft(), opt->rect.bottomLeft());

			p->restore();
		}
		QCommonStyle::drawComplexControl(cc, opt, p, widget);
		break;
	default:
		if(ENABLE_STYLE_DEBUG)
			appLog(LOG_CAT, Log::Warning) << "Unhandled drawComplexControl()";
		QCommonStyle::drawComplexControl(cc, opt, p, widget);
		break;
	}
}

QStyle::SubControl DarkStyle::hitTestComplexControl(
	ComplexControl cc, const QStyleOptionComplex *opt, const QPoint &pt,
	const QWidget *widget) const
{
	switch(cc) {
	case CC_ComboBox:
	case CC_ScrollBar:
		// Default uses `subControlRect()`
		break;
	default:
		if(ENABLE_STYLE_DEBUG) {
			appLog(LOG_CAT, Log::Warning)
				<< "Unhandled hitTestComplexControl()";
		}
		break;
	}
	return QCommonStyle::hitTestComplexControl(cc, opt, pt, widget);
}

QRect DarkStyle::subControlRect(
	ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc,
	const QWidget *widget) const
{
	if(cc == CC_ComboBox) {
		// Add extra padding to the combo box label
		QRect rect = QCommonStyle::subControlRect(cc, opt, sc, widget);
		switch(sc) {
		case SC_ComboBoxEditField:
			rect.adjust(1, 0, 0, 0);
			break;
		case SC_ComboBoxFrame:
		case SC_ComboBoxArrow:
		case SC_ComboBoxListBoxPopup:
			break;
		default:
			if(ENABLE_STYLE_DEBUG) {
				appLog(LOG_CAT, Log::Warning)
					<< "Unhandled subControlRect(CC_ComboBox)";
			}
			break;
		}
		return rect;
	} else if(cc == CC_ScrollBar) {
		QRect rect;
		if(const QStyleOptionSlider *scrollbar =
			qstyleoption_cast<const QStyleOptionSlider *>(opt))
		{
			rect = scrollbar->rect;

			// The size of the left/right buttons. We assume that these are
			// always square.
			const QSize lSize(0, 0);
			const QSize rSize(0, 0);

			// The amount of padding between the slider and the sides of the
			// scroll bar
			const int SLIDER_PADDING = 0;

			//-----------------------------------------------------------------
			// Calculate slider metrics. This is based off code in
			// QCommonStyle::subControlRect()

			// Maximum available space
			int sliderMaxLen =
				(scrollbar->orientation == Qt::Horizontal
				? rect.width() : rect.height())
				- lSize.width() - rSize.width();

			// Actual slider length
			int sliderLen;
			if(scrollbar->maximum != scrollbar->minimum) {
				uint range = scrollbar->maximum - scrollbar->minimum;
				sliderLen = (qint64(scrollbar->pageStep) * sliderMaxLen) /
					(range + scrollbar->pageStep);

				int sliderMin = proxy()->pixelMetric(
					PM_ScrollBarSliderMin, scrollbar, widget);
				if (sliderLen < sliderMin || range > INT_MAX / 2)
					sliderLen = sliderMin;
				if (sliderLen > sliderMaxLen)
					sliderLen = sliderMaxLen;
			} else
				sliderLen = sliderMaxLen;

			int sliderStart = lSize.width() + sliderPositionFromValue(
				scrollbar->minimum, scrollbar->maximum,
				scrollbar->sliderPosition, sliderMaxLen - sliderLen,
				scrollbar->upsideDown);

			//-----------------------------------------------------------------

			switch(sc) {
			case SC_ScrollBarSubLine: // Top/left button
				if(scrollbar->orientation == Qt::Horizontal) {
					rect.setRect(
						0, 0,
						lSize.width(), lSize.height());
				} else {
					rect.setRect(
						0, 0,
						lSize.width(), lSize.height());
				}
				break;
			case SC_ScrollBarAddLine: // Bottom/right button
				if(scrollbar->orientation == Qt::Horizontal)
					rect.setRect(
					rect.right() + 1 - rSize.width(), rect.top(),
					rSize.width(), rSize.height());
				else {
					rect.setRect(
						rect.left(), rect.bottom() + 1 - rSize.width(),
						rSize.width(), rSize.height());
				}
				break;
			case SC_ScrollBarSubPage: // Between top/left button and slider
				if(scrollbar->orientation == Qt::Horizontal) {
					rect.setRect(
						lSize.width(), 0,
						sliderStart - lSize.width(),
						rect.height());
				} else {
					rect.setRect(
						0, lSize.height(),
						rect.width(),
						sliderStart - lSize.height());
				}
				break;
			case SC_ScrollBarAddPage: // Between bottom/right button and slider
				if(scrollbar->orientation == Qt::Horizontal) {
					rect.setRect(
						sliderStart + sliderLen, 0,
						rect.width() + 1 - (sliderStart + sliderLen)
						- lSize.width(),
						rect.height());
				} else {
					rect.setRect(
						0, sliderStart + sliderLen,
						rect.width(),
						rect.height() + 1 - (sliderStart + sliderLen)
						- lSize.height());
				}
				break;
			case SC_ScrollBarFirst:
			case SC_ScrollBarLast:
				rect = QRect(); // Invisible sub-controls
				break;
			case SC_ScrollBarSlider: {
				if(scrollbar->orientation == Qt::Horizontal) {
					rect.setRect(
						sliderStart, SLIDER_PADDING,
						sliderLen, rect.height() - SLIDER_PADDING * 2);
				} else {
					rect.setRect(
						SLIDER_PADDING, sliderStart,
						rect.width() - SLIDER_PADDING * 2, sliderLen);
				}
				break; }
			case SC_ScrollBarGroove:
				if(scrollbar->orientation == Qt::Horizontal) {
					rect.setRect(
						lSize.width(), 0,
						rect.width() + 1 - lSize.width() - rSize.width(),
						rect.height());
				} else {
					rect.setRect(
						0, lSize.width(),
						rect.height(),
						rect.height() + 1 - lSize.width() - rSize.width());
				}
				break;
			default:
				rect = QCommonStyle::subControlRect(cc, opt, sc, widget);
				break;
			}
		}
		return rect;
	}

#if 0
	switch(cc) {
	default:
#endif // 0
		if(ENABLE_STYLE_DEBUG)
			appLog(LOG_CAT, Log::Warning) << "Unhandled subControlRect()";
#if 0
		break;
	}
#endif // 0

	return QCommonStyle::subControlRect(cc, opt, sc, widget);
}

int DarkStyle::pixelMetric(
	PixelMetric metric, const QStyleOption *option,
	const QWidget *widget) const
{
	switch(metric) {
	case PM_ButtonShiftHorizontal:
	case PM_ButtonShiftVertical:
		return 1;
	case PM_MenuButtonIndicator:
		return 12;
	case PM_IndicatorWidth: // Check box
	case PM_IndicatorHeight:
	case PM_ExclusiveIndicatorWidth: // Radio button
	case PM_ExclusiveIndicatorHeight:
		return 13;
	case PM_ScrollBarExtent:
		return 12; // Controls the width of the scroll bar
	case PM_ScrollBarSliderMin:
		return 0;
	case PM_ButtonMargin:
	case PM_ButtonIconSize:
	case PM_CheckBoxLabelSpacing:
	case PM_RadioButtonLabelSpacing:
	case PM_SmallIconSize:
	case PM_ComboBoxFrameWidth:
	case PM_FocusFrameHMargin:
	case PM_ScrollView_ScrollBarOverlap:
	case PM_DefaultFrameWidth:
	case PM_TextCursorWidth:
	case PM_MaximumDragDistance:
		// Use default value
		return QCommonStyle::pixelMetric(metric, option, widget);
	default:
		if(ENABLE_STYLE_DEBUG) {
			appLog(LOG_CAT, Log::Warning)
				<< "Unhandled pixelMetric() "
				<< pixelMetricToString(metric);
		}
		return QCommonStyle::pixelMetric(metric, option, widget);
	}
}

QSize DarkStyle::sizeFromContents(
	ContentsType ct, const QStyleOption *opt, const QSize &contentsSize,
	const QWidget *w) const
{
	// Let QCommonStyle simplify our calculations
	QSize size = QCommonStyle::sizeFromContents(ct, opt, contentsSize, w);

	switch(ct) {
	case CT_PushButton:
	case CT_ComboBox:
	case CT_LineEdit:
	case CT_CheckBox:
	case CT_RadioButton:
	case CT_ScrollBar:
		// Use default value. TODO: These are untested
		break;
	default:
		if(ENABLE_STYLE_DEBUG) {
			appLog(LOG_CAT, Log::Warning)
				<< "Unhandled sizeFromContents() "
				<< contentsTypeToString(ct);
		}
		break;
	}

	return size;
}

int DarkStyle::styleHint(
	StyleHint stylehint, const QStyleOption *opt, const QWidget *widget,
	QStyleHintReturn *returnData) const
{
	switch(stylehint) {
	case SH_UnderlineShortcut:
		return false;
	case SH_Menu_FlashTriggeredItem:
		return false;
	case SH_Menu_FadeOutOnHide:
		return false;
	case SH_ComboBox_PopupFrameStyle:
		// Use a box frame so that the popup uses the palette that we set in
		// `polish()`
		return QFrame::Box | QFrame::Plain;
	case SH_DitherDisabledText:
		return false;
	case SH_EtchDisabledText:
		return true;
	case SH_ComboBox_Popup:
	case SH_ComboBox_ListMouseTracking:
	case SH_ComboBox_LayoutDirection:
	case SH_Widget_ShareActivation:
	case SH_BlinkCursorWhenTextSelected:
	case SH_LineEdit_PasswordCharacter:
	case SH_ScrollBar_Transient:
	case SH_ScrollBar_MiddleClickAbsolutePosition:
	case SH_ScrollBar_RollBetweenButtons:
	case SH_ScrollBar_ScrollWhenPointerLeavesControl:
	case SH_ScrollBar_LeftClickAbsolutePosition:
	case SH_ScrollBar_ContextMenu:
	case SH_ScrollView_FrameOnlyAroundContents:
	case SH_RequestSoftwareInputPanel:
		// Use default value
		return QCommonStyle::styleHint(stylehint, opt, widget, returnData);
	default:
		if(ENABLE_STYLE_DEBUG) {
			appLog(LOG_CAT, Log::Warning)
				<< "Unhandled styleHint() "
				<< styleHintToString(stylehint);
		}
		return QCommonStyle::styleHint(stylehint, opt, widget, returnData);
	}
}

//=============================================================================
// Debug methods

QString DarkStyle::primitiveElementToString(PrimitiveElement pe) const
{
	switch(pe) {
	case PE_Frame: return "PE_Frame";
	case PE_FrameDefaultButton: return "PE_FrameDefaultButton";
	case PE_FrameDockWidget: return "PE_FrameDockWidget";
	case PE_FrameFocusRect: return "PE_FrameFocusRect";
	case PE_FrameGroupBox: return "PE_FrameGroupBox";
	case PE_FrameLineEdit: return "PE_FrameLineEdit";
	case PE_FrameMenu: return "PE_FrameMenu";
	case PE_FrameStatusBar: return "PE_FrameStatusBar";
	case PE_FrameTabWidget: return "PE_FrameTabWidget";
	case PE_FrameWindow: return "PE_FrameWindow";
	case PE_FrameButtonBevel: return "PE_FrameButtonBevel";
	case PE_FrameButtonTool: return "PE_FrameButtonTool";
	case PE_FrameTabBarBase: return "PE_FrameTabBarBase";
	case PE_PanelButtonCommand: return "PE_PanelButtonCommand";
	case PE_PanelButtonBevel: return "PE_PanelButtonBevel";
	case PE_PanelButtonTool: return "PE_PanelButtonTool";
	case PE_PanelMenuBar: return "PE_PanelMenuBar";
	case PE_PanelToolBar: return "PE_PanelToolBar";
	case PE_PanelLineEdit: return "PE_PanelLineEdit";
	case PE_IndicatorArrowDown: return "PE_IndicatorArrowDown";
	case PE_IndicatorArrowLeft: return "PE_IndicatorArrowLeft";
	case PE_IndicatorArrowRight: return "PE_IndicatorArrowRight";
	case PE_IndicatorArrowUp: return "PE_IndicatorArrowUp";
	case PE_IndicatorBranch: return "PE_IndicatorBranch";
	case PE_IndicatorButtonDropDown: return "PE_IndicatorButtonDropDown";
	case PE_IndicatorViewItemCheck: return "PE_IndicatorViewItemCheck";
	case PE_IndicatorCheckBox: return "PE_IndicatorCheckBox";
	case PE_IndicatorDockWidgetResizeHandle: return "PE_IndicatorDockWidgetResizeHandle";
	case PE_IndicatorHeaderArrow: return "PE_IndicatorHeaderArrow";
	case PE_IndicatorMenuCheckMark: return "PE_IndicatorMenuCheckMark";
	case PE_IndicatorProgressChunk: return "PE_IndicatorProgressChunk";
	case PE_IndicatorRadioButton: return "PE_IndicatorRadioButton";
	case PE_IndicatorSpinDown: return "PE_IndicatorSpinDown";
	case PE_IndicatorSpinMinus: return "PE_IndicatorSpinMinus";
	case PE_IndicatorSpinPlus: return "PE_IndicatorSpinPlus";
	case PE_IndicatorSpinUp: return "PE_IndicatorSpinUp";
	case PE_IndicatorToolBarHandle: return "PE_IndicatorToolBarHandle";
	case PE_IndicatorToolBarSeparator: return "PE_IndicatorToolBarSeparator";
	case PE_PanelTipLabel: return "PE_PanelTipLabel";
	case PE_IndicatorTabTear: return "PE_IndicatorTabTear";
	case PE_PanelScrollAreaCorner: return "PE_PanelScrollAreaCorner";
	case PE_Widget: return "PE_Widget";
	case PE_IndicatorColumnViewArrow: return "PE_IndicatorColumnViewArrow";
	case PE_IndicatorItemViewItemDrop: return "PE_IndicatorItemViewItemDrop";
	case PE_PanelItemViewItem: return "PE_PanelItemViewItem";
	case PE_PanelItemViewRow: return "PE_PanelItemViewRow";
	case PE_PanelStatusBar: return "PE_PanelStatusBar";
	case PE_IndicatorTabClose: return "PE_IndicatorTabClose";
	case PE_PanelMenu: return "PE_PanelMenu";
	default:
	case PE_CustomBase:
		if(pe >= PE_CustomBase)
			return "**Above PE_CustomBase**";
		return "**Unknown PE**";
	}
}

QString DarkStyle::controlElementToString(ControlElement element) const
{
	switch(element) {
	case CE_PushButton: return "CE_PushButton";
	case CE_PushButtonBevel: return "CE_PushButtonBevel";
	case CE_PushButtonLabel: return "CE_PushButtonLabel";
	case CE_CheckBox: return "CE_CheckBox";
	case CE_CheckBoxLabel: return "CE_CheckBoxLabel";
	case CE_RadioButton: return "CE_RadioButton";
	case CE_RadioButtonLabel: return "CE_RadioButtonLabel";
	case CE_TabBarTab: return "CE_TabBarTab";
	case CE_TabBarTabShape: return "CE_TabBarTabShape";
	case CE_TabBarTabLabel: return "CE_TabBarTabLabel";
	case CE_ProgressBar: return "CE_ProgressBar";
	case CE_ProgressBarGroove: return "CE_ProgressBarGroove";
	case CE_ProgressBarContents: return "CE_ProgressBarContents";
	case CE_ProgressBarLabel: return "CE_ProgressBarLabel";
	case CE_MenuItem: return "CE_MenuItem";
	case CE_MenuScroller: return "CE_MenuScroller";
	case CE_MenuVMargin: return "CE_MenuVMargin";
	case CE_MenuHMargin: return "CE_MenuHMargin";
	case CE_MenuTearoff: return "CE_MenuTearoff";
	case CE_MenuEmptyArea: return "CE_MenuEmptyArea";
	case CE_MenuBarItem: return "CE_MenuBarItem";
	case CE_MenuBarEmptyArea: return "CE_MenuBarEmptyArea";
	case CE_ToolButtonLabel: return "CE_ToolButtonLabel";
	case CE_Header: return "CE_Header";
	case CE_HeaderSection: return "CE_HeaderSection";
	case CE_HeaderLabel: return "CE_HeaderLabel";
	case CE_ToolBoxTab: return "CE_ToolBoxTab";
	case CE_SizeGrip: return "CE_SizeGrip";
	case CE_Splitter: return "CE_Splitter";
	case CE_RubberBand: return "CE_RubberBand";
	case CE_DockWidgetTitle: return "CE_DockWidgetTitle";
	case CE_ScrollBarAddLine: return "CE_ScrollBarAddLine";
	case CE_ScrollBarSubLine: return "CE_ScrollBarSubLine";
	case CE_ScrollBarAddPage: return "CE_ScrollBarAddPage";
	case CE_ScrollBarSubPage: return "CE_ScrollBarSubPage";
	case CE_ScrollBarSlider: return "CE_ScrollBarSlider";
	case CE_ScrollBarFirst: return "CE_ScrollBarFirst";
	case CE_ScrollBarLast: return "CE_ScrollBarLast";
	case CE_FocusFrame: return "CE_FocusFrame";
	case CE_ComboBoxLabel: return "CE_ComboBoxLabel";
	case CE_ToolBar: return "CE_ToolBar";
	case CE_ToolBoxTabShape: return "CE_ToolBoxTabShape";
	case CE_ToolBoxTabLabel: return "CE_ToolBoxTabLabel";
	case CE_HeaderEmptyArea: return "CE_HeaderEmptyArea";
	case CE_ColumnViewGrip: return "CE_ColumnViewGrip";
	case CE_ItemViewItem: return "CE_ItemViewItem";
	case CE_ShapedFrame: return "CE_ShapedFrame";
	default:
	case CE_CustomBase:
		if(element >= CE_CustomBase)
			return "**Above CE_CustomBase**";
		return "**Unknown CE**";
	}
}

QString DarkStyle::subElementToString(SubElement subElement) const
{
	switch(subElement) {
	case SE_PushButtonContents: return "SE_PushButtonContents";
	case SE_PushButtonFocusRect: return "SE_PushButtonFocusRect";
	case SE_CheckBoxIndicator: return "SE_CheckBoxIndicator";
	case SE_CheckBoxContents: return "SE_CheckBoxContents";
	case SE_CheckBoxFocusRect: return "SE_CheckBoxFocusRect";
	case SE_CheckBoxClickRect: return "SE_CheckBoxClickRect";
	case SE_RadioButtonIndicator: return "SE_RadioButtonIndicator";
	case SE_RadioButtonContents: return "SE_RadioButtonContents";
	case SE_RadioButtonFocusRect: return "SE_RadioButtonFocusRect";
	case SE_RadioButtonClickRect: return "SE_RadioButtonClickRect";
	case SE_ComboBoxFocusRect: return "SE_ComboBoxFocusRect";
	case SE_SliderFocusRect: return "SE_SliderFocusRect";
	case SE_ProgressBarGroove: return "SE_ProgressBarGroove";
	case SE_ProgressBarContents: return "SE_ProgressBarContents";
	case SE_ProgressBarLabel: return "SE_ProgressBarLabel";
	case SE_ToolBoxTabContents: return "SE_ToolBoxTabContents";
	case SE_HeaderLabel: return "SE_HeaderLabel";
	case SE_HeaderArrow: return "SE_HeaderArrow";
	case SE_TabWidgetTabBar: return "SE_TabWidgetTabBar";
	case SE_TabWidgetTabPane: return "SE_TabWidgetTabPane";
	case SE_TabWidgetTabContents: return "SE_TabWidgetTabContents";
	case SE_TabWidgetLeftCorner: return "SE_TabWidgetLeftCorner";
	case SE_TabWidgetRightCorner: return "SE_TabWidgetRightCorner";
	case SE_ViewItemCheckIndicator: return "SE_ViewItemCheckIndicator";
	case SE_TabBarTearIndicator: return "SE_TabBarTearIndicator";
	case SE_TreeViewDisclosureItem: return "SE_TreeViewDisclosureItem";
	case SE_LineEditContents: return "SE_LineEditContents";
	case SE_FrameContents: return "SE_FrameContents";
	case SE_DockWidgetCloseButton: return "SE_DockWidgetCloseButton";
	case SE_DockWidgetFloatButton: return "SE_DockWidgetFloatButton";
	case SE_DockWidgetTitleBarText: return "SE_DockWidgetTitleBarText";
	case SE_DockWidgetIcon: return "SE_DockWidgetIcon";
	case SE_CheckBoxLayoutItem: return "SE_CheckBoxLayoutItem";
	case SE_ComboBoxLayoutItem: return "SE_ComboBoxLayoutItem";
	case SE_DateTimeEditLayoutItem: return "SE_DateTimeEditLayoutItem";
	case SE_DialogButtonBoxLayoutItem: return "SE_DialogButtonBoxLayoutItem";
	case SE_LabelLayoutItem: return "SE_LabelLayoutItem";
	case SE_ProgressBarLayoutItem: return "SE_ProgressBarLayoutItem";
	case SE_PushButtonLayoutItem: return "SE_PushButtonLayoutItem";
	case SE_RadioButtonLayoutItem: return "SE_RadioButtonLayoutItem";
	case SE_SliderLayoutItem: return "SE_SliderLayoutItem";
	case SE_SpinBoxLayoutItem: return "SE_SpinBoxLayoutItem";
	case SE_ToolButtonLayoutItem: return "SE_ToolButtonLayoutItem";
	case SE_FrameLayoutItem: return "SE_FrameLayoutItem";
	case SE_GroupBoxLayoutItem: return "SE_GroupBoxLayoutItem";
	case SE_TabWidgetLayoutItem: return "SE_TabWidgetLayoutItem";
	case SE_ItemViewItemDecoration: return "SE_ItemViewItemDecoration";
	case SE_ItemViewItemText: return "SE_ItemViewItemText";
	case SE_ItemViewItemFocusRect: return "SE_ItemViewItemFocusRect";
	case SE_TabBarTabLeftButton: return "SE_TabBarTabLeftButton";
	case SE_TabBarTabRightButton: return "SE_TabBarTabRightButton";
	case SE_TabBarTabText: return "SE_TabBarTabText";
	case SE_ShapedFrameContents: return "SE_ShapedFrameContents";
	case SE_ToolBarHandle: return "SE_ToolBarHandle";
	default:
	case SE_CustomBase:
		if(subElement >= SE_CustomBase)
			return "**Above SE_CustomBase**";
		return "**Unknown SE**";
	}
}

QString DarkStyle::contentsTypeToString(ContentsType ct) const
{
	switch(ct) {
	case CT_PushButton: return "CT_PushButton";
	case CT_CheckBox: return "CT_CheckBox";
	case CT_RadioButton: return "CT_RadioButton";
	case CT_ToolButton: return "CT_ToolButton";
	case CT_ComboBox: return "CT_ComboBox";
	case CT_Splitter: return "CT_Splitter";
	case CT_ProgressBar: return "CT_ProgressBar";
	case CT_MenuItem: return "CT_MenuItem";
	case CT_MenuBarItem: return "CT_MenuBarItem";
	case CT_MenuBar: return "CT_MenuBar";
	case CT_Menu: return "CT_Menu";
	case CT_TabBarTab: return "CT_TabBarTab";
	case CT_Slider: return "CT_Slider";
	case CT_ScrollBar: return "CT_ScrollBar";
	case CT_LineEdit: return "CT_LineEdit";
	case CT_SpinBox: return "CT_SpinBox";
	case CT_SizeGrip: return "CT_SizeGrip";
	case CT_TabWidget: return "CT_TabWidget";
	case CT_DialogButtons: return "CT_DialogButtons";
	case CT_HeaderSection: return "CT_HeaderSection";
	case CT_GroupBox: return "CT_GroupBox";
	case CT_MdiControls: return "CT_MdiControls";
	case CT_ItemViewItem: return "CT_ItemViewItem";
	default:
	case CT_CustomBase:
		if(ct >= CT_CustomBase)
			return "**Above CT_CustomBase**";
		return "**Unknown CT**";
	}
}

QString DarkStyle::pixelMetricToString(PixelMetric metric) const
{
	switch(metric) {
	case PM_ButtonMargin: return "PM_ButtonMargin";
	case PM_ButtonDefaultIndicator: return "PM_ButtonDefaultIndicator";
	case PM_MenuButtonIndicator: return "PM_MenuButtonIndicator";
	case PM_ButtonShiftHorizontal: return "PM_ButtonShiftHorizontal";
	case PM_ButtonShiftVertical: return "PM_ButtonShiftVertical";
	case PM_DefaultFrameWidth: return "PM_DefaultFrameWidth";
	case PM_SpinBoxFrameWidth: return "PM_SpinBoxFrameWidth";
	case PM_ComboBoxFrameWidth: return "PM_ComboBoxFrameWidth";
	case PM_MaximumDragDistance: return "PM_MaximumDragDistance";
	case PM_ScrollBarExtent: return "PM_ScrollBarExtent";
	case PM_ScrollBarSliderMin: return "PM_ScrollBarSliderMin";
	case PM_SliderThickness: return "PM_SliderThickness";
	case PM_SliderControlThickness: return "PM_SliderControlThickness";
	case PM_SliderLength: return "PM_SliderLength";
	case PM_SliderTickmarkOffset: return "PM_SliderTickmarkOffset";
	case PM_SliderSpaceAvailable: return "PM_SliderSpaceAvailable";
	case PM_DockWidgetSeparatorExtent: return "PM_DockWidgetSeparatorExtent";
	case PM_DockWidgetHandleExtent: return "PM_DockWidgetHandleExtent";
	case PM_DockWidgetFrameWidth: return "PM_DockWidgetFrameWidth";
	case PM_TabBarTabOverlap: return "PM_TabBarTabOverlap";
	case PM_TabBarTabHSpace: return "PM_TabBarTabHSpace";
	case PM_TabBarTabVSpace: return "PM_TabBarTabVSpace";
	case PM_TabBarBaseHeight: return "PM_TabBarBaseHeight";
	case PM_TabBarBaseOverlap: return "PM_TabBarBaseOverlap";
	case PM_ProgressBarChunkWidth: return "PM_ProgressBarChunkWidth";
	case PM_SplitterWidth: return "PM_SplitterWidth";
	case PM_TitleBarHeight: return "PM_TitleBarHeight";
	case PM_MenuScrollerHeight: return "PM_MenuScrollerHeight";
	case PM_MenuHMargin: return "PM_MenuHMargin";
	case PM_MenuVMargin: return "PM_MenuVMargin";
	case PM_MenuPanelWidth: return "PM_MenuPanelWidth";
	case PM_MenuTearoffHeight: return "PM_MenuTearoffHeight";
	case PM_MenuDesktopFrameWidth: return "PM_MenuDesktopFrameWidth";
	case PM_MenuBarPanelWidth: return "PM_MenuBarPanelWidth";
	case PM_MenuBarItemSpacing: return "PM_MenuBarItemSpacing";
	case PM_MenuBarVMargin: return "PM_MenuBarVMargin";
	case PM_MenuBarHMargin: return "PM_MenuBarHMargin";
	case PM_IndicatorWidth: return "PM_IndicatorWidth";
	case PM_IndicatorHeight: return "PM_IndicatorHeight";
	case PM_ExclusiveIndicatorWidth: return "PM_ExclusiveIndicatorWidth";
	case PM_ExclusiveIndicatorHeight: return "PM_ExclusiveIndicatorHeight";
	case PM_DialogButtonsSeparator: return "PM_DialogButtonsSeparator";
	case PM_DialogButtonsButtonWidth: return "PM_DialogButtonsButtonWidth";
	case PM_DialogButtonsButtonHeight: return "PM_DialogButtonsButtonHeight";
	case PM_MdiSubWindowFrameWidth: return "PM_MdiSubWindowFrameWidth";
	case PM_MdiSubWindowMinimizedWidth: return "PM_MdiSubWindowMinimizedWidth";
	case PM_HeaderMargin: return "PM_HeaderMargin";
	case PM_HeaderMarkSize: return "PM_HeaderMarkSize";
	case PM_HeaderGripMargin: return "PM_HeaderGripMargin";
	case PM_TabBarTabShiftHorizontal: return "PM_TabBarTabShiftHorizontal";
	case PM_TabBarTabShiftVertical: return "PM_TabBarTabShiftVertical";
	case PM_TabBarScrollButtonWidth: return "PM_TabBarScrollButtonWidth";
	case PM_ToolBarFrameWidth: return "PM_ToolBarFrameWidth";
	case PM_ToolBarHandleExtent: return "PM_ToolBarHandleExtent";
	case PM_ToolBarItemSpacing: return "PM_ToolBarItemSpacing";
	case PM_ToolBarItemMargin: return "PM_ToolBarItemMargin";
	case PM_ToolBarSeparatorExtent: return "PM_ToolBarSeparatorExtent";
	case PM_ToolBarExtensionExtent: return "PM_ToolBarExtensionExtent";
	case PM_SpinBoxSliderHeight: return "PM_SpinBoxSliderHeight";
	case PM_DefaultTopLevelMargin: return "PM_DefaultTopLevelMargin";
	case PM_DefaultChildMargin: return "PM_DefaultChildMargin";
	case PM_DefaultLayoutSpacing: return "PM_DefaultLayoutSpacing";
	case PM_ToolBarIconSize: return "PM_ToolBarIconSize";
	case PM_ListViewIconSize: return "PM_ListViewIconSize";
	case PM_IconViewIconSize: return "PM_IconViewIconSize";
	case PM_SmallIconSize: return "PM_SmallIconSize";
	case PM_LargeIconSize: return "PM_LargeIconSize";
	case PM_FocusFrameVMargin: return "PM_FocusFrameVMargin";
	case PM_FocusFrameHMargin: return "PM_FocusFrameHMargin";
	case PM_ToolTipLabelFrameWidth: return "PM_ToolTipLabelFrameWidth";
	case PM_CheckBoxLabelSpacing: return "PM_CheckBoxLabelSpacing";
	case PM_TabBarIconSize: return "PM_TabBarIconSize";
	case PM_SizeGripSize: return "PM_SizeGripSize";
	case PM_DockWidgetTitleMargin: return "PM_DockWidgetTitleMargin";
	case PM_MessageBoxIconSize: return "PM_MessageBoxIconSize";
	case PM_ButtonIconSize: return "PM_ButtonIconSize";
	case PM_DockWidgetTitleBarButtonMargin: return "PM_DockWidgetTitleBarButtonMargin";
	case PM_RadioButtonLabelSpacing: return "PM_RadioButtonLabelSpacing";
	case PM_LayoutLeftMargin: return "PM_LayoutLeftMargin";
	case PM_LayoutTopMargin: return "PM_LayoutTopMargin";
	case PM_LayoutRightMargin: return "PM_LayoutRightMargin";
	case PM_LayoutBottomMargin: return "PM_LayoutBottomMargin";
	case PM_LayoutHorizontalSpacing: return "PM_LayoutHorizontalSpacing";
	case PM_LayoutVerticalSpacing: return "PM_LayoutVerticalSpacing";
	case PM_TabBar_ScrollButtonOverlap: return "PM_TabBar_ScrollButtonOverlap";
	case PM_TextCursorWidth: return "PM_TextCursorWidth";
	case PM_TabCloseIndicatorWidth: return "PM_TabCloseIndicatorWidth";
	case PM_TabCloseIndicatorHeight: return "PM_TabCloseIndicatorHeight";
	case PM_ScrollView_ScrollBarSpacing: return "PM_ScrollView_ScrollBarSpacing";
	case PM_ScrollView_ScrollBarOverlap: return "PM_ScrollView_ScrollBarOverlap";
	case PM_SubMenuOverlap: return "PM_SubMenuOverlap";
	default:
	case PM_CustomBase:
		if(metric >= PM_CustomBase)
			return "**Above PM_CustomBase**";
		return "**Unknown PM**";
	}
}

QString DarkStyle::styleHintToString(StyleHint stylehint) const
{
	switch(stylehint) {
	case SH_EtchDisabledText: return "SH_EtchDisabledText";
	case SH_DitherDisabledText: return "SH_DitherDisabledText";
	case SH_ScrollBar_MiddleClickAbsolutePosition: return "SH_ScrollBar_MiddleClickAbsolutePosition";
	case SH_ScrollBar_ScrollWhenPointerLeavesControl: return "SH_ScrollBar_ScrollWhenPointerLeavesControl";
	case SH_TabBar_SelectMouseType: return "SH_TabBar_SelectMouseType";
	case SH_TabBar_Alignment: return "SH_TabBar_Alignment";
	case SH_Header_ArrowAlignment: return "SH_Header_ArrowAlignment";
	case SH_Slider_SnapToValue: return "SH_Slider_SnapToValue";
	case SH_Slider_SloppyKeyEvents: return "SH_Slider_SloppyKeyEvents";
	case SH_ProgressDialog_CenterCancelButton: return "SH_ProgressDialog_CenterCancelButton";
	case SH_ProgressDialog_TextLabelAlignment: return "SH_ProgressDialog_TextLabelAlignment";
	case SH_PrintDialog_RightAlignButtons: return "SH_PrintDialog_RightAlignButtons";
	case SH_MainWindow_SpaceBelowMenuBar: return "SH_MainWindow_SpaceBelowMenuBar";
	case SH_FontDialog_SelectAssociatedText: return "SH_FontDialog_SelectAssociatedText";
	case SH_Menu_AllowActiveAndDisabled: return "SH_Menu_AllowActiveAndDisabled";
	case SH_Menu_SpaceActivatesItem: return "SH_Menu_SpaceActivatesItem";
	case SH_Menu_SubMenuPopupDelay: return "SH_Menu_SubMenuPopupDelay";
	case SH_ScrollView_FrameOnlyAroundContents: return "SH_ScrollView_FrameOnlyAroundContents";
	case SH_MenuBar_AltKeyNavigation: return "SH_MenuBar_AltKeyNavigation";
	case SH_ComboBox_ListMouseTracking: return "SH_ComboBox_ListMouseTracking";
	case SH_Menu_MouseTracking: return "SH_Menu_MouseTracking";
	case SH_MenuBar_MouseTracking: return "SH_MenuBar_MouseTracking";
	case SH_ItemView_ChangeHighlightOnFocus: return "SH_ItemView_ChangeHighlightOnFocus";
	case SH_Widget_ShareActivation: return "SH_Widget_ShareActivation";
	case SH_Workspace_FillSpaceOnMaximize: return "SH_Workspace_FillSpaceOnMaximize";
	case SH_ComboBox_Popup: return "SH_ComboBox_Popup";
	case SH_TitleBar_NoBorder: return "SH_TitleBar_NoBorder";
	case SH_Slider_StopMouseOverSlider: return "SH_Slider_StopMouseOverSlider";
	case SH_BlinkCursorWhenTextSelected: return "SH_BlinkCursorWhenTextSelected";
	case SH_RichText_FullWidthSelection: return "SH_RichText_FullWidthSelection";
	case SH_Menu_Scrollable: return "SH_Menu_Scrollable";
	case SH_GroupBox_TextLabelVerticalAlignment: return "SH_GroupBox_TextLabelVerticalAlignment";
	case SH_GroupBox_TextLabelColor: return "SH_GroupBox_TextLabelColor";
	case SH_Menu_SloppySubMenus: return "SH_Menu_SloppySubMenus";
	case SH_Table_GridLineColor: return "SH_Table_GridLineColor";
	case SH_LineEdit_PasswordCharacter: return "SH_LineEdit_PasswordCharacter";
	case SH_DialogButtons_DefaultButton: return "SH_DialogButtons_DefaultButton";
	case SH_ToolBox_SelectedPageTitleBold: return "SH_ToolBox_SelectedPageTitleBold";
	case SH_TabBar_PreferNoArrows: return "SH_TabBar_PreferNoArrows";
	case SH_ScrollBar_LeftClickAbsolutePosition: return "SH_ScrollBar_LeftClickAbsolutePosition";
	case SH_ListViewExpand_SelectMouseType: return "SH_ListViewExpand_SelectMouseType";
	case SH_UnderlineShortcut: return "SH_UnderlineShortcut";
	case SH_SpinBox_AnimateButton: return "SH_SpinBox_AnimateButton";
	case SH_SpinBox_KeyPressAutoRepeatRate: return "SH_SpinBox_KeyPressAutoRepeatRate";
	case SH_SpinBox_ClickAutoRepeatRate: return "SH_SpinBox_ClickAutoRepeatRate";
	case SH_Menu_FillScreenWithScroll: return "SH_Menu_FillScreenWithScroll";
	case SH_ToolTipLabel_Opacity: return "SH_ToolTipLabel_Opacity";
	case SH_DrawMenuBarSeparator: return "SH_DrawMenuBarSeparator";
	case SH_TitleBar_ModifyNotification: return "SH_TitleBar_ModifyNotification";
	case SH_Button_FocusPolicy: return "SH_Button_FocusPolicy";
	case SH_MessageBox_UseBorderForButtonSpacing: return "SH_MessageBox_UseBorderForButtonSpacing";
	case SH_TitleBar_AutoRaise: return "SH_TitleBar_AutoRaise";
	case SH_ToolButton_PopupDelay: return "SH_ToolButton_PopupDelay";
	case SH_FocusFrame_Mask: return "SH_FocusFrame_Mask";
	case SH_RubberBand_Mask: return "SH_RubberBand_Mask";
	case SH_WindowFrame_Mask: return "SH_WindowFrame_Mask";
	case SH_SpinControls_DisableOnBounds: return "SH_SpinControls_DisableOnBounds";
	case SH_Dial_BackgroundRole: return "SH_Dial_BackgroundRole";
	case SH_ComboBox_LayoutDirection: return "SH_ComboBox_LayoutDirection";
	case SH_ItemView_EllipsisLocation: return "SH_ItemView_EllipsisLocation";
	case SH_ItemView_ShowDecorationSelected: return "SH_ItemView_ShowDecorationSelected";
	case SH_ItemView_ActivateItemOnSingleClick: return "SH_ItemView_ActivateItemOnSingleClick";
	case SH_ScrollBar_ContextMenu: return "SH_ScrollBar_ContextMenu";
	case SH_ScrollBar_RollBetweenButtons: return "SH_ScrollBar_RollBetweenButtons";
	case SH_Slider_AbsoluteSetButtons: return "SH_Slider_AbsoluteSetButtons";
	case SH_Slider_PageSetButtons: return "SH_Slider_PageSetButtons";
	case SH_Menu_KeyboardSearch: return "SH_Menu_KeyboardSearch";
	case SH_TabBar_ElideMode: return "SH_TabBar_ElideMode";
	case SH_DialogButtonLayout: return "SH_DialogButtonLayout";
	case SH_ComboBox_PopupFrameStyle: return "SH_ComboBox_PopupFrameStyle";
	case SH_MessageBox_TextInteractionFlags: return "SH_MessageBox_TextInteractionFlags";
	case SH_DialogButtonBox_ButtonsHaveIcons: return "SH_DialogButtonBox_ButtonsHaveIcons";
	case SH_SpellCheckUnderlineStyle: return "SH_SpellCheckUnderlineStyle";
	case SH_MessageBox_CenterButtons: return "SH_MessageBox_CenterButtons";
	case SH_Menu_SelectionWrap: return "SH_Menu_SelectionWrap";
	case SH_ItemView_MovementWithoutUpdatingSelection: return "SH_ItemView_MovementWithoutUpdatingSelection";
	case SH_ToolTip_Mask: return "SH_ToolTip_Mask";
	case SH_FocusFrame_AboveWidget: return "SH_FocusFrame_AboveWidget";
	case SH_TextControl_FocusIndicatorTextCharFormat: return "SH_TextControl_FocusIndicatorTextCharFormat";
	case SH_WizardStyle: return "SH_WizardStyle";
	case SH_ItemView_ArrowKeysNavigateIntoChildren: return "SH_ItemView_ArrowKeysNavigateIntoChildren";
	case SH_Menu_Mask: return "SH_Menu_Mask";
	case SH_Menu_FlashTriggeredItem: return "SH_Menu_FlashTriggeredItem";
	case SH_Menu_FadeOutOnHide: return "SH_Menu_FadeOutOnHide";
	case SH_SpinBox_ClickAutoRepeatThreshold: return "SH_SpinBox_ClickAutoRepeatThreshold";
	case SH_ItemView_PaintAlternatingRowColorsForEmptyArea: return "SH_ItemView_PaintAlternatingRowColorsForEmptyArea";
	case SH_FormLayoutWrapPolicy: return "SH_FormLayoutWrapPolicy";
	case SH_TabWidget_DefaultTabPosition: return "SH_TabWidget_DefaultTabPosition";
	case SH_ToolBar_Movable: return "SH_ToolBar_Movable";
	case SH_FormLayoutFieldGrowthPolicy: return "SH_FormLayoutFieldGrowthPolicy";
	case SH_FormLayoutFormAlignment: return "SH_FormLayoutFormAlignment";
	case SH_FormLayoutLabelAlignment: return "SH_FormLayoutLabelAlignment";
	case SH_ItemView_DrawDelegateFrame: return "SH_ItemView_DrawDelegateFrame";
	case SH_TabBar_CloseButtonPosition: return "SH_TabBar_CloseButtonPosition";
	case SH_DockWidget_ButtonsHaveFrame: return "SH_DockWidget_ButtonsHaveFrame";
	case SH_ToolButtonStyle: return "SH_ToolButtonStyle";
	case SH_RequestSoftwareInputPanel: return "SH_RequestSoftwareInputPanel";
	case SH_ScrollBar_Transient: return "SH_ScrollBar_Transient";
	default:
	case SH_CustomBase:
		if(stylehint >= SH_CustomBase)
			return "**Above SH_CustomBase**";
		return "**Unknown SH**";
	}
}
