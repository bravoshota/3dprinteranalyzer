#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QVector>
#include <QByteArray>
#include <QDataStream>
#include <QMessageBox>
#include <boost/cstdint.hpp> 
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/compute_average_spacing.h>
#include "functions.h"
#include<CGAL/Polyhedron_incremental_builder_3.h>
#include<CGAL/Polyhedron_3.h>

typedef Polyhedron::HalfedgeDS             HalfedgeDS;

// the utility class for CGAL mesh creation from arrays
template<class HDS>
class polyhedron_builder : public CGAL::Modifier_base<HDS> {
public:
	// input arrays
	std::vector<double> &coords;	// coordinates of vertices
	std::vector<int>    &tris;		// facets as vertices indices

	// initiate builder with arrays
	polyhedron_builder(std::vector<double> &_coords, std::vector<int> &_tris) : coords(_coords), tris(_tris) {}
	void operator()(HDS& hds) {
		typedef typename HDS::Vertex   Vertex;
		typedef typename Vertex::Point Point;

		// create a cgal incremental builder
		CGAL::Polyhedron_incremental_builder_3<HDS> B(hds, true);
		B.begin_surface(coords.size() / 3, tris.size() / 3);

		// add the polyhedron vertices
		for (int i = 0; i<(int)coords.size(); i += 3) {
			B.add_vertex(Point(coords[i + 0], coords[i + 1], coords[i + 2]));
		}

		// add the polyhedron facets
		for (int i = 0; i<(int)tris.size(); i += 3) {
			B.begin_facet();
			B.add_vertex_to_facet(tris[i + 0]);
			B.add_vertex_to_facet(tris[i + 1]);
			B.add_vertex_to_facet(tris[i + 2]);
			B.end_facet();
		}

		// finish up the surface
		B.end_surface();
	}
};

namespace stl {
	// utility object (the vertex)
	struct point {
		float x;
		float y;
		float z;
		// initiate the point with coordinates
		point() : x(0), y(0), z(0) {}
		point(float xp, float yp, float zp) : x(xp), y(yp), z(zp) {}
	};

	// output method of point
	std::ostream& operator<<(std::ostream& out, const point p) {
		out << p.x << " " << p.y << " " << p.z;
		return out;
	}
}

// define the type of STL file (ascii or binary)
int getStlFileFormat(const QString &path)
{
	// input variables:
	// path - the path of file to load

	// get the file
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
	{
		// can't open, return error
		qDebug("\n\tUnable to open \"%s\"", qPrintable(path));
		return STL_INVALID;
	}
	// get the size of file (bytes)
	QFileInfo fileInfo(path);
	size_t fileSize = fileInfo.size();

	// check the size of file
	if (fileSize < 15)
	{
		// the size is too short, return error
		qDebug("\n\tThe STL file is not long enough (%u bytes).", uint(fileSize));
		return STL_INVALID;
	}

	// check the header of ascii STL
	QByteArray sixBytes = file.read(6);
	if (sixBytes.startsWith("solid "))
	{
		// loop over lines of file
		QByteArray buf;
		QByteArray prev;
		do
		{
			prev = buf;
			buf = file.readLine();
		} while (!buf.isEmpty());
		// check the ending of ascii STL
		if (prev.startsWith("endsolid"))
			// header and ending are proper, return ascii type
			return STL_ASCII;
	}

	// come back to start of file
	if (!file.reset())
	{
		// can't do, return error
		qDebug("\n\tCannot seek to the 0th byte (before the header)");
		return STL_INVALID;
	}
	
	// check the size of file (the minimum is longer for binary)
	if (fileSize < 84)
	{
		// the size is too short, return error
		qDebug("\n\tThe STL file is not long enough (%u bytes).", uint(fileSize));
		return STL_INVALID;
	}

	// skip the header of binary STL
	if (!file.seek(80))
	{
		// can't do, return error
		qDebug("\n\tCannot seek to the 80th byte (after the header)");
		return STL_INVALID;
	}

	// read the number of facets
	QByteArray nTrianglesBytes = file.read(4);
	if (nTrianglesBytes.size() != 4)
	{
		// can't do, return error
		qDebug("\n\tCannot read the number of triangles (after the header)");
		return STL_INVALID;
	}

	// convert to int
	uint32_t nTriangles = *((uint32_t*)nTrianglesBytes.data());
	// check the length of file (header + facets)
	if (fileSize == (84 + (nTriangles * 50)))
		// if proper return binary type
		return STL_BINARY;

	// not proper, return error
	return STL_INVALID;
}

