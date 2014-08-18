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

#ifndef ANIMATEDFLOAT_H
#define ANIMATEDFLOAT_H

#include <QtCore/QEasingCurve>

//=============================================================================
/// <summary>
/// Animates a float between two values based on a duration and an easing
/// curve.
/// </summary>
class AnimatedFloat
{
public: // Members ------------------------------------------------------------
	float			startValue;
	float			endValue;
	QEasingCurve	easingCurve;
	float			duration;
	float			progress;

public: // Constructor/destructor ---------------------------------------------
	AnimatedFloat();
	AnimatedFloat(float value);
	AnimatedFloat(
		float startValue_, float endValue_, const QEasingCurve &easingCurve_,
		float duration_);
	AnimatedFloat(const AnimatedFloat &other);

public: // Methods ------------------------------------------------------------
	float			currentValue() const;
	void			addTime(float dt);
	bool			atEnd() const;

	// Operators
	AnimatedFloat &	operator=(const AnimatedFloat &r);

private:
	float			lerp(float a, float b, float t) const;
};
//=============================================================================

/**
* Uninitialized default constructor
*/
inline AnimatedFloat::AnimatedFloat()
{
}

/**
* Constructor from single value
*/
inline AnimatedFloat::AnimatedFloat(float value)
	: startValue(value)
	, endValue(value)
	, easingCurve(QEasingCurve::Linear)
	, duration(0.0f)
	, progress(0.0f)
{
}

/**
* Constructor from full input
*/
inline AnimatedFloat::AnimatedFloat(
	float startValue_, float endValue_, const QEasingCurve &easingCurve_,
	float duration_)
	: startValue(startValue_)
	, endValue(endValue_)
	, easingCurve(easingCurve_)
	, duration(duration_)
	, progress(0.0f)
{
}

/**
* Copy constructor
*/
inline AnimatedFloat::AnimatedFloat(const AnimatedFloat &other)
	: startValue(other.startValue)
	, endValue(other.endValue)
	, easingCurve(other.easingCurve)
	, duration(other.duration)
	, progress(other.progress)
{
}

inline float AnimatedFloat::currentValue() const
{
	if(duration == 0.0f)
		return endValue;
	return lerp(startValue, endValue,
		easingCurve.valueForProgress(qBound(0.0f, progress / duration, 1.0f)));
}

inline void AnimatedFloat::addTime(float dt)
{
	progress += dt;
	if(progress > duration)
		progress = duration;
}

inline bool AnimatedFloat::atEnd() const
{
	return (progress >= duration) ? true : false;
}

inline AnimatedFloat &AnimatedFloat::operator=(const AnimatedFloat &r)
{
	startValue = r.startValue;
	endValue = r.endValue;
	easingCurve = r.easingCurve;
	duration = r.duration;
	progress = r.progress;
	return *this;
}

inline float AnimatedFloat::lerp(float a, float b, float t) const
{
	return a + t * (b - a);
}

#endif // ANIMATEDFLOAT_H
