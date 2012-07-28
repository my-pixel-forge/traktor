#include <numeric>
#include "Core/Math/Const.h"
#include "Core/Math/Float.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Winding3.h"

namespace traktor
{

Winding3::Winding3()
{
}

Winding3::Winding3(const AlignedVector< Vector4 >& points_)
:	points(points_)
{
}

Winding3::Winding3(const Vector4* points_, size_t npoints)
:	points(npoints)
{
	for (size_t i = 0; i < npoints; ++i)
		points[i] = points_[i];
}

bool Winding3::angleIndices(uint32_t& outI1, uint32_t& outI2, uint32_t& outI3) const
{
	if (points.size() < 3)
		return false;

	for (size_t p = points.size() - 1, i = 0; i < points.size(); p = i++)
	{
		size_t n = (i + 1) % points.size();

		const Vector4& p1 = points[p];
		const Vector4& p2 = points[i];
		const Vector4& p3 = points[n];

		Vector4 e1 = p1 - p2;
		Vector4 e2 = p3 - p2;

		Scalar ln1 = e1.length();
		Scalar ln2 = e2.length();

		if (ln1 > FUZZY_EPSILON && ln2 > FUZZY_EPSILON)
		{
			e1 /= ln1;
			e2 /= ln2;

			Scalar phi = dot3(e1, e2);
			if (abs(phi) < 1.0f - FUZZY_EPSILON)
			{
				outI1 = p;
				outI2 = i;
				outI3 = n;
				return true;
			}
		}
	}

	return false;
}

bool Winding3::getPlane(Plane& outPlane) const
{
	// Get indices to points which form a good frame.
	uint32_t i1, i2, i3;
	if (!angleIndices(i1, i2, i3))
		return false;

	// Create plane from indexed points.
	outPlane = Plane(points[i1], points[i2], points[i3]);
	return true;
}

void Winding3::split(const Plane& plane, Winding3& outFront, Winding3& outBack) const
{
	for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++)
	{
		const Vector4& a = points[i];
		const Vector4& b = points[j];

		Scalar da = plane.distance(a);
		Scalar db = plane.distance(b);

		if ((da < -FUZZY_EPSILON && db > FUZZY_EPSILON) || (da > FUZZY_EPSILON && db < -FUZZY_EPSILON))
		{
			Scalar k;
			plane.segmentIntersection(a, b, k);
			T_ASSERT (k >= 0.0f && k <= 1.0f);

			Vector4 p = lerp(a, b, k);
			outFront.points.push_back(p);
			outBack.points.push_back(p);
		}

		if (da >= -FUZZY_EPSILON)
			outFront.points.push_back(a);
		if (da <= FUZZY_EPSILON)
			outBack.points.push_back(a);
	}
}

int Winding3::classify(const Plane& plane) const
{
	int side[2] = { 0, 0 };
	for (size_t i = 0; i < points.size(); ++i)
	{
		float d = plane.distance(points[i]);
		if (d > FUZZY_EPSILON)
			side[CfFront]++;
		else if (d < -FUZZY_EPSILON)
			side[CfBack]++;
	}

	if (side[CfFront] && !side[CfBack])
		return CfFront;
	else if (!side[CfFront] && side[CfBack])
		return CfBack;
	else if (!side[CfFront] && !side[CfBack])
		return CfCoplanar;

	T_ASSERT (side[CfFront] && side[CfBack]);
	return CfSpan;
}

float Winding3::area() const
{
	int32_t n = int32_t(points.size());
	if (n <= 2)
		return 0.0f;

	Plane plane;
	if (!getPlane(plane))
		return 0.0f;

	float area = 0.0f;
	int32_t i, j, k;

	Vector4 N = plane.normal();
	Vector4 A = N.absolute();
	int32_t coord = majorAxis3(A);

	for (i = 1, j = 2, k = 0; i <= n; i++, j++, k++)
	{
		int32_t ii = i % n;
		int32_t jj = j % n;
		int32_t kk = k % n;

		switch (coord)
		{
		case 0:
			area += (points[ii].y() * (points[jj].z() - points[kk].z()));
			break;

		case 1:
			area += (points[ii].x() * (points[jj].z() - points[kk].z()));
			break;

		case 2:
			area += (points[ii].x() * (points[jj].y() - points[kk].y()));
			break;
		}
	}

	float Aln = A.length();
	switch (coord)
	{
	case 0:
		area *= (Aln / (2.0f * A.x()));
		break;

	case 1:
		area *= (Aln / (2.0f * A.y()));
		break;

	case 2:
		area *= (Aln / (2.0f * A.z()));
		break;
	}

	return area;
}

Vector4 Winding3::center() const
{
	if (!points.empty())
		return std::accumulate(points.begin(), points.end(), Vector4::zero()) / Scalar(float(points.size()));
	else
		return Vector4::origo();
}

bool Winding3::rayIntersection(
	const Vector4& origin,
	const Vector4& direction,
	Scalar& outK,
	Vector4* outPoint
) const
{
	Plane plane;
	Vector4 p;
	
	if (!getPlane(plane))
		return false;
	
	if (!plane.rayIntersection(origin, direction, outK, &p) || outK <= 0.0f)
		return false;
		
	Vector4 normal = plane.normal();
	int32_t major = majorAxis3(normal);
		
	// Use maximum axis to determine projection plane.
	Vector4 u(0.0f, 0.0f, 0.0f), v(0.0f, 0.0f, 0.0f);
	if (major == 0)	// X
	{
		u = normal.x() > 0.0f ? Vector4(0.0f, 0.0f, -1.0f) : Vector4(0.0f, 0.0f, 1.0f);
		v = Vector4(0.0f, 1.0f, 0.0f);
	}
	else if (major == 1)	// Y
	{
		u = normal.y() > 0.0f ? Vector4(0.0f, 0.0f, 1.0f) : Vector4(0.0f, 0.0f, -1.0f);
		v = Vector4(1.0f, 0.0f, 0.0f);
	}
	else if (major == 2)	// Z
	{
		u = normal.z() > 0.0f ? Vector4(1.0f, 0.0f, 0.0f) : Vector4(-1.0f, 0.0f, 0.0f);
		v = Vector4(0.0f, 1.0f, 0.0f);
	}

	// Project all points onto 2d plane.
	Vector2 projected[32];
	T_ASSERT (points.size() <= sizeof_array(projected));
	for (size_t i = 0; i < points.size(); ++i)
	{
		projected[i].x = dot3(u, points[i]);
		projected[i].y = dot3(v, points[i]);
	}
	
	// Use odd-even rule to determine if point is in polygon.
	Vector2 pnt(
		dot3(u, p),
		dot3(v, p)
	);
	
	bool pass = false;
	for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++)
	{
		float dx = projected[j].x - projected[i].x;
		float dy = projected[j].y - projected[i].y;
		
		if (abs(dy) <= FUZZY_EPSILON)
			continue;

		float mny = min(projected[i].y, projected[j].y);
		float mxy = max(projected[i].y, projected[j].y);
		
		float x = projected[i].x + dx * (pnt.y - projected[i].y) / dy;
		if (
			(pnt.y >= mny && pnt.y <= mxy) &&
			(pnt.x < x)
		)
			pass = !pass;
	}
	
	if (!pass)
		return false;
	
	if (outPoint)
		*outPoint = p;

	return true;
}

}