// convert ascii STL to OFF
char * ascStl2Off(char * filename)
{
	// input variables:
	// filename - the path of file to load

	// open the file
	std::ifstream obj(filename, std::ios::in);
	// define the maximum size of line to process
	const int buffsize = 100;
	char str[buffsize];
	char tstr[buffsize];
	
	// initiate the arrays of vertices and facets
	std::vector<stl::point> points;
	std::vector<Face> faces;
	int number_of_points = 0;
	int number_of_snapped_points = 0;

	// read the first line
	obj.getline(str, buffsize);

	// loop over the lines of file
	while (obj.getline(str, buffsize))
	{
		// count the leading spaces
		int b = 0;
		while (isspace(str[b])) b++;
		// skip the leading spaces and convert all chars to lowercase
		for (int tb = b; tb < buffsize; tb++) tstr[tb - b] = tolower(str[tb]);
		
		// set the rest of chars to space
		for (int tb = buffsize - b; tb < buffsize; tb++) tstr[tb] = ' ';

		// check do we find the start of new facet
		if (strncmp(tstr, "facet", 5) == 0) 
		{
			// initiate the new facet
			int index[3];
			// skip the line with normal data
			obj.getline(str, buffsize);
			
			// loop over facet's vertices
			for (int j = 0; j < 3; j++)
			{
				// read the line with vertex data
				obj.getline(str, buffsize);
				
				// count the leading spaces
				int b = 0;
				while (isspace(str[b])) b++;
				// skip the leading spaces and convert all chars to lowercase
				for (int tb = b; tb < buffsize; tb++) tstr[tb - b] = tolower(str[tb]);

				// set the rest of chars to space
				for (int tb = buffsize - b; tb < buffsize; tb++) tstr[tb] = ' ';

				// split the buffer by spaces
				std::vector<std::string> entities;
				boost::split(entities, tstr, boost::is_any_of(" "), boost::token_compress_on);
				// convert coordinates into float numbers (skip the "vertex" which has zero index)
				float x = stof(entities[1]);
				float y = stof(entities[2]);
				float z = stof(entities[3]);

				// initiate the checker of the vertex is already exist
				bool found_close_point = false;
				// loop over previous vertices
				for (int k = 0; k < points.size(); k++) {

					// check does the distance by X axis lower than eps (little constant)
					float dx = x - points[k].x;
					if (dx > -1E-5 && dx < 1E-5) {
						// check does the distance by Y axis lower than eps (little constant)
						float dy = y - points[k].y;
						if (dy > -1E-5 && dy < 1E-5) {
							// check does the distance by Z axis lower than eps (little constant)
							float dz = z - points[k].z;
							if (dz > -1E-5 && dz < 1E-5) {
								// the current vertex is close to existing one, set it into facet
								index[j] = k;
								found_close_point = true;
								number_of_snapped_points++;
								break;
							}
						}
					}
				}

				// the current vertex is not close to any existing
				if (!found_close_point) {
					// create new vertex
					stl::point p(x, y, z);
					// add it to array
					points.push_back(p);
					// set the index of new vertex into facet
					index[j] = number_of_points;
					number_of_points++;
				}
			}

			// add new facet into array
			faces.push_back(boost::make_tuple(index[0], index[1], index[2]));
			// skip the end of facet
			obj.getline(str, buffsize);
			obj.getline(str, buffsize);
		}
	}

	// construct the filename of new OFF file
	int offLen = (int)std::strlen(filename) + 1;
	char* offFile = new char[offLen];
	strncpy_s(offFile, offLen, filename, offLen - 4);
	strcat_s(offFile, offLen, "off");

	// open OFF file
	std::ofstream out;
	out.open(offFile);
	// output the header of OFF
	out << "OFF\n" << points.size() << " " << faces.size() << " 0" << std::endl << std::endl;

	// loop over the vertices
	for (int i = 0; i < points.size(); i++) {
		// output vertex
		out << points[i] << std::endl;
	}

	// loop over the facets
	for (int i = 0; i < faces.size(); i++) {
		// output facet
		out << "3 " << boost::get<0>(faces[i]) << " " << boost::get<1>(faces[i]) << " " << boost::get<2>(faces[i]) << std::endl;
	}

	// close the file
	out.close();

	// return the path to new OFF file
	return offFile;
}

