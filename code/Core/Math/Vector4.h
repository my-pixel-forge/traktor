#ifndef traktor_Vector4_H
#define traktor_Vector4_H

#include "Core/Config.h"
#include "Core/Math/MathConfig.h"
#include "Core/Math/MathUtils.h"
#include "Core/Math/Scalar.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_CORE_EXPORT)
#define T_DLLCLASS T_DLLEXPORT
#else
#define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

/*! \brief 4d vector.
 * \ingroup Core
 */
T_MATH_ALIGN16 class T_DLLCLASS Vector4
{
public:
#if defined(T_MATH_USE_SSE2)
	__m128 m_data;
#else
	union
	{
		struct { float _x, _y, _z, _w; };
		float _e[4];
	};
#endif

	T_MATH_INLINE Vector4();

	T_MATH_INLINE Vector4(const Vector4& v);

#if defined(T_MATH_USE_SSE2)
	explicit T_MATH_INLINE Vector4(__m128 v);
#endif

	explicit T_MATH_INLINE Vector4(float x, float y, float z, float w = 0);

	explicit T_MATH_INLINE Vector4(const float* p);

	static T_MATH_INLINE const Vector4& zero();

	static T_MATH_INLINE const Vector4& origo();

	T_MATH_INLINE void set(float x, float y, float z, float w = 0);

	T_MATH_INLINE Scalar x() const;

	T_MATH_INLINE Scalar y() const;

	T_MATH_INLINE Scalar z() const;

	T_MATH_INLINE Scalar w() const;

	T_MATH_INLINE Vector4 xyz0() const;

	T_MATH_INLINE Vector4 xyz1() const;

	template < int iX, int iY, int iZ, int iW >
	T_MATH_INLINE Vector4 shuffle() const;

	T_MATH_INLINE Scalar length() const;

	T_MATH_INLINE Scalar length2() const;

	T_MATH_INLINE Vector4 normalized() const;

	T_MATH_INLINE Vector4 absolute() const;

	T_MATH_INLINE void store(float* out) const;

	T_MATH_INLINE Scalar get(int index) const;

	T_MATH_INLINE void set(int index, const Scalar& value);

	T_MATH_INLINE Vector4& operator = (const Vector4& v);

	T_MATH_INLINE Vector4 operator - () const;

	T_MATH_INLINE Vector4& operator += (const Scalar& v);

	T_MATH_INLINE Vector4& operator += (const Vector4& v);

	T_MATH_INLINE Vector4& operator -= (const Scalar& v);

	T_MATH_INLINE Vector4& operator -= (const Vector4& v);

	T_MATH_INLINE Vector4& operator *= (const Scalar& v);

	T_MATH_INLINE Vector4& operator *= (const Vector4& v);

	T_MATH_INLINE Vector4& operator /= (const Scalar& v);

	T_MATH_INLINE Vector4& operator /= (const Vector4& v);

	T_MATH_INLINE bool operator == (const Vector4& v) const;

	T_MATH_INLINE bool operator != (const Vector4& v) const;

	T_MATH_INLINE Scalar operator [] (int index) const;

	friend T_MATH_INLINE T_DLLCLASS Vector4 operator + (const Vector4& l, const Scalar& r);

	friend T_MATH_INLINE T_DLLCLASS Vector4 operator + (const Scalar& l, const Vector4& r);

	friend T_MATH_INLINE T_DLLCLASS Vector4 operator + (const Vector4& l, const Vector4& r);

	friend T_MATH_INLINE T_DLLCLASS Vector4 operator - (const Vector4& l, const Scalar& r);

	friend T_MATH_INLINE T_DLLCLASS Vector4 operator - (const Scalar& l, const Vector4& r);

	friend T_MATH_INLINE T_DLLCLASS Vector4 operator - (const Vector4& l, const Vector4& r);

	friend T_MATH_INLINE T_DLLCLASS Vector4 operator * (const Vector4& l, const Scalar& r);

	friend T_MATH_INLINE T_DLLCLASS Vector4 operator * (const Scalar& l, const Vector4& r);

	friend T_MATH_INLINE T_DLLCLASS Vector4 operator * (const Vector4& l, const Vector4& r);

	friend T_MATH_INLINE T_DLLCLASS Vector4 operator / (const Vector4& l, const Scalar& r);

	friend T_MATH_INLINE T_DLLCLASS Vector4 operator / (const Scalar& l, const Vector4& r);

	friend T_MATH_INLINE T_DLLCLASS Vector4 operator / (const Vector4& l, const Vector4& r);
};

#if defined(T_MATH_USE_SSE2)

template < int iX, int iY, int iZ, int iW >
T_MATH_INLINE Vector4 Vector4::shuffle() const
{
	const int mask = iX | (iY << 2) | (iZ << 4) | (iW << 6);
	return Vector4(_mm_shuffle_ps(m_data, m_data, mask));
}

#else

template < int iX, int iY, int iZ, int iW >
T_MATH_INLINE Vector4 Vector4::shuffle() const
{
	return Vector4(_e[iX], _e[iY], _e[iZ], _e[iW]);
}

#endif

T_MATH_INLINE T_DLLCLASS Scalar dot3(const Vector4& l, const Vector4& r);

T_MATH_INLINE T_DLLCLASS Scalar dot4(const Vector4& l, const Vector4& r);

T_MATH_INLINE T_DLLCLASS Vector4 cross(const Vector4& l, const Vector4& r);

T_MATH_INLINE T_DLLCLASS Vector4 lerp(const Vector4& a, const Vector4& b, const Scalar& c);

T_MATH_INLINE T_DLLCLASS Vector4 reflect(const Vector4& v, const Vector4& at);

T_MATH_INLINE T_DLLCLASS int majorAxis3(const Vector4& v);

T_MATH_INLINE T_DLLCLASS Vector4 min(const Vector4& l, const Vector4& r);

T_MATH_INLINE T_DLLCLASS Vector4 max(const Vector4& l, const Vector4& r);

}

#if defined(T_MATH_USE_INLINE)
#	if defined(T_MATH_USE_SSE2)
#		include "Core/Math/Sse2/Vector4.inl"
#	else
#		include "Core/Math/Std/Vector4.inl"
#	endif
#endif

#endif	// traktor_Vector4_H
