#include <fstream>
#include <vector>
#include <QtGui> 
#include <QtWidgets>
#include "scene3D.h"
#include "functions.h"
#include "mainwindow.h"

// define the most used CGAL types
typedef CGAL::Mean_curvature_flow_skeletonization<Polyhedron>			Skeletonization;
typedef Skeletonization::Skeleton										Skeleton;
typedef Skeleton::vertex_descriptor										Skeleton_vertex;
typedef Skeleton::edge_descriptor										Skeleton_edge;

// Initiation of Scene3D object
Scene3D::Scene3D(QWidget* parent) : QGLWidget(parent)
{
	// default values of parameters
	xRot = -90; yRot = 0; zRot = 0; zTra = 0; nSca = 1;	vmesh = 0; vskel = 0; vpart = 0;
}

// Initiation of OpenGL
void Scene3D::initializeGL()
{
	// set the background color
	qglClearColor(Qt::white);
	// to hide the covered deeper objects
	glEnable(GL_DEPTH_TEST);
	// disable the shade
	glShadeModel(GL_FLAT);
	// to use the blended colors
	glEnable(GL_BLEND);
	// enable the transparency
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// calculate the aspect ratio
	getAsp();
	// calculate the mesh arrays
	getMesh();
	// calculate the parts arrays
	getParts();
	// to use the arrays of vertices for drawing
	glEnableClientState(GL_VERTEX_ARRAY);
	// to use the arrays of colors for drawing
	glEnableClientState(GL_COLOR_ARRAY);
}

// Recalculate the scene parameters after the window will resized
void Scene3D::resizeGL(int nWidth, int nHeight)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// choose the most problem direction
	GLfloat ratio = (GLfloat)nHeight / (GLfloat)nWidth;
	if (nWidth >= nHeight)
		// change the matrix using horizontal ratio
		glOrtho(-1.0 / ratio, 1.0 / ratio, -1.0, 1.0, -10.0, 1.0);
	else
		// change the matrix using vertical ratio
		glOrtho(-1.0, 1.0, -1.0*ratio, 1.0*ratio, -10.0, 1.0);
	// transform to window coordinates
	glViewport(0, 0, (GLint)nWidth, (GLint)nHeight);
}

// Draw the scene
void Scene3D::paintGL()
{
	// set the initial parameters
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	// reset the transformation matrix to identity
	glLoadIdentity();
	// apply the scale
	glScalef(nSca, nSca, nSca);
	// apply the translation
	glTranslatef(0.0f, zTra, 0.0f);
	// apply the rotations
	glRotatef(xRot, 1.0f, 0.0f, 0.0f);
	glRotatef(yRot, 0.0f, 1.0f, 0.0f);
	glRotatef(zRot, 0.0f, 0.0f, 1.0f);

	// draw the elements using the 'elements visibility' variable
	if (showElem & shAxis) drawAxis();
	if (showElem & shSkeleton) drawSkeleton();
	if (showElem & shWireframe) drawWireframe();
	if ((showElem & shWireframe) && (showElem & shParts)) drawPWireframe();
	if (showElem & shFacets) drawFacets();
	if (showElem & shParts) drawParts();
}

// Set the initial position of actions doing by mouse
void Scene3D::mousePressEvent(QMouseEvent* pe)
{
	// save the mouse position
	ptrMousePosition = pe->pos();
}

// Event don't used; it was created for consistent purpose
void Scene3D::mouseReleaseEvent(QMouseEvent* pe)
{
	//
}

// Process rotation by mouse
void Scene3D::mouseMoveEvent(QMouseEvent* pe)
{
	// calculate rotation by X axis
	xRot += 180 / nSca * (GLfloat)(pe->y() - ptrMousePosition.y()) / height();
	// calculate rotation by Z axis
	zRot += 180 / nSca * (GLfloat)(pe->x() - ptrMousePosition.x()) / width();
	// save the mouse position
	ptrMousePosition = pe->pos();
	// draw the scene
	updateGL();
}

// Process zoom by mouse
void Scene3D::wheelEvent(QWheelEvent* pe)
{
	// choose the scale method
	if ((pe->delta()) > 0) scale_plus(); else if ((pe->delta()) < 0) scale_minus();
	// draw the scene
	updateGL();
}

// The controls actions of the scene
// zoom
void Scene3D::scale_plus() { nSca = nSca * 1.1; }
void Scene3D::scale_minus() { nSca = nSca/1.1; }
// rotation
void Scene3D::rotate_up() { xRot += 1.0f; }
void Scene3D::rotate_down() { xRot -= 1.0f; }
void Scene3D::rotate_left() { zRot += 1.0f; }
void Scene3D::rotate_right() { zRot -= 1.0f; }
// translation
void Scene3D::translate_down() { zTra -= 0.05f; }
void Scene3D::translate_up() { zTra += 0.05f; }
// reset to default
void Scene3D::defaultScene() { xRot=-90; yRot=0; zRot=0; zTra=0; nSca=1; }