// convert binary STL to OFF
char * binStl2Off(char * filename)
{
	// input variables:
	// filename - the path of file to load

	// open the file
	std::ifstream obj(filename, std::ios::in | std::ios::binary);
	// skip the header
	for (int i = 0; i < 80; i++) {
		boost::uint8_t c;
		obj.read(reinterpret_cast<char*>(&c), sizeof(c));
	}

	// read the number of facets
	boost::uint32_t N32;
	obj.read(reinterpret_cast<char*>(&N32), sizeof(N32));
	unsigned int N = N32;

	// initiate the arrays of vertices and facets
	std::vector<stl::point> points;
	std::vector<Face> faces;
	faces.reserve(N);
	int number_of_points = 0;
	int number_of_snapped_points = 0;

	// loop over facets
	for (int i = 0; i < (int)N; i++) {
		std::cout << i << std::endl;
		// initiate the new normal
		float normal[3];
		// read the floats (normal coordinates) from binary
		obj.read(reinterpret_cast<char*>(&normal[0]), sizeof(normal[0]));
		obj.read(reinterpret_cast<char*>(&normal[1]), sizeof(normal[1]));
		obj.read(reinterpret_cast<char*>(&normal[2]), sizeof(normal[2]));

		// initiate the new facet
		int index[3];
		// loop over facet's vertices
		for (int j = 0; j < 3; j++) {
			// read the floats (vertex coordinates) from binary
			float x, y, z;
			obj.read(reinterpret_cast<char*>(&x), sizeof(x));
			obj.read(reinterpret_cast<char*>(&y), sizeof(y));
			obj.read(reinterpret_cast<char*>(&z), sizeof(z));
			
			// initiate the checker of the vertex is already exist
			bool found_close_point = false;
			// loop over previous vertices
			for (int k = 0; k < points.size(); k++) {
				// check does the distance by X axis lower than eps (little constant)
				float dx = x - points[k].x;
				if (dx > -1E-5 && dx < 1E-5) {
					// check does the distance by Y axis lower than eps (little constant)
					float dy = y - points[k].y;
					if (dy > -1E-5 && dy < 1E-5) {
						// check does the distance by Z axis lower than eps (little constant)
						float dz = z - points[k].z;
						if (dz > -1E-5 && dz < 1E-5) {
							// the current vertex is close to existing one, set it into facet
							index[j] = k;
							found_close_point = true;
							number_of_snapped_points++;
							break;
						}
					}
				}
			}
			
			// the current vertex is not close to any existing
			if (!found_close_point) {
				// create new vertex
				stl::point p(x, y, z);
				// add it to array
				points.push_back(p);
				// set the index of new vertex into facet
				index[j] = number_of_points;
				number_of_points++;
			}
		}

		// add new facet into array
		faces.push_back(boost::make_tuple(index[0], index[1], index[2]));
		// skip the end of facet
		char c;
		obj.read(reinterpret_cast<char*>(&c), sizeof(c));
		obj.read(reinterpret_cast<char*>(&c), sizeof(c));
	}

	// construct the filename of new OFF file
	int offLen = (int)std::strlen(filename) + 1;
	char* offFile = new char[offLen];
	strncpy_s(offFile, offLen, filename, offLen - 4);
	strcat_s(offFile, offLen, "off");

	// open OFF file
	std::ofstream out;
	out.open(offFile);
	// output the header of OFF
	out << "OFF\n" << points.size() << " " << faces.size() << " 0" << std::endl << std::endl;

	// loop over the vertices
	for (int i = 0; i < points.size(); i++) {
		// output vertex
		out << points[i] << std::endl;
	}

	// loop over the facets
	for (int i = 0; i < faces.size(); i++) {
		// output facet
		out << "3 " << boost::get<0>(faces[i]) << " " << boost::get<1>(faces[i]) << " " << boost::get<2>(faces[i]) << std::endl;
	}

	// close the file
	out.close();

	// return the path to new OFF file
	return offFile;
}

