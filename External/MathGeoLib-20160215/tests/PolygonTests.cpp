#include <stdio.h>
#include <stdlib.h>

#include "../src/MathGeoLib.h"
#include "../src/Math/myassert.h"
#include "TestRunner.h"

MATH_IGNORE_UNUSED_VARS_WARNING

UNIQUE_TEST(Polygon_collinear_points_Plane)
{
	math::Polygon poly;
	poly.p.push_back(POINT_VEC(4980, 8600, 13060));
	poly.p.push_back(POINT_VEC(4849.33301f, 8600, 13060));
	poly.p.push_back(POINT_VEC(4820, 8600, 13060));
	poly.p.push_back(POINT_VEC(4820, 8600, 12835));
	poly.p.push_back(POINT_VEC(4980, 8600, 12835));

	assert1(!poly.IsDegenerate(), poly);
	assert1(!poly.IsNull(), poly);
	assert1(poly.IsPlanar(), poly);
	assert1(poly.IsFinite(), poly);

	math::Plane plane = poly.PlaneCCW();

	assert1(!plane.IsDegenerate(), plane);

	for(size_t i = 0; i < poly.p.size(); ++i)
		assert(plane.Contains(poly.p[i]));
}

UNIQUE_TEST(Polygon_IsPlanarCase)
{
	Polygon p;
	p.p.push_back(POINT_VEC(0.001175f, 0.f, 0.f));
	p.p.push_back(POINT_VEC(0.f,       0.f, 0.f));
	p.p.push_back(POINT_VEC(0.f,       0.f, 1.f));
	p.p.push_back(POINT_VEC(0.001175f, 0.f, 1.f));
	assert(p.IsPlanar());
}

UNIQUE_TEST(Polygon_IsPlanarCase2)
{
	Polygon p;
	p.p.push_back(POINT_VEC(35.2095566f,148.158905f,13.9513502f));
	p.p.push_back(POINT_VEC(36.0442657f,147.617859f,13.1296129f));
	p.p.push_back(POINT_VEC(185.938354f,-49.2594376f,24.0656204f));
	p.p.push_back(POINT_VEC(-38.1764603f,96.0109634f,244.697083f));
	assert(p.IsPlanar());
}

UNIQUE_TEST(Polygon_IsPlanarCase3)
{
	Polygon p;
	p.p.push_back(POINT_VEC(-134.367065f,-26.9767799f,5.93324137f));
	p.p.push_back(POINT_VEC(-134.202698f,-27.2182236f,6.43854856f));
	p.p.push_back(POINT_VEC(-23.2372704f,4.65003967f,204.453552f));
	p.p.push_back(POINT_VEC(-130.144333f,161.677658f,-124.184113f));
	assert(p.IsPlanar());
}

UNIQUE_TEST(Triangle_ContainsPoint)
{
	Triangle t(POINT_VEC(37.8599548f,-7.36342621f,62.7666626f),
		POINT_VEC(112.436401f,-119.294914f,-119.175751f),
		POINT_VEC(99.9088364f,-100.386391f,-88.2388687f));

	vec pt = POINT_VEC(96.5959015f,-95.449379f,-80.2810669f);

	assert(t.Contains(pt));
}
