#include "Core/Math/Scalar.h"

namespace traktor
{

T_MATH_INLINE Scalar::Scalar()
{
}

T_MATH_INLINE Scalar::Scalar(const Scalar& value)
:	m_data(value.m_data)
{
}

T_MATH_INLINE Scalar::Scalar(float value)
{
	m_data = _mm_load1_ps(&value);
}

T_MATH_INLINE Scalar::Scalar(__m128 value)
:	m_data(value)
{
}

T_MATH_INLINE Scalar Scalar::operator - () const
{
	static const uint32_t T_ALIGN16 c_negateMask[] = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
	__m128 mask = _mm_load_ps((const float *)c_negateMask);
	return Scalar(_mm_xor_ps(m_data, mask));
}

T_MATH_INLINE Scalar& Scalar::operator += (const Scalar& v)
{
	m_data = _mm_add_ps(m_data, v.m_data);
	return *this;
}

T_MATH_INLINE Scalar& Scalar::operator -= (const Scalar& v)
{
	m_data = _mm_sub_ps(m_data, v.m_data);
	return *this;
}

T_MATH_INLINE Scalar& Scalar::operator *= (const Scalar& v)
{
	m_data = _mm_mul_ps(m_data, v.m_data);
	return *this;
}

T_MATH_INLINE Scalar& Scalar::operator /= (const Scalar& v)
{
	m_data = _mm_div_ps(m_data, v.m_data);
	return *this;
}

T_MATH_INLINE bool Scalar::operator < (const Scalar& r)
{
	__m128 tmp = _mm_cmplt_ps(m_data, r.m_data);
	uint32_t imm; _mm_store_ss((float*)&imm, tmp);
	return imm != 0;
}

T_MATH_INLINE bool Scalar::operator <= (const Scalar& r)
{
	__m128 tmp = _mm_cmple_ps(m_data, r.m_data);
	uint32_t imm; _mm_store_ss((float*)&imm, tmp);
	return imm != 0;
}

T_MATH_INLINE bool Scalar::operator > (const Scalar& r)
{
	__m128 tmp = _mm_cmpgt_ps(m_data, r.m_data);
	uint32_t imm; _mm_store_ss((float*)&imm, tmp);
	return imm != 0;
}

T_MATH_INLINE bool Scalar::operator >= (const Scalar& r)
{
	__m128 tmp = _mm_cmpge_ps(m_data, r.m_data);
	uint32_t imm; _mm_store_ss((float*)&imm, tmp);
	return imm != 0;
}

T_MATH_INLINE bool Scalar::operator == (const Scalar& r)
{
	__m128 tmp = _mm_cmpeq_ps(m_data, r.m_data);
	uint32_t imm; _mm_store_ss((float*)&imm, tmp);
	return imm != 0;
}

T_MATH_INLINE bool Scalar::operator != (const Scalar& r)
{
	__m128 tmp = _mm_cmpneq_ps(m_data, r.m_data);
	uint32_t imm; _mm_store_ss((float*)&imm, tmp);
	return imm != 0;
}

T_MATH_INLINE Scalar::operator float () const
{
	float tmp; _mm_store_ss(&tmp, m_data);
	return tmp;
}

T_MATH_INLINE Scalar operator + (const Scalar& l, const Scalar& r)
{
	return Scalar(_mm_add_ps(l.m_data, r.m_data));
}

T_MATH_INLINE Scalar operator - (const Scalar& l, const Scalar& r)
{
	return Scalar(_mm_sub_ps(l.m_data, r.m_data));
}

T_MATH_INLINE Scalar operator * (const Scalar& l, const Scalar& r)
{
	return Scalar(_mm_mul_ps(l.m_data, r.m_data));
}

T_MATH_INLINE Scalar operator / (const Scalar& l, const Scalar& r)
{
	return Scalar(_mm_div_ps(l.m_data, r.m_data));
}

T_MATH_INLINE Scalar abs(const Scalar& s)
{
	static const uint32_t T_ALIGN16 c_absoluteMask[] = { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff };
	__m128 mask = _mm_load_ps((const float *)c_absoluteMask);
	return Scalar(_mm_and_ps(s.m_data, mask));
}

T_MATH_INLINE Scalar squareRoot(const Scalar& s)
{
	return Scalar(_mm_sqrt_ps(s.m_data));
}

T_MATH_INLINE Scalar lerp(const Scalar& a, const Scalar& b, const Scalar& c)
{
	return a * (Scalar(1.0f) - c) + b * c;
}

}