// convert the arrays into CGAL mesh object
Polyhedron arrToMesh(std::vector<double> coords, std::vector<int> tris)
{
	// input variables:
	// coords - the array of vertices coordinates
	// tris - the array of facets vertices

	Polyhedron P;
	// create the mesh builder
	polyhedron_builder<HalfedgeDS> builder(coords, tris);
	// construct the mesh
	P.delegate(builder);

	return P;
}

// save CGAL mesh to OFF file
void meshToOff(char * filename, Polyhedron P)
{
	// input variables:
	// filename - the path to save
	// P - the CGAL mesh object

	// open the file
	std::ofstream os(filename);
	// output the mesh
	os << P;
	// close the file
	os.close();
}

// create the segmentation of skeleton
std::vector<int> SkelSeg(uint32_t vsize, uint32_t esize, uint32_t * edgeArr, int32_t * segArr)
{
	// input variables:
	// vsize - the number of skeleton's vertices
	// esize - the number of skeleton's edges
	// edgeArr - the array of skeleton's edges
	// segArr - the array of skeleton's segments

	// initiate the array of unjoined groups of segments
	std::vector<int> group;
	uint32_t i;

	// define the buffer of non-processed vertices
	std::vector<std::array<uint32_t, 3>> buf;
	// define the buffer of neighboring vertices
	std::vector<uint32_t> next;
	// initiate the counters of segments and unjoined groups
	int maxSeg = 0;
	int maxgroup = 0;

	// loop over skeleton's vertices (which segments are processed)
	int restpoint = vsize;
	while (restpoint > 0) {

		// loop over skeleton's vertices
		for (i = 0; i < vsize; i++) {
			// initiate the buffer of neighboring vertices
			buf.clear();
			// if the current vertex is not processed
			if (segArr[i] == -1) {
				// loop over skeleton's edges
				for (uint32_t j = 0; j < esize * 2; j++)
					// if the edge include the current vertex
					if (edgeArr[2 * j] == i)
						// add neighboring vertex to buffer
						buf.push_back({ edgeArr[2 * j + 1], i, 0 });
				
				// choose as start point the any not a simple medium vertex 
				if (buf.size() != 2) break;
			}
		}

		// create new group
		group.push_back(maxgroup);
		// create the new segment and set it to start vertex
		segArr[i] = maxSeg++;
		// set the start vertex as processed
		restpoint--;

		// neighbors of start vertex will be used as initial set of non-processed vertices
		// in a case of branching vertex
		if (buf.size() > 2)
			// loop over neighboring vertices (exclude the last one)
			for (uint32_t j = 0; j < buf.size() - 1; j++)
				// set the 'new segment' property
				buf[j][2] = 1;

		// loop while we have non-processed vertices (joined to start)
		while (buf.size() > 0) {
			// get the last vertex from buffer
			uint32_t cur = buf.back()[0];
			// if it has no segment yet
			if (segArr[cur] == -1) {
				// set the current vertex as processed
				restpoint--;

				// get the previous vertex (by moving direction)
				uint32_t prev = buf.back()[1];
				// initiate the 'new branch' property as false
				uint32_t tp = 0;

				// if 'new branch' is false
				if (buf.back()[2] == 0) {
					// set the segment of current vertex the same as of previous
					segArr[cur] = segArr[prev];
				}
				// 'new branch' is true
				else {
					// create new segment and set it to current branch
					group.push_back(maxgroup);
					segArr[cur] = maxSeg++;
				}
				
				// remove the current vertex from buffer
				buf.pop_back();
				// clear the buffer of neighboring vertices
				next.clear();

				// loop over skeleton's edges 
				for (uint32_t j = 0; j < esize * 2; j++)
					// if the edge include the current vertex
					if (edgeArr[2 * j] == cur) {
						// if the neighboring vertex is non-processed
						if (segArr[edgeArr[2 * j + 1]] == -1)
							// add the neighboring vertex to buffer
							next.push_back(edgeArr[2 * j + 1]);
					}

				// if we have more than one neighbors then set the 'new branch' property to true
				if (next.size() > 1) tp = 1;

				// loop over neighbors
				for (uint32_t j = 0; j < next.size(); j++)
					// add neighbor to the buffer of non-processed vertices
					buf.push_back({ next[j], cur, tp });
			}
			
			// vertex is already processed, just remove it from the buffer
			else buf.pop_back();
		}
		
		// the current group was ended, go the next one 
		maxgroup++;
	}
	return group;
}