// Draw the axis
void Scene3D::drawAxis() 
{
	// set the line width
	glLineWidth(3.0f);
	// set the mode to 'lines'
	glBegin(GL_LINES);
	
	// the X axis
	glColor4f(1.00f, 0.00f, 0.00f, 1.0f);
	glVertex3f(1.0f, 0.0f, 0.0f);
	glVertex3f(-1.0f, 0.0f, 0.0f);
	// the Y axis
	glColor4f(0.00f, 1.00f, 0.00f, 1.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.0f, -1.0f, 0.0f);
	// the Z axis
	glColor4f(0.00f, 0.00f, 1.00f, 1.0f);
	glVertex3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, -1.0f);
	
	// end of the 'lines' mode
	glEnd();
}

// Calculate the aspect ratio of given mesh
void Scene3D::getAsp()
{
	// initiate the ratio and center point
	asp = 1;
	cx = 0;
	cy = 0;
	cz = 0;

	// initiate the total number of vertices of all parts
	vmesh = 0;
	// if we have no parts return
	int lmesh = (int)tmesh.size();
	if (lmesh == 0) return;

	// loop over the parts
	for (int p = 0; p < tmesh.size(); p++)
		// add the number of vertices
		vmesh += (int)tmesh[p].size_of_vertices();
	// if we have no vertices return
	if (vmesh == 0) return;

	// initiate the maximum and minimum values of X, Y and Z
	int i = 0;
	float mx, my, mz;
	float Mx, My, Mz;
	// loop over the parts
	for (int p = 0; p < lmesh; p++) {
		// loop over the vertices
		for (auto it = tmesh[p].vertices_begin(); it != tmesh[p].vertices_end(); it++) {
			// check and replace the maximum and minimum values of X, Y and Z
			auto p = it->point();
			if (i == 0) { mx = p.x(); Mx = p.x(); }
			if (mx > p.x()) mx = p.x();
			if (Mx < p.x()) Mx = p.x();
			if (i == 0) { my = p.y(); My = p.y(); }
			if (my > p.y()) my = p.y();
			if (My < p.y()) My = p.y();
			if (i == 0) { mz = p.z(); Mz = p.z(); }
			if (mz > p.z()) mz = p.z();
			if (Mz < p.z()) Mz = p.z();
			i++;
		}
	}

	// define the middle point
	cx = (mx + Mx) / 2;
	cy = (my + My) / 2;
	cz = (mz + Mz) / 2;

	// calculate the lowest ratio
	if (Mx != mx) asp = 1 / (Mx - mx);
	if (My != my) { if (1 / (My - my) < asp) asp = 1 / (My - my); }
	if (Mz != mz) { if (1 / (Mz - mz) < asp) asp = 1 / (Mz - mz); }
}

// Calculate the arrays of mesh to use OpenGL (vertices, colors, facets, edges)
void Scene3D::getMesh()
{
	// check we have a correct mesh
	vmesh = 0;
	int lmesh = (int)tmesh.size() - 1;
	if (lmesh < 0) return;
	if (tmesh[lmesh].size_of_vertices() == 0) return;

	// get the number of facets and vertices
	vmesh = (int)tmesh[lmesh].size_of_vertices();
	fmesh = (int)tmesh[lmesh].size_of_facets();
	
	// resize the arrays
	MeshVertex = new GLfloat[3 * vmesh];
	MeshColor = new GLfloat[4 * vmesh];
	MeshIndex = new GLuint[3 * fmesh];
	EdgeColor = new GLfloat[4 * vmesh];
	EdgeIndex = new GLuint[6 * fmesh];
	SkelMapMesh = new GLuint[vmesh];

	// loop over vertices
	int i = 0;
	for (auto it = tmesh[lmesh].vertices_begin(); it != tmesh[lmesh].vertices_end(); it++, i++) {
		auto p = it->point();
		it->id() = i;
		// add vertex coordinates to array
		MeshVertex[3 * i] = (p.x() - cx) * asp;
		MeshVertex[3 * i + 1] = (p.y() - cy) * asp;
		MeshVertex[3 * i + 2] = (p.z() - cz) * asp;
		
		// add vertex colors to array of facets colors
		MeshColor[4 * i] = 0.5f;
		MeshColor[4 * i + 1] = 0.7f;
		MeshColor[4 * i + 2] = 0.5f;
		MeshColor[4 * i + 3] = 0.3f;

		// add vertex color to array of edges colors
		EdgeColor[4 * i] = 0.25f;
		EdgeColor[4 * i + 1] = 0.0f;
		EdgeColor[4 * i + 2] = 0.0f;
		EdgeColor[4 * i + 3] = 1.0f;
	}

	// loop over facets
	i = 0;
	for (Facet_iterator fi = tmesh[lmesh].facets_begin(); fi != tmesh[lmesh].facets_end(); ++fi, i++)	{
		// get the facet's vertices enumerator
		Halfedge_facet_circulator fj = fi->facet_begin();
		CGAL_assertion(CGAL::circulator_size(fj) >= 3);
		// loop over facet's vertices
		int j = 0;
		do 
			// add vertices index to array
			MeshIndex[3 * i + j++] = std::distance(tmesh[lmesh].vertices_begin(), fj->vertex());
		while (++fj != fi->facet_begin());
	}
	// loop over facets
	for (i = 0; i < fmesh; i++) {
		// add the edges to array
		EdgeIndex[6 * i] = MeshIndex[3 * i];
		EdgeIndex[6 * i + 1] = MeshIndex[3 * i + 1];
		EdgeIndex[6 * i + 2] = MeshIndex[3 * i + 1];
		EdgeIndex[6 * i + 3] = MeshIndex[3 * i + 2];
		EdgeIndex[6 * i + 4] = MeshIndex[3 * i + 2];
		EdgeIndex[6 * i + 5] = MeshIndex[3 * i];
	}
}

