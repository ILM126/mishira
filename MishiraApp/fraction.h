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

#ifndef FRACTION_H
#define FRACTION_H

#include <cstdint>

//=============================================================================
template <typename T>
class FractionGeneric
{
public: // Members ------------------------------------------------------------
	T	numerator;
	T	denominator;

public: // Constructor/destructor ---------------------------------------------
	inline FractionGeneric()
		: numerator(0)
		, denominator(0)
	{
	};

	inline FractionGeneric(const T &num, const T &denom)
		: numerator(num)
		, denominator(denom)
	{
	};

public: // Methods ------------------------------------------------------------
	float asFloat() const
	{
		return (float)numerator / (float)denominator;
	};

	// "0/anything" is zero
	bool isZero() const
	{
		return numerator == 0;
	};

	// "0/0" is legal and is interpreted as zero.
	bool isLegal() const
	{
		return denominator != 0 || numerator == 0;
	};

	void reduce()
	{
		T common = gcd(numerator, denominator);
		numerator /= common;
		denominator /= common;
	};

	FractionGeneric<T> reduced() const
	{
		if(isZero())
			return FractionGeneric<T>(0, 0);
		T common = gcd(numerator, denominator);
		return FractionGeneric<T>(numerator / common, denominator / common);
	};

	bool operator==(const FractionGeneric<T> &r) const
	{
		FractionGeneric<T> a = reduced();
		FractionGeneric<T> b = r.reduced();
		return a.numerator == b.numerator && a.denominator == b.denominator;
	};

	bool operator!=(const FractionGeneric<T> &r) const
	{
		FractionGeneric<T> a = reduced();
		FractionGeneric<T> b = r.reduced();
		return a.numerator != b.numerator || a.denominator != b.denominator;
	};

private:
	// Get the greatest common divisor of two numbers using the Euclidean
	// algorithm. See: http://en.wikipedia.org/wiki/Euclidean_algorithm
	static T gcd(T a, T b)
	{
		while(b != 0) {
			T t = b;
			b = a % t;
			a = t;
		}
		return a;
	};
};
//=============================================================================

typedef FractionGeneric<unsigned int> Fraction;
typedef FractionGeneric<uint32_t> Fraction32;
typedef FractionGeneric<uint64_t> Fraction64;

#endif // FRACTION_H
