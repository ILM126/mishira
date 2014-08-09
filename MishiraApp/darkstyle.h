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

#ifndef DARKSTYLE_H
#define DARKSTYLE_H

#include <QtWidgets/QCommonStyle>

class QCheckBox;

//=============================================================================
class DarkStyle : public QCommonStyle
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	QVector<const QWidget *>	m_nextCheckStateIsTri; // HACK for check box rendering

public: // Constructor/destructor ---------------------------------------------
	DarkStyle();
	virtual ~DarkStyle();

public:
	void				addNextCheckStateIsTri(QCheckBox *w);
	void				removeNextCheckStateIsTri(QCheckBox *w);

public: // QStyle Methods -----------------------------------------------------
	virtual void		polish(QWidget *widget);
	virtual void		unpolish(QWidget *widget);
	virtual QRect		itemTextRect(
		const QFontMetrics &fm, const QRect &r, int flags, bool enabled,
		const QString &text) const;
	virtual void		drawItemText(QPainter *painter, const QRect &rect,
		int flags, const QPalette &pal, bool enabled, const QString &text,
		QPalette::ColorRole textRole = QPalette::NoRole) const;
	virtual void		drawPrimitive(
		PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
		const QWidget *w = NULL) const;
	virtual void		drawControl(
		ControlElement element, const QStyleOption *opt, QPainter *p,
		const QWidget *w = NULL) const;
	virtual QRect		subElementRect(
		SubElement subElement, const QStyleOption *option,
		const QWidget *widget = NULL) const;
	virtual void		drawComplexControl(
		ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
		const QWidget *widget = NULL) const;
	virtual SubControl	hitTestComplexControl(
		ComplexControl cc, const QStyleOptionComplex *opt, const QPoint &pt,
		const QWidget *widget = NULL) const;
	virtual QRect		subControlRect(
		ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc,
		const QWidget *widget = NULL) const;
	virtual int			pixelMetric(
		PixelMetric metric, const QStyleOption *option = NULL,
		const QWidget *widget = NULL) const;
	virtual QSize		sizeFromContents(
		ContentsType ct, const QStyleOption *opt, const QSize &contentsSize,
		const QWidget *w = NULL) const;
	virtual int			styleHint(
		StyleHint stylehint, const QStyleOption *opt = NULL,
		const QWidget *widget = NULL,
		QStyleHintReturn *returnData = NULL) const;

private: // Debug methods -----------------------------------------------------
	QString		primitiveElementToString(PrimitiveElement pe) const;
	QString		controlElementToString(ControlElement element) const;
	QString		subElementToString(SubElement subElement) const;
	QString		contentsTypeToString(ContentsType ct) const;
	QString		pixelMetricToString(PixelMetric metric) const;
	QString		styleHintToString(StyleHint stylehint) const;
};
//=============================================================================

#endif // DARKSTYLE_H