// Calculate the arrays of mesh to use OpenGL (vertices, colors, facets, edges)
void Scene3D::getParts()
{
	// check do we have the correct parts
	vpart = 0;
	int lmesh = (int)tmesh.size() - 1;
	if (lmesh < 1) return;
	for (int p = 0; p < lmesh; p++)
		vpart += (int)tmesh[p].size_of_vertices();
	if (vpart == 0) return;

	// create the common mesh from the parts
	Polyhedron mesh = mergePoly(tmesh, 0, lmesh);

	// get the number of facets and vertices
	vpart = (int)mesh.size_of_vertices();
	fpart = (int)mesh.size_of_facets();

	// resize the arrays
	PartVertex = new GLfloat[3 * vpart];
	PartColor = new GLfloat[4 * vpart];
	PartIndex = new GLuint[3 * fpart];
	PEdgeColor = new GLfloat[4 * vpart];
	PEdgeIndex = new GLuint[6 * fpart];

	// loop over vertices
	int i = 0;
	for (auto it = mesh.vertices_begin(); it != mesh.vertices_end(); it++, i++) {
		auto p = it->point();
		it->id() = i;
		
		// add vertex coordinates to array
		PartVertex[3 * i] = (p.x() - cx) * asp;
		PartVertex[3 * i + 1] = (p.y() - cy) * asp;
		PartVertex[3 * i + 2] = (p.z() - cz) * asp;
		
		// add vertex colors to array of facet's colors
		PartColor[4 * i] = 0.7f;
		PartColor[4 * i + 1] = 0.5f;
		PartColor[4 * i + 2] = 0.5f;
		PartColor[4 * i + 3] = 0.3f;
		
		// add vertex colors to array of edge's colors
		PEdgeColor[4 * i] = 0.0f;
		PEdgeColor[4 * i + 1] = 0.0f;
		PEdgeColor[4 * i + 2] = 0.25f;
		PEdgeColor[4 * i + 3] = 1.0f;
	}
	
	// loop over facets
	i = 0;
	for (Facet_iterator fi = mesh.facets_begin(); fi != mesh.facets_end(); ++fi, i++) {
		// get the facet's vertices enumerator
		Halfedge_facet_circulator fj = fi->facet_begin();
		CGAL_assertion(CGAL::circulator_size(fj) >= 3);
		// loop over facet's vertices
		int j = 0;
		do
			// add vertex indices to array
			PartIndex[3 * i + j++] = std::distance(mesh.vertices_begin(), fj->vertex());
		while (++fj != fi->facet_begin());
	}
	// loop over facets
	for (i = 0; i < fpart; i++) {
		// add the edges to array
		PEdgeIndex[6 * i] = PartIndex[3 * i];
		PEdgeIndex[6 * i + 1] = PartIndex[3 * i + 1];
		PEdgeIndex[6 * i + 2] = PartIndex[3 * i + 1];
		PEdgeIndex[6 * i + 3] = PartIndex[3 * i + 2];
		PEdgeIndex[6 * i + 4] = PartIndex[3 * i + 2];
		PEdgeIndex[6 * i + 5] = PartIndex[3 * i];
	}
}

