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

#ifndef EDITABLELABEL_H
#define EDITABLELABEL_H

#include <QtWidgets/QLineEdit>

//=============================================================================
class EditableLabel : public QLineEdit
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	bool	m_inEditMode;
	bool	m_allowClickEdit;
	int		m_editTimer;
	bool	m_editTimerEnabled;
	bool	m_mouseDown;
	QColor	m_baseColor;
	QColor	m_origTextColor;
	QColor	m_borderColor;
	QColor	m_textColor;
	QString	m_fullText;
	QString	m_displayText;

public: // Constructor/destructor ---------------------------------------------
	EditableLabel(QWidget *parent = 0);
	EditableLabel(const QString &contents, QWidget *parent = 0);
	~EditableLabel();

public: // Methods ------------------------------------------------------------
	void			setInEditMode(bool editMode);
	bool			isInEditMode() const;
	void			setAllowClickEdit(bool allowClickEdit);
	bool			getAllowClickEdit() const;
	void			setTextColor(const QColor &color);
	QColor			getTextColor() const;
	void			setEditColors(
		const QColor &baseCol, const QColor &textCol, const QColor &borderCol);
	void			forceFocusLost();
	QMargins		getActualTextMargins();
	QString			text() const;
	QString			displayText() const;
	void			setText(const QString &text);

private:
	void			enterEditMode();
	void			exitEditMode();
	void			calculateDisplayText();

private: // Events ------------------------------------------------------------
	virtual bool	event(QEvent *ev);
	virtual void	keyPressEvent(QKeyEvent *ev);
	virtual void	mousePressEvent(QMouseEvent *ev);
	virtual void	mouseReleaseEvent(QMouseEvent *ev);
	virtual void	mouseDoubleClickEvent(QMouseEvent *ev);
	virtual void	focusOutEvent(QFocusEvent *ev);
	virtual void	timerEvent(QTimerEvent *ev);
	virtual void	paintEvent(QPaintEvent *ev);
	virtual void	resizeEvent(QResizeEvent *ev);

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void			internalTextChanged();
};
//=============================================================================

inline bool EditableLabel::isInEditMode() const
{
	return m_inEditMode;
}

inline void EditableLabel::setAllowClickEdit(bool allowClickEdit)
{
	m_allowClickEdit = allowClickEdit;
}

inline bool EditableLabel::getAllowClickEdit() const
{
	return m_allowClickEdit;
}

inline QColor EditableLabel::getTextColor() const
{
	return m_textColor;
}

/// Reimplementation
inline QString EditableLabel::text() const
{
	return m_fullText;
}

/// Reimplementation
inline QString EditableLabel::displayText() const
{
	return m_displayText;
}

#endif // EDITABLELABEL_H
