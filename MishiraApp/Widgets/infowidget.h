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

#ifndef INFOWIDGET_H
#define INFOWIDGET_H

#include <QtCore/QMap>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>

class AudioInput;
class Target;

//=============================================================================
/// <summary>
/// The majority of this class is copied from `StyledRadioTile`. It does not
/// inherit from that class however because this one has a more advanced
/// widget layout.
/// </summary>
class InfoWidget : public QWidget
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	QRadioButton		m_radio;
	QLabel				m_titleLabel;
	QLabel				m_iconLabel;
	QMap<int, QLabel *>	m_leftText;
	QMap<int, QLabel *>	m_rightText;
	int					m_rightTextMaxWidth;
	QGridLayout			m_mainLayout;
	QWidget				m_textWidget;
	QGridLayout			m_textLayout;
	bool				m_selected;
	bool				m_mouseDown;

public: // Constructor/destructor ---------------------------------------------
	InfoWidget(QWidget *parent = 0);
	~InfoWidget();

public: // Methods ------------------------------------------------------------
	QRadioButton *	getButton();
	void			select();
	bool			isSelected() const;
	void			setTitle(const QString &text);
	void			setIconPixmap(const QPixmap &icon);
	void			setItemText(
		int id, const QString &left, const QString &right,
		bool doUpdateLayout);

protected:
	QSize			minimumSizeHint() const;
	void			updateChildLabel(QLabel *label);
	void			updateLayout();

protected: // Events ----------------------------------------------------------
	virtual void	paintEvent(QPaintEvent *ev);
	virtual void	mousePressEvent(QMouseEvent *ev);
	virtual void	mouseReleaseEvent(QMouseEvent *ev);
	virtual void	mouseDoubleClickEvent(QMouseEvent *ev);
	virtual void	focusOutEvent(QFocusEvent *ev);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void			clicked();
	void			rightClicked();
	void			doubleClicked();

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			radioChanged(bool checked);
};
//=============================================================================

inline QRadioButton *InfoWidget::getButton()
{
	return &m_radio;
}

inline bool InfoWidget::isSelected() const
{
	return m_selected;
}

//=============================================================================
class TargetInfoWidget : public InfoWidget
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	Target *	m_target;

public: // Constructor/destructor ---------------------------------------------
	TargetInfoWidget(Target *target, QWidget *parent = 0);
	~TargetInfoWidget();

public: // Methods ------------------------------------------------------------
	Target *	getTarget() const;
};
//=============================================================================

inline Target *TargetInfoWidget::getTarget() const
{
	return m_target;
}

//=============================================================================
class AudioInputInfoWidget : public InfoWidget
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	AudioInput *	m_input;

public: // Constructor/destructor ---------------------------------------------
	AudioInputInfoWidget(AudioInput *input, QWidget *parent = 0);
	~AudioInputInfoWidget();

public: // Methods ------------------------------------------------------------
	AudioInput *	getInput() const;
};
//=============================================================================

inline AudioInput *AudioInputInfoWidget::getInput() const
{
	return m_input;
}

#endif // INFOWIDGET_H