// Create the CGAL skeleton of the main segment and calculate its arrays
int Scene3D::getSkeleton()
{
	// check do we have the correct main part
	int lmesh = (int)tmesh.size() - 1;
	if (lmesh < 0) return 0;
	if (vmesh == 0) return 0;
	if (tmesh[lmesh].size_of_vertices() == 0) return 0;
	
	// calculate skeleton using CGAL
	Skeleton skeleton;
	CGAL::extract_mean_curvature_flow_skeleton(tmesh[lmesh], skeleton);

	// get the number of facets and vertices
	vskel = (int)boost::num_vertices(skeleton);
	eskel = (int)boost::num_edges(skeleton);

	// resize the arrays
	SkelVertex = new GLfloat[3 * vskel];
	SkelColor = new GLfloat[4 * vskel];
	SkelIndex = new GLuint[2 * eskel];
	SkelSegVert = new GLint[vskel];
	SkelSegEdge = new GLuint[4 * eskel];

	// loop over skeleton's vertices
	int i = 0;
	BOOST_FOREACH(Skeleton_vertex v, vertices(skeleton)) {
		// loop over joined solid's vertices
		BOOST_FOREACH(vertex_descriptor vd, skeleton[v].vertices)
			// add relation to array
			SkelMapMesh[vd->id()] = (GLuint)v;
		
		// add vertex coordinates to array
		SkelVertex[3 * i] = (skeleton[v].point.x() - cx) * asp;
		SkelVertex[3 * i + 1] = (skeleton[v].point.y() - cy) * asp;
		SkelVertex[3 * i + 2] = (skeleton[v].point.z() - cz) * asp;
		
		// add vertex colors to array of edge's colors
		SkelColor[4 * i] = 0.0f;
		SkelColor[4 * i + 1] = 0.0f;
		SkelColor[4 * i + 2] = 0.75f;
		SkelColor[4 * i + 3] = 1.00f;
		SkelSegVert[i] = -1;
		i++;
	}

	// loop over skeleton's edges
	i = 0;
	BOOST_FOREACH(Skeleton_edge e, edges(skeleton)) {
		// get the indices of the start and end points
		GLuint s = (GLuint)e.m_source;
		GLuint t = (GLuint)e.m_target;

		// add vertex indices to arrays
		SkelIndex[2 * i] = s;
		SkelIndex[2 * i + 1] = t;
		SkelSegEdge[4 * i] = s;
		SkelSegEdge[4 * i + 1] = t;
		SkelSegEdge[4 * i + 2] = t;
		SkelSegEdge[4 * i + 3] = s;
		i++;
	}

	// prepare the segmentation of skeleton
	SegGroups = SkelSeg(vskel, eskel, SkelSegEdge, SkelSegVert);
	// get the number of unjoined skeleton's parts
	int maxSeg = (int)SegGroups.size();
	maxgroup = SegGroups[maxSeg - 1];
	// loop over unjoined parts; try to find the parts which can be joined
	while (maxgroup > 0)
	{
		// initiate the shortest edge's index and length
		int bestEdge = -1;
		double bestDist = 100;

		// loop over solid's edges
		for (i = 0; i < 3 * fmesh; i++) {
			// get the unjoined part index of start and end of the edge
			int grS = SegGroups[SkelSegVert[SkelMapMesh[EdgeIndex[2 * i]]]];
			int grT = SegGroups[SkelSegVert[SkelMapMesh[EdgeIndex[2 * i + 1]]]];
			
			// if they are different
			if (grS != grT) {
				// calculate the length of the edge
				double dx = MeshVertex[3 * EdgeIndex[2 * i]] - MeshVertex[3 * EdgeIndex[2 * i + 1]];
				double dy = MeshVertex[3 * EdgeIndex[2 * i] + 1] - MeshVertex[3 * EdgeIndex[2 * i + 1] + 1];
				double dz = MeshVertex[3 * EdgeIndex[2 * i] + 2] - MeshVertex[3 * EdgeIndex[2 * i + 1] + 2];
				double curDist = dx * dx + dy * dy + dz * dz;
				
				// if this edge is shorter than the previous best
				if (curDist < bestDist) {
					// change the index and length of the shortest edge
					bestDist = curDist;
					bestEdge = i;
				}
			}
		}

		// we have no possibility to join some pair of parts
		if (bestEdge == -1) break;

		// get the unjoined part index of start and end of the best edge
		int grL = SegGroups[SkelSegVert[SkelMapMesh[EdgeIndex[2 * bestEdge]]]];
		int grH = SegGroups[SkelSegVert[SkelMapMesh[EdgeIndex[2 * bestEdge + 1]]]];
		// use the higher index
		if (grL > grH) grH = grL;

		// add the new sceleton's edge
		eskel++;
		SkelSegEdge = (GLuint *)realloc(SkelSegEdge, 4 * eskel * sizeof(GLuint));
		SkelSegEdge[4 * eskel - 4] = SkelMapMesh[EdgeIndex[2 * bestEdge]];
		SkelSegEdge[4 * eskel - 3] = SkelMapMesh[EdgeIndex[2 * bestEdge + 1]];
		SkelSegEdge[4 * eskel - 2] = SkelMapMesh[EdgeIndex[2 * bestEdge + 1]];
		SkelSegEdge[4 * eskel - 1] = SkelMapMesh[EdgeIndex[2 * bestEdge]];
		
		// loop over unjoined parts
		for (i = 0; i < maxSeg; i++)
			// shift all the indices higher than cutted
			if (SegGroups[i] >= grH) SegGroups[i]--;
		maxgroup--;
	}
	
	// we still have unjoined parts
	if (maxgroup > 0)
	{
		// create the error message
		std::stringstream ss;
		ss << "The model consist of " << maxgroup + 1 << " solids.";
		// show the message
		QMessageBox msgBox;
		msgBox.setText(QString::fromStdString(ss.str()));
		msgBox.setInformativeText("Do you want to save them as separate OFF files?");
		msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Save);
		int ret = msgBox.exec();
		
		// if user choose the 'Save' then save unjoined parts as separate OFF files
		if (ret == QMessageBox::Save) groupsToOff();
	}
	
	// return the number of unjoined parts: zero is normal return
	return maxgroup;
}

