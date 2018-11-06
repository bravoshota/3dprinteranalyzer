#ifndef SCENE3D_H
#define SCENE3D_H
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Mean_curvature_flow_skeletonization.h>
#include <CGAL/extract_mean_curvature_flow_skeleton.h>
#include <CGAL/mesh_segmentation.h>
#include <CGAL/Subdivision_method_3.h>
#include <CGAL/assertions.h>
#include <CGAL/exceptions.h>
#include <vector>
#include <QtGui> 
#include <QtWidgets>
#include <QtOpenGL/QGLWidget>

// define the most used CGAL types
typedef CGAL::Simple_cartesian<double>									Kernel;
typedef Kernel::Point_3													Point;
typedef CGAL::Polyhedron_3<Kernel, CGAL::Polyhedron_items_with_id_3>	Polyhedron;
typedef Polyhedron::Facet_iterator										Facet_iterator;
typedef Polyhedron::Halfedge_around_facet_circulator					Halfedge_facet_circulator;
typedef boost::graph_traits<Polyhedron>::vertex_descriptor				vertex_descriptor;

// Scene3D class to 3D objects visualization using Qt
class Scene3D : public QGLWidget
{
private:
	GLfloat xRot;				// the rotation angle of X axis
	GLfloat yRot;				// the rotation angle of Y axis
	GLfloat zRot;				// the rotation angle of Z axis
	GLfloat zTra;				// translation by Z axis
	GLfloat nSca;				// scale of image

	int vmesh;					// the number of mesh vertices
	int fmesh;					// the number of mesh facets
	int vpart;					// the number of parts vertices
	int fpart;					// the number of parts facets
	int vskel;					// the number of skeleton vertices
	int eskel;					// the number of skeleton edges
	int maxgroup;				// the number of skeleton's segments

	GLfloat asp;				// aspect ratio to set the maximum distance from mesh surface to origin equal to one
	// shifts to set the center of mesh on the point of origin
	GLfloat cx;					// shift by X axis
	GLfloat cy;					// shifh by Y axis
	GLfloat cz;					// shift by Z axis
	QPoint ptrMousePosition;	// the last saved mouse position

	GLfloat *MeshVertex;		// array of solid's vertices
	GLfloat *MeshColor;			// array of solid's colors of vertices (for facets)
	GLuint *MeshIndex;			// array of solid's facets
	GLfloat *EdgeColor;			// array of solid's colors of vertices (for edges)
	GLuint *EdgeIndex;			// array of solid's edges
	GLfloat *SkelVertex;		// array of skeleton's vertices
	GLfloat *SkelColor;			// array of skeleton's colors of vertices (for edges)
	GLuint *SkelIndex;			// array of skeleton's edges
	GLint *SkelSegVert;			// array of skeleton's relations vertex<->segment
	GLuint *SkelSegEdge;		// array of skeleton's edges (doubled)
	GLuint *SkelMapMesh;		// array of relations skeleton<->solid vertices
	GLfloat *PartVertex;		// array of part's vertices
	GLfloat *PartColor;			// array of part's colors of vertices (for facets)
	GLuint *PartIndex;			// array of part's facets
	GLfloat *PEdgeColor;		// array of part's colors of vertices (for edges)
	GLuint *PEdgeIndex;			// array of part's edges
	std::vector<int> SegGroups;	// array of skeleton's unjoined parts

	void scale_plus();
	void scale_minus();
	void rotate_up();
	void rotate_down();
	void rotate_left();
	void rotate_right();
	void translate_down();
	void translate_up();
	void defaultScene();

	void drawAxis();
	void getAsp();
	void getMesh();
	void addMesh(Polyhedron P);
	void drawSkeleton();
	void drawWireframe();
	void drawFacets();

protected:
	void initializeGL();
	void resizeGL(int nWidth, int nHeight);
	void paintGL();
	void mousePressEvent(QMouseEvent* pe);
	void mouseMoveEvent(QMouseEvent* pe);
	void mouseReleaseEvent(QMouseEvent* pe);
	void wheelEvent(QWheelEvent* pe);

public:
	int showElem;
	std::vector<Polyhedron> tmesh;
	Scene3D(QWidget * parent = 0);
	void load(Polyhedron mesh);
	void add(Polyhedron mesh);
	void keyPressEvent(QKeyEvent* pe)
	{
		// set the actions of keyboard keys
		switch (pe->key())
		{
		case Qt::Key_Plus: scale_plus(); break;
		case Qt::Key_Equal: scale_plus(); break;
		case Qt::Key_Minus: scale_minus(); break;
		case Qt::Key_Up: rotate_up(); break;
		case Qt::Key_Down: rotate_down(); break;
		case Qt::Key_Left: rotate_left(); break;
		case Qt::Key_Right: rotate_right(); break;
		case Qt::Key_Z: translate_down(); break;
		case Qt::Key_X: translate_up(); break;
		case Qt::Key_Space: defaultScene(); break;
		case Qt::Key_Escape: this->close(); break;
		}
		updateGL();
	}
	int getSkeleton();
	void getParts();
	void newPart();
	void groupsToOff();
	void drawParts();
	void drawPWireframe();
};
#endif