// check has the skeleton a loop which include given segment
int isLoop(uint32_t start, uint32_t vsize, uint32_t esize, uint32_t * edgeArr, int32_t * segArr)
{
	// input variables:
	// start - the start point of given segment
	// vsize - the number of skeleton's vertices
	// esize - the number of skeleton's edges
	// edgeArr - the array of skeleton's edges
	// segArr - the array of skeleton's segments

	// initiate the array of indices (distances from given segment)
	GLuint * IndexMap = new GLuint[vsize];

	// loop over skeleton's vertices
	for (uint32_t i = 0; i < vsize; i++)
		// set zero value to given segment's points
		if (segArr[i] == segArr[start])
			IndexMap[i] = 0;
		// and -1 for others
		else
			IndexMap[i] = -1;

	// initiate the buffer for processing the points
	std::vector<int> buf;
	buf.push_back(start);
	// get the number of given segment
	int seg = segArr[start];
	
	// loop over skeleton's vertices while we still have points to process
	while (buf.size() > 0)
	{
		// get the current point
		uint32_t cur = buf.back();
		// remove it from buffer
		buf.pop_back();

		// loop over skeleton's edges
		for (uint32_t i = 0; i < esize * 2; i++)
			// if the edge include the current point
			if (edgeArr[2 * i] == cur) {
				// check do we reach the point of given segment which is differ from start point
				if (segArr[cur] != seg && segArr[edgeArr[2 * i + 1]] == seg && edgeArr[2 * i + 1] != start)
					// return the end point (start-end branch is a part of loop)
					return edgeArr[2 * i + 1];
				// check was the distance set for the next point (by the edge)
				if (IndexMap[edgeArr[2 * i + 1]] == -1) {
					// set the distance
					IndexMap[edgeArr[2 * i + 1]] = IndexMap[cur] + 1;
					// add next point to buffer
					buf.push_back(edgeArr[2 * i + 1]);
				}
			}
	}
	
	// the loop doesn't exist
	return -1;
}

// search for the neighboring point of given one which has the same segment
int nearSkel(uint32_t start, uint32_t esize, uint32_t * edgeArr, int32_t * segArr)
{
	// input variables:
	// start - the start point of given segment
	// esize - the number of skeleton's edges
	// edgeArr - the array of skeleton's edges
	// segArr - the array of skeleton's segments

	// get the number of given segment
	int32_t seg = segArr[start];

	// loop over skeleton's edges
	for (uint32_t i = 0; i < esize * 2; i++)
	{
		// check the edge include start point and other point is of the same segment
		if (edgeArr[2 * i] == start && segArr[edgeArr[2 * i + 1]] == seg)
			// return the index of neighboring point
			return edgeArr[2 * i + 1];
	}

	// neighboring point don't exist
	return -1;
}

// search for the 'multi-threads' segments of skeleton
std::vector<uint32_t> GetSeg3(uint32_t vsize, uint32_t esize, uint32_t * edgeArr, int32_t * segArr)
{
	// input variables:
	// vsize - the number of skeleton's vertices
	// esize - the number of skeleton's edges
	// edgeArr - the array of skeleton's edges
	// segArr - the array of skeleton's segments

	// initiate the array of 'multi-threads' segments
	std::vector<std::array<uint32_t, 2>> segs;
	// initiate the set of properties of chosen segment
	std::vector<uint32_t> result;
	
	// loop over skeleton's vertices
	uint32_t i;
	for (i = 0; i < vsize; i++)
	{
		// initiate the counter of neighboring vertices
		int count = 0;
		// loop over skeleton's edges
		for (uint32_t j = 0; j < esize * 2; j++) {
			// if the edge include the current vertex increase the counter
			if (edgeArr[2 * j] == i) count++;
		}
		
		// if the vertex has more than two neighbor then it can be a start of 'multi-threads' branch
		if (count > 2) segs.push_back({ (uint32_t)segArr[i], i });
	}

	// if we have no any 'multi-threads' segment return error
	if (segs.size() == 0) return result;

	// loop over the segments
	for (int s = 0; s < segs.size(); s++)
	{
		// define the start vertex of segment 
		int start = segs[s][1];
		// try to find the end of segment (and check it's a part of loop - to be 'multi-threads')
		int end = isLoop(start, vsize, esize, edgeArr, segArr);
		// if we found the end
		if (end > -1)
		{
			// set all the parameters of segment
			result.push_back(segArr[start]);							// segment index
			result.push_back(start);									// start vertex
			result.push_back(end);										// end vertex
			result.push_back(nearSkel(end, esize, edgeArr, segArr));	// end's neighboring vertex (of other segment)
			return result;
		}
	}
	return result;
}