// Extract the new segment from solid
void Scene3D::newPart()
{
	// get the longest 'output branch' of skeleton
	std::array<int32_t, 3> segs = GetSeg(vskel, eskel, SkelSegEdge, SkelSegVert);
	// check do we have correct branch
	if (segs[0] >= 0 && segs[2] > 2) {
		
		// initiate the arrays of new extracted part and of the rest of solid
		std::vector<int> pVert;			// part's vertices (relation to old indices)
		std::vector<double> pCoords;	// part's vertices coordinates
		std::vector<int> pTris;			// part's facets
		std::vector<int> mVert;			// main vertices (relation to old indices)
		std::vector<double> mCoords;	// main vertices coordinates
		std::vector<int> mTris;			// main facets

		// loop over solid's vertices  + new vertex from skeleton
		for (int j = 0; j < vmesh + 1; j++) {
			// initiate the arrays of indices relations 
			pVert.push_back(-1);
			mVert.push_back(-1);
		}
		// create the new vertex from last vertex of skeleton's branch which will be used to cut solid
		for (int v = 0; v < 3; v++) {
			// add it to both part's and main arrays
			pCoords.push_back(SkelVertex[3 * segs[1] + v]);
			mCoords.push_back(SkelVertex[3 * segs[1] + v]);
		}
		// initiate the counters of number of vertices (part's and main)
		int pvert = 1;
		int mvert = 1;

		// loop over solid's facets
		for (int j = 0; j < fmesh; j++)
		{
			// initiate the checkers
			int isSeg = 0;		// number of vertices which is of new part
			int notSeg = -1;	// index of vertex which is not of the new part

			// loop over facet's vertices
			for (int v = 0; v < 3; v++) {
				// set the checkers
				if (SkelSegVert[SkelMapMesh[MeshIndex[3 * j + v]]] == segs[0]) isSeg++;
				else notSeg = v;
			}
			
			// all the vertices is of the new part
			if (isSeg == 3) {
				// loop over facet's vertices
				for (int v = 0; v < 3; v++) {
					// get the index of current vertex
					int vn = MeshIndex[3 * j + v];
					// it was not saved in array yet
					if (pVert[vn] == -1)
					{
						// add the relation between solid's and part's indices
						pVert[vn] = pvert++;
						// add the coordinates to part's array
						pCoords.push_back(MeshVertex[3 * vn]);
						pCoords.push_back(MeshVertex[3 * vn + 1]);
						pCoords.push_back(MeshVertex[3 * vn + 2]);
					}
					// add the new index to part's array
					pTris.push_back(pVert[vn]);
				}
			}
			
			// the facet is on the border between part and main
			else if (isSeg == 2)
			{
				// save the existing faset to main
				// loop over facet's vertices
				for (int v = 0; v < 3; v++) {
					// get the index of current vertex
					int vn = MeshIndex[3 * j + v];
					// it was not saved in array yet
					if (mVert[vn] == -1) {
						// add the relation between solid's and main indices
						mVert[vn] = mvert++;
						// add the coordinates to main array
						mCoords.push_back(MeshVertex[3 * vn]);
						mCoords.push_back(MeshVertex[3 * vn + 1]);
						mCoords.push_back(MeshVertex[3 * vn + 2]);
					}
					// add the new index to main array
					mTris.push_back(mVert[vn]);
				}

				// create new facets to main and part
				// initiate the temporary facet
				std::array<int, 3> vr;
				// loop over facet's vertices
				for (int v = 0; v < 3; v++) {
					// get the index of current vertex
					int vn = MeshIndex[3 * j + v];

					// change the vertex which is not of new part to skeleton's vertex
					if (notSeg == v) {
						// into part's array
						pTris.push_back(0);
						// into temporary facet (for main array)
						vr[v] = 0;
					}
					// the vertices which is on new part will stay
					else {
						// it was not saved in part's array yet
						if (pVert[vn] == -1) {
							// add the relation between solid's and part's indices
							pVert[vn] = pvert++;
							// add the coordinates to part's array
							pCoords.push_back(MeshVertex[3 * vn]);
							pCoords.push_back(MeshVertex[3 * vn + 1]);
							pCoords.push_back(MeshVertex[3 * vn + 2]);
						}
						// add the new index to part's array
						pTris.push_back(pVert[vn]);
						// add the new index to temporary facet (for main array)
						vr[v] = mVert[vn];
					}
				}

				// save the temporary facet into main array in reverse order of vertices
				for (int v = 2; v >=0; v--) {
					mTris.push_back(vr[v]);
				}
			}
			
			// all the vertices is of the main
			else {
				// loop over facet's vertices
				for (int v = 0; v < 3; v++) {
					// get the index of current vertex
					int vn = MeshIndex[3 * j + v];
					// it was not saved in array yet
					if (mVert[vn] == -1) {
						// add the relation between solid's and main indices
						mVert[vn] = mvert++;
						// add the coordinates to main array
						mCoords.push_back(MeshVertex[3 * vn]);
						mCoords.push_back(MeshVertex[3 * vn + 1]);
						mCoords.push_back(MeshVertex[3 * vn + 2]);
					}
					// add the new index to main array
					mTris.push_back(mVert[vn]);
				}
			}
		}

		try {
			// create the new CGAL mesh objects of new part and main
			Polyhedron part = arrToMesh(pCoords, pTris);
			Polyhedron main = arrToMesh(mCoords, mTris);

			// exclude the previous main mesh from widget
			tmesh.pop_back();
			// add the mesh of new part
			tmesh.push_back(part);
			// add the mesh of new main
			tmesh.push_back(main);
		}
		
		// the extracted vertices and facets don't form a proper mesh
		catch (const CGAL::Assertion_exception e) {
			// create the error message
			QMessageBox msg;
			msg.setText("The part extraction was unsuccessful, please reopen the model and try again");
			msg.setStandardButtons(QMessageBox::Ok);
			msg.setDefaultButton(QMessageBox::Ok);
			msg.exec();
		}
		// set off the previous skeleton
		vskel = 0;
		// recalculate arrays for OpenGL
		getMesh();
		getParts();

		return;
	}

	// we have no 'outer' branches; try to extract the branch from 'multi-thread' construction
	std::vector<uint32_t> seg3 = GetSeg3(vskel, eskel, SkelSegEdge, SkelSegVert);
	// check do we find such one
	if (seg3.size() > 0) {

		// initiate the arrays of new extracted part and of the rest of solid
		std::vector<int> pVert;			// part's vertices (relation to old indices)
		std::vector<double> pCoords;	// part's vertices coordinates
		std::vector<int> pTris;			// part's facets
		std::vector<int> mVert;			// main vertices (relation to old indices)
		std::vector<double> mCoords;	// main vertices coordinates
		std::vector<int> mTris;			// main facets

		// loop over solid's vertices + 2 new vertices from skeleton
		for (int j = 0; j < vmesh + 2; j++) {
			// initiate the arrays of indices relations
			pVert.push_back(-1);
			mVert.push_back(-1);
		}

		// create the new vertices from out vertices of skeleton's branch which will be used to cut solid
		// the start
		for (int v = 0; v < 3; v++) {
			// add it to both part's and main arrays
			pCoords.push_back(SkelVertex[3 * seg3[1] + v]);
			mCoords.push_back(SkelVertex[3 * seg3[1] + v]);
		}
		// the end
		for (int v = 0; v < 3; v++) {
			// add it to both part's and main arrays
			pCoords.push_back(SkelVertex[3 * seg3[2] + v]);
			mCoords.push_back(SkelVertex[3 * seg3[2] + v]);
		}
		// initiate the counters of number of vertices (part's and main)
		int pvert = 2;
		int mvert = 2;
		
		// loop over solid's facets
		for (int j = 0; j < fmesh; j++)
		{
			// initiate the checkers
			int isSeg = 0;		// number of vertices which is of new part
			int notSeg = -1;	// index of vertex which is not of the new part
			int isEnd = 0;		// the start/end checker

			// loop over facet's vertices
			for (int v = 0; v < 3; v++) {
				// set the checkers
				int skl = SkelMapMesh[MeshIndex[3 * j + v]];
				if (SkelSegVert[skl] == seg3[0] && skl != seg3[1] && skl != seg3[2]) isSeg++;
				else notSeg = v;
				if (skl == seg3[2] || skl == seg3[3]) isEnd = 1;
			}

			// all the vertices is of the new part
			if (isSeg == 3) {
				// loop over facet's vertices
				for (int v = 0; v < 3; v++) {
					// get the index of current vertex
					int vn = MeshIndex[3 * j + v];
					// it was not saved in array yet
					if (pVert[vn] == -1)
					{
						// add the relation between solid's and part's indices
						pVert[vn] = pvert++;
						// add the coordinates to part's array
						pCoords.push_back(MeshVertex[3 * vn]);
						pCoords.push_back(MeshVertex[3 * vn + 1]);
						pCoords.push_back(MeshVertex[3 * vn + 2]);
					}
					// add the new index to part's array
					pTris.push_back(pVert[vn]);
				}
			}

			// the facet is on the border between part and main
			else if (isSeg == 2)
			{
				// save the existing faset to main
				// loop over facet's vertices
				for (int v = 0; v < 3; v++) {
					// get the index of current vertex
					int vn = MeshIndex[3 * j + v];
					// it was not saved in main array yet
					if (mVert[vn] == -1) {
						// add the relation between solid's and main indices
						mVert[vn] = mvert++;
						// add the coordinates to main array
						mCoords.push_back(MeshVertex[3 * vn]);
						mCoords.push_back(MeshVertex[3 * vn + 1]);
						mCoords.push_back(MeshVertex[3 * vn + 2]);
					}
					// add the new index to main array
					mTris.push_back(mVert[vn]);
				}
				
				// create new facets to main and part
				// initiate the temporary facet
				std::array<int, 3> vr;
				// loop over facet's vertices
				for (int v = 0; v < 3; v++) {
					// get the index of current vertex
					int vn = MeshIndex[3 * j + v];
					
					// change the vertex which is not of new part to skeleton's vertex
					if (notSeg == v) {
						// into part's array
						pTris.push_back(isEnd);
						// into temporary facet (for main array)
						vr[v] = isEnd;
					}
					// the vertices which is on new part will stay
					else {
						// it was not saved in part's array yet
						if (pVert[vn] == -1) {
							// add the relation between solid's and part's indices
							pVert[vn] = pvert++;
							// add the coordinates to part's array
							pCoords.push_back(MeshVertex[3 * vn]);
							pCoords.push_back(MeshVertex[3 * vn + 1]);
							pCoords.push_back(MeshVertex[3 * vn + 2]);
						}
						// add the new index to part's array
						pTris.push_back(pVert[vn]);
						// add the new index to temporary facet (for main array)
						vr[v] = mVert[vn];
					}
				}

				// save the temporary facet into main array in reverse order of vertices
				for (int v = 2; v >= 0; v--) {
					mTris.push_back(vr[v]);
				}
			}

			// all the vertices is of the main
			else {
				// loop over facet's vertices
				for (int v = 0; v < 3; v++) {
					// get the index of current vertex
					int vn = MeshIndex[3 * j + v];
					// it was not saved in array yet
					if (mVert[vn] == -1) {
						// add the relation between solid's and main indices
						mVert[vn] = mvert++;
						// add the coordinates to main array
						mCoords.push_back(MeshVertex[3 * vn]);
						mCoords.push_back(MeshVertex[3 * vn + 1]);
						mCoords.push_back(MeshVertex[3 * vn + 2]);
					}
					// add the new index to main array
					mTris.push_back(mVert[vn]);
				}
			}
		}

		try {
			// create the new CGAL mesh objects of new part and main
			Polyhedron part = arrToMesh(pCoords, pTris);
			Polyhedron main = arrToMesh(mCoords, mTris);

			// exclude the previous main mesh from widget
			tmesh.pop_back();
			// add the mesh of new part
			tmesh.push_back(part);
			// add the mesh of new main
			tmesh.push_back(main);
		}
		
		// the extracted vertices and facets don't form a proper mesh
		catch (const CGAL::Assertion_exception e) {
			// create the error message
			QMessageBox msg;
			msg.setText("The part extraction was unsuccessful, please reopen the model and try again");
			msg.setStandardButtons(QMessageBox::Ok);
			msg.setDefaultButton(QMessageBox::Ok);
			msg.exec();
		}

		// set off the previous skeleton
		vskel = 0;
		// recalculate arrays for OpenGL
		getMesh();
		getParts();

		return;
	}
	
	// we have no 'output' and 'multi-thread' branches, create the info message
	QMessageBox msg;
	msg.setText("Any acceptable type of part for extraction not found");
	msg.setStandardButtons(QMessageBox::Ok);
	msg.setDefaultButton(QMessageBox::Ok);
	msg.exec();

}

