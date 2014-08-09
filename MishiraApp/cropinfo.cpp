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

#include "cropinfo.h"

CropInfo::CropInfo()
	: m_margins(0, 0, 0, 0)
	, m_leftAnchor(CropInfo::TopLeft)
	, m_rightAnchor(CropInfo::BottomRight)
	, m_topAnchor(CropInfo::TopLeft)
	, m_botAnchor(CropInfo::BottomRight)
{
}

CropInfo::CropInfo(const CropInfo &other)
	: m_margins(other.getMargins())
	, m_leftAnchor(CropInfo::TopLeft)
	, m_rightAnchor(CropInfo::BottomRight)
	, m_topAnchor(CropInfo::TopLeft)
	, m_botAnchor(CropInfo::BottomRight)
{
	other.getAnchors(
		&m_leftAnchor, &m_rightAnchor, &m_topAnchor, &m_botAnchor);
}

CropInfo::~CropInfo()
{
}

bool CropInfo::operator==(const CropInfo &r) const
{
	CropInfo::Anchor leftAnchor, rightAnchor, topAnchor, botAnchor;
	r.getAnchors(&leftAnchor, &rightAnchor, &topAnchor, &botAnchor);
	return (m_margins == r.getMargins() && m_leftAnchor == leftAnchor &&
		m_rightAnchor == rightAnchor && m_topAnchor == topAnchor &&
		m_botAnchor == botAnchor);
}

bool CropInfo::operator!=(const CropInfo &r) const
{
	CropInfo::Anchor leftAnchor, rightAnchor, topAnchor, botAnchor;
	r.getAnchors(&leftAnchor, &rightAnchor, &topAnchor, &botAnchor);
	return (m_margins != r.getMargins() || m_leftAnchor != leftAnchor ||
		m_rightAnchor != rightAnchor || m_topAnchor != topAnchor ||
		m_botAnchor != botAnchor);
}

/// <summary>
/// Calculates the rectangle of the texture that will be displayed based on the
/// specified input texture size. As the input size is typically the unscaled
/// texture size this method will return the subrectangle of the unscaled
/// texture that should be passed to the `GraphicsContext::prepareTexture()`
/// method.
/// </summary>
QRect CropInfo::calcCroppedRectForSize(const QSize &size) const
{
	QRect cropRect;
	if(m_leftAnchor == CropInfo::TopLeft)
		cropRect.setLeft(m_margins.left());
	else
		cropRect.setLeft(size.width() - m_margins.left());
	if(m_rightAnchor == CropInfo::TopLeft)
		cropRect.setRight(m_margins.right() - 1);
	else
		cropRect.setRight(size.width() - m_margins.right() - 1);
	if(m_topAnchor == CropInfo::TopLeft)
		cropRect.setTop(m_margins.top());
	else
		cropRect.setTop(size.height() - m_margins.top());
	if(m_botAnchor == CropInfo::TopLeft)
		cropRect.setBottom(m_margins.bottom() - 1);
	else {
		cropRect.setBottom(
			size.height() - m_margins.bottom() - 1);
	}
	if(!cropRect.isValid())
		cropRect = QRect(0, 0, 0, 0);
	return cropRect;
}

QDataStream &operator<<(QDataStream &s, const CropInfo &info)
{
	CropInfo::Anchor leftAnchor, rightAnchor, topAnchor, botAnchor;
	info.getAnchors(&leftAnchor, &rightAnchor, &topAnchor, &botAnchor);

	s << info.getMargins();
	s << (quint32)leftAnchor;
	s << (quint32)rightAnchor;
	s << (quint32)topAnchor;
	s << (quint32)botAnchor;

	return s;
}

QDataStream &operator>>(QDataStream &s, CropInfo &info)
{
	quint32 uint32Data;
	QMargins cropMargins;
	CropInfo::Anchor leftAnchor, rightAnchor, topAnchor, botAnchor;

	s >> cropMargins;
	s >> uint32Data;
	leftAnchor = (CropInfo::Anchor)uint32Data;
	s >> uint32Data;
	rightAnchor = (CropInfo::Anchor)uint32Data;
	s >> uint32Data;
	topAnchor = (CropInfo::Anchor)uint32Data;
	s >> uint32Data;
	botAnchor = (CropInfo::Anchor)uint32Data;

	info.setMargins(cropMargins);
	info.setAnchors(leftAnchor, rightAnchor, topAnchor, botAnchor);

	return s;
}
