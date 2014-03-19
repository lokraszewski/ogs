/**
 * \author Norihiro Watanabe
 * \date   2014-03-18
 *
 * \copyright
 * Copyright (c) 2013, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#include "LinearInterpolationAlongSurface.h"

#include <limits>
#include <array>
#include <cassert>

#include "MathLib/Vector3.h"
#include "GeoLib/Surface.h"
#include "GeoLib/Triangle.h"
#include "GeoLib/AnalyticalGeometry.h"
#include "MeshLib/Mesh.h"
#include "MeshLib/Node.h"
#include "MeshGeoToolsLib/MeshNodesAlongSurface.h"

namespace NumLib
{

LinearInterpolationAlongSurface::LinearInterpolationAlongSurface(
		const GeoLib::Surface& sfc,
		const std::vector<std::size_t>& vec_interpolate_point_ids,
		const std::vector<double>& vec_interpolate_point_values,
		const double default_value)
: _sfc(sfc), _vec_interpolate_point_ids(vec_interpolate_point_ids),
  _vec_interpolate_point_values(vec_interpolate_point_values), _default_value(default_value)
{
	assert(vec_interpolate_point_ids.size()==vec_interpolate_point_values.size());
}

double LinearInterpolationAlongSurface::operator()(const GeoLib::Point& pnt) const
{
	const double* coords = pnt.getCoords();
	if (!_sfc.isPntInBoundingVolume(coords))
		return _default_value;
	auto* tri = _sfc.findTriangle(coords);
	if (tri == nullptr)
		return _default_value;

	std::array<double, 3> pnt_values;
	for (unsigned j=0; j<3; j++) {
		auto itr = std::find(_vec_interpolate_point_ids.begin(), _vec_interpolate_point_ids.end(), (*tri)[j]);
		if (itr != _vec_interpolate_point_ids.end()) {
			const std::size_t index = std::distance(_vec_interpolate_point_ids.begin(), itr);
			pnt_values[j] = _vec_interpolate_point_values[index];
		} else {
			pnt_values[j] = _default_value;
		}
	}
	double val = interpolateInTri(*tri, pnt_values.data(), coords);
	return val;
}

double LinearInterpolationAlongSurface::interpolateInTri(const GeoLib::Triangle &tri, double const* const vertex_values, double const* const pnt) const
{
	std::vector<GeoLib::Point> pnts;
	for (unsigned i=0; i<3; i++)
		pnts.emplace_back(*tri.getPoint(i));
	std::vector<GeoLib::Point*> p_pnts = {{&pnts[0], &pnts[1], &pnts[2]}};
	GeoLib::rotatePolygonToXY(p_pnts);

	const double* v1 = pnts[0].getCoords();
	const double* v2 = pnts[1].getCoords();
	const double* v3 = pnts[2].getCoords();
	const double area = MathLib::calcTriangleArea(v1, v2, v3);

	if (area==.0) {
		// take average if all points have the same coordinates
		double sum = .0;
		for (unsigned i=0; i<3; i++)
			sum += vertex_values[i];
		return sum / static_cast<double>(3);
	}

	double a[3], b[3], c[3];
	// 1st vertex
	a[0] = 0.5/area*(v2[0]*v3[1]-v3[0]*v2[1]);
	b[0] = 0.5/area*(v2[1]-v3[1]);
	c[0] = 0.5/area*(v3[0]-v2[0]);
	// 2nd vertex
	a[1] = 0.5/area*(v3[0]*v1[1]-v1[0]*v3[1]);
	b[1] = 0.5/area*(v3[1]-v1[1]);
	c[1] = 0.5/area*(v1[0]-v3[0]);
	// 3rd vertex
	a[2] = 0.5/area*(v1[0]*v2[1]-v2[0]*v1[1]);
	b[2] = 0.5/area*(v1[1]-v2[1]);
	c[2] = 0.5/area*(v2[0]-v1[0]);

	double val = .0;
	for (unsigned i=0; i<3; i++)
		val += (a[i]+b[i]*pnt[0]+c[i]*pnt[1]) * vertex_values[i];

	return val;
}

} // NumLib