// Save the group of parts as separate OFF files
void Scene3D::groupsToOff()
{
	// get the filename using the standard Qt file dialog
	QString partName = QFileDialog::getSaveFileName(this, "Save", QDir::currentPath(), "Part (*.off)");
	// loop over the parts
	for (int part = 0; part < maxgroup + 1; part++) {
		
		// create the name of current part including the part index
		std::stringstream sc;
		sc.str(std::string());
		sc << std::setw(3) << std::setfill('0') << part << ".off";
		QString partN = QString::fromStdString(sc.str());

		// initiate the arrays 
		std::vector<int> NewVert;		// new indices of vertices
		std::vector<double> coords;		// vertices coodinates
		std::vector<int> tris;			// facets
		
		// loop over solid's vertices
		for (int i = 0; i < vmesh; i++) {
			
			// if the vertex is of target part
			if (SegGroups[SkelSegVert[SkelMapMesh[i]]] == part) {
				// add its coordinates to array
				coords.push_back(MeshVertex[3 * i]);
				coords.push_back(MeshVertex[3 * i + 1]);
				coords.push_back(MeshVertex[3 * i + 2]);
			}
			// add its new index to array
			NewVert.push_back((int)(coords.size() / 3) - 1);
		}

		// loop over facets
		for (int i = 0; i < fmesh; i++)
			// if the facet is of target part
			if (SegGroups[SkelSegVert[SkelMapMesh[MeshIndex[3 * i]]]] == part)
			{
				// add it to array
				tris.push_back(NewVert[MeshIndex[3 * i]]);
				tris.push_back(NewVert[MeshIndex[3 * i + 1]]);
				tris.push_back(NewVert[MeshIndex[3 * i + 2]]);
			}
		
		// convert the arrays into new CGAL mesh object
		Polyhedron partMesh = arrToMesh(coords, tris);
		// create the whole filename of current part
		QByteArray ba = partName.toLatin1();
		QString curName = QString(ba).replace(".off", partN);
		// save the part as OFF
		meshToOff(curName.toLatin1().data(), partMesh);
	}
}

