#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// The masks of 'elements visibility' variable 
#define shAxis 0x01
#define shSkeleton 0x02
#define shWireframe 0x04
#define shFacets 0x08
#define shParts 0x10

#include <QMainWindow>
#include "scene3d.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

private:

public:
	MainWindow();
	Scene3D * widget;		// Qt widget to show the 3D objects
	QMenu * menuActions;	// 'Process' menu
	QMenu * menuOptions;	// 'Elements' menu
	private slots:
	void openModel();
	void addPart();
	void savePOFF();
	void saveSOFF();
	void Skeleton();
	void sGroups();
	void nPart();
	int createSkeleton() { return widget->getSkeleton(); }
	void saveGroups() { widget->groupsToOff(); }
	void setFigureOn();
	void setDockOptions();
	void keyPressEvent(QKeyEvent* pe) { return widget->keyPressEvent(pe); }
};
#endif