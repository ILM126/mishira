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

#ifndef CROPINFO_H
#define CROPINFO_H

#include "common.h"
#include <QtCore/QMargins>

//=============================================================================
/// <summary>
/// A helper class for calculating texture cropping.
/// </summary>
class CropInfo
{
public: // Datatypes ----------------------------------------------------------
	enum Anchor {
		TopLeft = 0,
		BottomRight
	};

protected: // Members ---------------------------------------------------------
	QMargins	m_margins;
	Anchor		m_leftAnchor;
	Anchor		m_rightAnchor;
	Anchor		m_topAnchor;
	Anchor		m_botAnchor;

public: // Constructor/destructor ---------------------------------------------
	CropInfo();
	CropInfo(const CropInfo &other);
	virtual ~CropInfo();

public: // Methods ------------------------------------------------------------
	bool		operator==(const CropInfo &r) const;
	bool		operator!=(const CropInfo &r) const;

	// Individual setters/getters
	void		setLeftMargin(int left);
	int			getLeftMargin() const;
	void		setRightMargin(int right);
	int			getRightMargin() const;
	void		setTopMargin(int top);
	int			getTopMargin() const;
	void		setBottomMargin(int bottom);
	int			getBottomMargin() const;
	void		setLeftAnchor(Anchor anchor);
	Anchor		getLeftAnchor() const;
	void		setRightAnchor(Anchor anchor);
	Anchor		getRightAnchor() const;
	void		setTopAnchor(Anchor anchor);
	Anchor		getTopAnchor() const;
	void		setBottomAnchor(Anchor anchor);
	Anchor		getBottomAnchor() const;

	// Grouped setters/getters
	void		setMargins(const QMargins &margins);
	QMargins	getMargins() const;
	void		setAnchors(
		Anchor left, Anchor right, Anchor top, Anchor bottom);
	void		getAnchors(
		Anchor *left, Anchor *right, Anchor *top, Anchor *bottom) const;

	QRect		calcCroppedRectForSize(const QSize &size) const;
};
QDataStream &operator<<(QDataStream &s, const CropInfo &info);
QDataStream &operator>>(QDataStream &s, CropInfo &info);
//=============================================================================

inline void CropInfo::setLeftMargin(int left)
{
	m_margins.setLeft(left);
}

inline int CropInfo::getLeftMargin() const
{
	return m_margins.left();
}

inline void CropInfo::setRightMargin(int right)
{
	m_margins.setRight(right);
}

inline int CropInfo::getRightMargin() const
{
	return m_margins.right();
}

inline void CropInfo::setTopMargin(int top)
{
	m_margins.setTop(top);
}

inline int CropInfo::getTopMargin() const
{
	return m_margins.top();
}

inline void CropInfo::setBottomMargin(int bottom)
{
	m_margins.setBottom(bottom);
}

inline int CropInfo::getBottomMargin() const
{
	return m_margins.bottom();
}

inline void CropInfo::setLeftAnchor(Anchor anchor)
{
	m_leftAnchor = anchor;
}

inline CropInfo::Anchor CropInfo::getLeftAnchor() const
{
	return m_leftAnchor;
}

inline void CropInfo::setRightAnchor(Anchor anchor)
{
	m_rightAnchor = anchor;
}

inline CropInfo::Anchor CropInfo::getRightAnchor() const
{
	return m_rightAnchor;
}

inline void CropInfo::setTopAnchor(Anchor anchor)
{
	m_topAnchor = anchor;
}

inline CropInfo::Anchor CropInfo::getTopAnchor() const
{
	return m_topAnchor;
}

inline void CropInfo::setBottomAnchor(Anchor anchor)
{
	m_botAnchor = anchor;
}

inline CropInfo::Anchor CropInfo::getBottomAnchor() const
{
	return m_botAnchor;
}

inline void CropInfo::setMargins(const QMargins &margins)
{
	m_margins = margins;
}

inline QMargins CropInfo::getMargins() const
{
	return m_margins;
}

inline void CropInfo::setAnchors(
	CropInfo::Anchor left, CropInfo::Anchor right, CropInfo::Anchor top,
	CropInfo::Anchor bottom)
{
	m_leftAnchor = left;
	m_rightAnchor = right;
	m_topAnchor = top;
	m_botAnchor = bottom;
}

inline void CropInfo::getAnchors(
	CropInfo::Anchor *left, CropInfo::Anchor *right, CropInfo::Anchor *top,
	CropInfo::Anchor *bottom) const
{
	if(left != NULL)
		*left = m_leftAnchor;
	if(right != NULL)
		*right = m_rightAnchor;
	if(top != NULL)
		*top = m_topAnchor;
	if(bottom != NULL)
		*bottom = m_botAnchor;
}

#endif // CROPINFO_H