// Draw the skeleton using Qt
void Scene3D::drawSkeleton()
{
	// check do the skeleton exist
	if (vskel == 0) return;
	// set the line width
	glLineWidth(5.0f);
	// set the vertices
	glVertexPointer(3, GL_FLOAT, 0, SkelVertex);
	// set the colors
	glColorPointer(4, GL_FLOAT, 0, SkelColor);
	// set the edges
	glDrawElements(GL_LINES, 2 * eskel, GL_UNSIGNED_INT, SkelIndex);
}

// Draw the facets of mesh
void Scene3D::drawFacets()
{
	// check do the mesh exist
	if (vmesh == 0) return;
	// set the vertices
	glVertexPointer(3, GL_FLOAT, 0, MeshVertex);
	// set the colors
	glColorPointer(4, GL_FLOAT, 0, MeshColor);
	// set the facets
	glDrawElements(GL_TRIANGLES, 3 * fmesh, GL_UNSIGNED_INT, MeshIndex);
}

// Draw the wireframe of mesh
void Scene3D::drawWireframe()
{
	// check do the mesh exist
	if (vmesh == 0) return;
	// set the line width
	glLineWidth(2.0f);
	// set the vertices
	glVertexPointer(3, GL_FLOAT, 0, MeshVertex);
	// set the colors
	glColorPointer(4, GL_FLOAT, 0, EdgeColor);
	// set the edges
	glDrawElements(GL_LINES, 6 * fmesh, GL_UNSIGNED_INT, EdgeIndex);
}

