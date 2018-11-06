#pragma once

// the constants of STL file types
#define STL_INVALID -1
#define STL_BINARY 0
#define STL_ASCII 1

#include "scene3D.h"

// define the most used types
typedef boost::tuple<int, int, int> Face;
typedef CGAL::Simple_cartesian<double> K;
typedef K::Point_3 Point_3;

int getStlFileFormat(const QString &path);
char * ascStl2Off(char * filename);
char * binStl2Off(char * filename);
Polyhedron arrToMesh(std::vector<double> coords, std::vector<int> tris);
void meshToOff(char * filename, Polyhedron P);
std::vector<int> SkelSeg(uint32_t vsize, uint32_t esize, uint32_t * edgeArr, int32_t * segArr);
int isLoop(uint32_t start, uint32_t vsize, uint32_t esize, uint32_t * edgeArr, int32_t * segArr);
int nearSkel(uint32_t start, uint32_t esize, uint32_t * edgeArr, int32_t * segArr);
std::vector<uint32_t> GetSeg3(uint32_t vsize, uint32_t esize, uint32_t * edgeArr, int32_t * segArr);
std::array<int32_t, 3> GetSeg(uint32_t vsize, uint32_t esize, uint32_t * edgeArr, int32_t * segArr);
Polyhedron mergePoly(std::vector<Polyhedron> P, int sp, int lp);