// search for the 'outer' segments of skeleton
std::array<int32_t, 3> GetSeg(uint32_t vsize, uint32_t esize, uint32_t * edgeArr, int32_t * segArr)
{
	// input variables:
	// vsize - the number of skeleton's vertices
	// esize - the number of skeleton's edges
	// edgeArr - the array of skeleton's edges
	// segArr - the array of skeleton's segments

	// initiate the array of 'outer' segments
	std::vector<std::array<int32_t, 3>> segs;

	// loop over skeleton's vertices
	uint32_t i;
	for (i = 0; i < vsize; i++)	{
		// initiate the counter of neighboring vertices	
		int count = 0;
		// loop over skeleton's edges
		for (uint32_t j = 0; j < esize * 2; j++)
			// if the edge include the current vertex increase the counter
			if (edgeArr[2 * j] == i) count++;

		// if the vertex has only single neighbor then it's a start of 'outer' branch
		if (count == 1) segs.push_back({ segArr[i], 0, 0 });
	}

	// if we have no any 'outer' segment
	if (segs.size() == 0)	{
		// return error
		segs.push_back({ -1, 0, 0 });
		return segs[0];
	}

	// loop over skeleton's vertices
	for (i = 0; i < vsize; i++)
		// loop over the segments
		for (uint32_t j = 0; j < segs.size(); j++)
			// if the current vertex has the same segment then increase the counter
			if (segArr[i] == segs[j][0]) segs[j][2]++;
	
	// loop over skeleton's edges
	for (uint32_t j = 0; j < esize * 2; j++)
		// loop over the segments
		for (int s = 0; s < segs.size(); s++)
			// if current vertex has the same segment but its neighbor has different one
			if (segArr[edgeArr[2 * j]] == segs[s][0] && segArr[edgeArr[2 * j + 1]] != segs[s][0])
				// save it as the end point of segment
				segs[s][1] = edgeArr[2 * j];

	// initiate the maximum length of 'outer' segment
	uint32_t maxSeg = 0;
	// loop over 'outer' segments
	for (uint32_t j = 1; j < segs.size(); j++)
		// check do the current segment longer than current maximum
		if (segs[maxSeg][2] < segs[j][2]) maxSeg = j;
	
	// return the longest 'outer' segment
	return segs[maxSeg];
}

// Merge the set of CGAL mesh objects into the single one 
Polyhedron mergePoly(std::vector<Polyhedron> P, int sp, int lp)
{
	// input variables:
	// P - the array of CGAL mesh objects to be merged
	// sp - the start index of object to be merged, included
	// lp - the end index of object to be merged, excluded
	
	// initiate the arrays
	std::vector<double> coords;		// vertices coordinates
	std::vector<int> tris;			// facets
	
	// initiate the number of vertices
	int vmesh = 0;
	// define the limits of index
	int s = sp;
	if (s < 0) s = 0;
	int l = lp;
	if (l > (int)P.size()) l = (int)P.size();

	// loop over input objects
	for (int pi = s; pi < l; pi++) {
		// loop over vertices of current object
		for (auto it = P[pi].vertices_begin(); it != P[pi].vertices_end(); it++) {
			auto p = it->point();
			// add coordinates to array
			coords.push_back(p.x());
			coords.push_back(p.y());
			coords.push_back(p.z());
		}

		// loop over facets of current object
		for (Facet_iterator fi = P[pi].facets_begin(); fi != P[pi].facets_end(); ++fi) {
			Halfedge_facet_circulator fj = fi->facet_begin();
			CGAL_assertion(CGAL::circulator_size(fj) >= 3);
			do
				// add to array the vertex index shifted by the total number of vertices into previous objects
				tris.push_back(std::distance(P[pi].vertices_begin(), fj->vertex()) + vmesh);
			while (++fj != fi->facet_begin());
		}

		// add the number of vertices to total
		vmesh += (int)P[pi].size_of_vertices();
	}

	// return the new CGAL mesh (converted from the arrays)
	return arrToMesh(coords, tris);
}