// Draw the facets of extracted segments
void Scene3D::drawParts()
{
	// check do the segments exist
	if (vpart == 0) return;
	// set the vertices
	glVertexPointer(3, GL_FLOAT, 0, PartVertex);
	// set the colors
	glColorPointer(4, GL_FLOAT, 0, PartColor);
	// set the facets
	glDrawElements(GL_TRIANGLES, 3 * fpart, GL_UNSIGNED_INT, PartIndex);
}

// Draw the edges of extracted segments
void Scene3D::drawPWireframe()
{
	// check do the segments exist
	if (vpart == 0) return;
	// set the line width
	glLineWidth(2.0f);
	// set the vertices
	glVertexPointer(3, GL_FLOAT, 0, PartVertex);
	// set the colors
	glColorPointer(4, GL_FLOAT, 0, PEdgeColor);
	// set the edges
	glDrawElements(GL_LINES, 6 * fpart, GL_UNSIGNED_INT, PEdgeIndex);
}

// Initiate the scene with a new mesh
void Scene3D::load(Polyhedron mesh) 
{
	// loop over existing parts
	for (int i = (int)tmesh.size() - 1; i >= 0; i--) {
		// delete the current part
		Polyhedron p = tmesh[i];
		tmesh.pop_back();
		delete &p;
	}
	// add the input mesh into the scene's list
	tmesh.push_back(mesh);
	// reset the key index
	vskel = 0;
	// calculate the aspect ratio
	getAsp();
	// calculate the mesh arrays
	getMesh();
}

// Add new part to existing scene
void Scene3D::add(Polyhedron mesh)
{
	// add the input mesh into the scene's list
	tmesh.push_back(mesh);
	// reset the key index
	vskel = 0;
	// calculate the aspect ratio
	getAsp();
	// calculate the mesh arrays
	getMesh();
	// calculate the parts arrays
	getParts();
}