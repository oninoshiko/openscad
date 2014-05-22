/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "export.h"
#include "polyset.h"
#include "polyset-utils.h"
#include "dxfdata.h"

#include <boost/foreach.hpp>

#ifdef ENABLE_CGAL
#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include "cgalutils.h"

static void append_stl(const PolySet &ps, std::ostream &output)
{
	PolySet triangulated(3);
	PolysetUtils::tessellate_faces(ps, triangulated);

	BOOST_FOREACH(const PolySet::Polygon &p, triangulated.polygons) {
		output << "  facet normal 0 0 0\n";
		output << "    outer loop\n";
		assert(p.size() == 3); // STL only allows triangles
		BOOST_FOREACH(const Vector3d &v, p) {
			output << "vertex " << v[0] << " " << v[1] << " " << v[2] << "\n";
		}
		output << "    endloop\n";
		output << "  endfacet\n";
	}
}

static void append_stl(const CGAL_Polyhedron &P, std::ostream &output)
{
	typedef CGAL_Polyhedron::Vertex                                 Vertex;
	typedef CGAL_Polyhedron::Vertex_const_iterator                  VCI;
	typedef CGAL_Polyhedron::Facet_const_iterator                   FCI;
	typedef CGAL_Polyhedron::Halfedge_around_facet_const_circulator HFCC;

	for (FCI fi = P.facets_begin(); fi != P.facets_end(); ++fi) {
		HFCC hc = fi->facet_begin();
		HFCC hc_end = hc;
		Vertex v1, v2, v3;
		v1 = *VCI((hc++)->vertex());
		v3 = *VCI((hc++)->vertex());
		do {
			v2 = v3;
			v3 = *VCI((hc++)->vertex());
			double x1 = CGAL::to_double(v1.point().x());
			double y1 = CGAL::to_double(v1.point().y());
			double z1 = CGAL::to_double(v1.point().z());
			double x2 = CGAL::to_double(v2.point().x());
			double y2 = CGAL::to_double(v2.point().y());
			double z2 = CGAL::to_double(v2.point().z());
			double x3 = CGAL::to_double(v3.point().x());
			double y3 = CGAL::to_double(v3.point().y());
			double z3 = CGAL::to_double(v3.point().z());
			std::stringstream stream;
			stream << x1 << " " << y1 << " " << z1;
			std::string vs1 = stream.str();
			stream.str("");
			stream << x2 << " " << y2 << " " << z2;
			std::string vs2 = stream.str();
			stream.str("");
			stream << x3 << " " << y3 << " " << z3;
			std::string vs3 = stream.str();
			if (vs1 != vs2 && vs1 != vs3 && vs2 != vs3) {
				// The above condition ensures that there are 3 distinct vertices, but
				// they may be collinear. If they are, the unit normal is meaningless
				// so the default value of "1 0 0" can be used. If the vertices are not
				// collinear then the unit normal must be calculated from the
				// components.
				if (!CGAL::collinear(v1.point(),v2.point(),v3.point())) {
					CGAL_Polyhedron::Traits::Vector_3 normal = CGAL::normal(v1.point(),v2.point(),v3.point());
					output << "  facet normal "
								 << CGAL::sign(normal.x()) * sqrt(CGAL::to_double(normal.x()*normal.x()/normal.squared_length()))
								 << " "
								 << CGAL::sign(normal.y()) * sqrt(CGAL::to_double(normal.y()*normal.y()/normal.squared_length()))
								 << " "
								 << CGAL::sign(normal.z()) * sqrt(CGAL::to_double(normal.z()*normal.z()/normal.squared_length()))
								 << "\n";
				}
				else output << "  facet normal 1 0 0\n";
				output << "    outer loop\n";
				output << "      vertex " << vs1 << "\n";
				output << "      vertex " << vs2 << "\n";
				output << "      vertex " << vs3 << "\n";
				output << "    endloop\n";
				output << "  endfacet\n";
			}
		} while (hc != hc_end);
	}
}

/*!
	Saves the current 3D CGAL Nef polyhedron as STL to the given file.
	The file must be open.
 */
static void append_stl(const CGAL_Nef_polyhedron &root_N, std::ostream &output)
{
	if (!root_N.p3->is_simple()) {
		PRINT("Object isn't a valid 2-manifold! Modify your design.\n");
	}

	bool usePolySet = false;
	if (usePolySet) {
		PolySet ps(3);
		bool err = createPolySetFromNefPolyhedron3(*(root_N.p3), ps);
		if (err) { PRINT("ERROR: Nef->PolySet failed"); }
		else {
			append_stl(ps, output);
		}
	}
	else {
		CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
		try {
			CGAL_Polyhedron P;
			//root_N.p3->convert_to_Polyhedron(P);
			bool err = nefworkaround::convert_to_Polyhedron<CGAL_Kernel3>( *(root_N.p3), P );
			if (err) {
				PRINT("ERROR: CGAL NefPolyhedron->Polyhedron conversion failed");
				return;
			}
			append_stl(P, output);
		}
		catch (const CGAL::Assertion_exception &e) {
			PRINTB("CGAL error in CGAL_Nef_polyhedron3::convert_to_Polyhedron(): %s", e.what());
		}
		catch (...) {
			PRINT("CGAL unknown error in CGAL_Nef_polyhedron3::convert_to_Polyhedron()");
		}
		CGAL::set_error_behaviour(old_behaviour);
	}
}

void append_stl(const shared_ptr<const Geometry> &geom, std::ostream &output)
{
	if (const GeometryList *geomlist = dynamic_cast<const GeometryList *>(geom.get())) {
		BOOST_FOREACH(const shared_ptr<const Geometry> &geom, geomlist->getChildren()) {
			append_stl(geom, output);
		}
	}
	else if (const CGAL_Nef_polyhedron *N = dynamic_cast<const CGAL_Nef_polyhedron *>(geom.get())) {
		append_stl(*N, output);
	}
	else if (const PolySet *ps = dynamic_cast<const PolySet *>(geom.get())) {
		append_stl(*ps, output);
	}
	else if (const Polygon2d *poly = dynamic_cast<const Polygon2d *>(geom.get())) {
		assert(false && "Unsupported file format");
	} else {
		assert(false && "Not implemented");
	}
}

void export_stl(const shared_ptr<const Geometry> &geom, std::ostream &output)
{
	setlocale(LC_NUMERIC, "C"); // Ensure radix is . (not ,) in output
	output << "solid OpenSCAD_Model\n";

	append_stl(geom, output);

	output << "endsolid OpenSCAD_Model\n";
	setlocale(LC_NUMERIC, "");      // Set default locale
}

#endif // ENABLE_CGAL