#include "mainwindow.h"
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QtWidgets>
#include <Windows.h>
#include "scene3d.h"
#include "functions.h"

// The MainWindow class to visualization of 3D-objects and their processing control
MainWindow::MainWindow()
{
	// create the window controls
	// define the action object to menu items creation
	QAction * action;
	
	// create the 'File' menu which will provide the file loading/saving
	QMenu * menu = menuBar()->addMenu(tr("&File"));
	// create 'New Model' item
	menu->addAction(tr("New Model"), this, &MainWindow::openModel);
	// create 'Append Part' item
	menu->addAction(tr("Append Part"), this, &MainWindow::addPart);
	menu->addSeparator();
	// create 'Save Parts' item
	menu->addAction(tr("Save Parts"), this, &MainWindow::savePOFF);
	// create 'Save Single' item
	menu->addAction(tr("Save Single"), this, &MainWindow::saveSOFF);
	menu->addSeparator();
	// create 'Quit' item
	menu->addAction(tr("&Quit"), this, &QWidget::close);

	// create the 'Process' menu which will provide the start of different calculations
	menuActions = menuBar()->addMenu(tr("&Process"));
	// create 'Create Skeleton' item and set it as disabled
	action = menuActions->addAction(tr("Create Skeleton"), this, &MainWindow::Skeleton);
	action->setEnabled(false);
	// create 'Save separate parts' item and set it as disabled
	action = menuActions->addAction(tr("Save separate parts"), this, &MainWindow::sGroups);
	action->setEnabled(false);
	menuActions->addSeparator();
	// create 'Get new part' item and set it as disabled
	action = menuActions->addAction(tr("Get new part"), this, &MainWindow::nPart);
	action->setEnabled(false);

	// create widget (Scene3D object) to show the 3D-objects
	widget = new Scene3D(this);
	// Set it as central widget of window
	setCentralWidget(widget);

	// create the 'Elements' menu which will provide the control of visibility of elements
	menuOptions = menuBar()->addMenu(tr("Elements"));
	// create the checker for axis
	action = menuOptions->addAction(tr("Axis"));
	action->setCheckable(true);
	action->setChecked(true);
	connect(action, &QAction::toggled, this, &MainWindow::setDockOptions);
	// create the checker for solid's skeleton
	action = menuOptions->addAction(tr("Skeleton"));
	action->setCheckable(true);
	action->setChecked(false);
	action->setEnabled(false);
	connect(action, &QAction::toggled, this, &MainWindow::setDockOptions);
	// create the checker for solid's wireframe
	action = menuOptions->addAction(tr("Wireframe"));
	action->setCheckable(true);
	action->setChecked(false);
	action->setEnabled(false);
	connect(action, &QAction::toggled, this, &MainWindow::setDockOptions);
	// create the checker for solid's facets
	action = menuOptions->addAction(tr("Facets"));
	action->setCheckable(true);
	action->setChecked(false);
	action->setEnabled(false);
	connect(action, &QAction::toggled, this, &MainWindow::setDockOptions);
	// create the checker for extracted segments
	action = menuOptions->addAction(tr("Parts"));
	action->setCheckable(true);
	action->setChecked(false);
	action->setEnabled(false);
	connect(action, &QAction::toggled, this, &MainWindow::setDockOptions);
}

// Open the STL and OFF files
void MainWindow::openModel()
{
	// define the CGAL mesh object
	Polyhedron mesh;
	// get the filename using the standard Qt file dialog
	QString fileName = QFileDialog::getOpenFileName(this, "Load", QDir::currentPath(), "Models (*.stl);;Models (*.off)");
	// extract the file extension
	QString ext = fileName.right(3);
	
	// if the file is STL
	if (QString::compare(ext, "stl", Qt::CaseInsensitive) == 0) {
		// check the type of STL (ascii or binary)
		int type = getStlFileFormat(fileName);
		char * offName;
		// if type is binary
		if (type == STL_BINARY)
			// convert binary STL to OFF
			offName = binStl2Off(fileName.toLatin1().data());
		// if type is ascii
		else if (type == STL_ASCII)
			// convert ascii STL to OFF
			offName = ascStl2Off(fileName.toLatin1().data());
		// unrecognized type of STL
		else {
			// show the error message
			QMessageBox msgBox;
			msgBox.setText("This file corrupt and cannot be loaded");
			msgBox.setInformativeText("STL Load");
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.exec();
			return;
		}
		// load OFF into mesh using CGAL
		std::ifstream input(offName);
		input >> mesh;
	}
	// if the file is OFF
	else {
		// load OFF into mesh using CGAL
		QByteArray ba = fileName.toLatin1();
		std::ifstream input(ba.data());
		input >> mesh;
	}
	// delete the existing widget
	delete widget;
	// create the widget with new mesh
	widget = new Scene3D(this);
	widget->load(mesh);
	// set it as central widget of window
	setCentralWidget(widget);
	setFigureOn();
}

// Skeleton creation start from GUI
void MainWindow::Skeleton()
{
	// call the method of Scene3D (see header)
	int mGroup = createSkeleton();
	// change the statement of 'Skeleton' checker
	QList<QAction*> actions = menuOptions->actions();
	actions.at(1)->setChecked(true);
	actions.at(1)->setEnabled(true);
	
	// disable the 'Create Skeleton' item
	actions = menuActions->actions();
	actions.at(0)->setEnabled(false);
	// check do we have extracted parts
	if (mGroup > 0)
		// enable 'Save separate parts' item
		actions.at(1)->setEnabled(true);
	// enable 'Get new part' item
	actions.at(3)->setEnabled(true);

	// refresh the 'elements visibility' variable
	setDockOptions();
}

// Save all the extracted segments
void MainWindow::sGroups()
{
	// call the method of Scene3D (see header)
	saveGroups();
}

// Try to extract the new segment
void MainWindow::nPart()
{
	// call the method of Scene3D
	widget->newPart();
	
	// use the 'Elements' menu
	QList<QAction*> actions = menuOptions->actions();
	// disable 'Skeleton' checker
	actions.at(1)->setChecked(false);
	actions.at(1)->setEnabled(false);
	
	// use the 'Process' menu
	actions = menuActions->actions();
	// enable 'New Model' item 
	actions.at(0)->setEnabled(true);
	// disable 'Append Part' item
	actions.at(1)->setEnabled(false);
	// disable 'Save Parts' item
	actions.at(3)->setEnabled(false);

	// refresh the 'elements visibility' variable
	setDockOptions();
}

// Save all #D-objects (main and extracted parts) to OFF files
void MainWindow::savePOFF()
{
	// get the filename using the standard Qt file dialog
	QString partName = QFileDialog::getSaveFileName(this, "Save", QDir::currentPath(), "Part (*.off)");
	// loop over the existing objects
	for (int part = 0; part < widget->tmesh.size(); part++) {
		// create the serial part of filename
		std::stringstream sc;
		sc.str(std::string());
		sc << std::setw(3) << std::setfill('0') << part << ".off";
		QString partN = QString::fromStdString(sc.str());
		// replace the extension with a serial part 
		QByteArray ba = partName.toLatin1();
		QString curName = QString(ba).replace(".off", partN);
		// call the method from functions module
		meshToOff(curName.toLatin1().data(), widget->tmesh[part]);
	}
}

// Save the single (main) 3D-object to OFF file
void MainWindow::saveSOFF()
{
	// get the filename using the standard Qt file dialog
	QString fileName = QFileDialog::getSaveFileName(this, "Save", QDir::currentPath(), "Part (*.off)");
	// call the method from functions module
	meshToOff(fileName.toLatin1().data(), mergePoly(widget->tmesh, 0, (int)widget->tmesh.size()));
}

// Add new part from file
void MainWindow::addPart()
{
	// define the CGAL mesh object
	Polyhedron mesh;
	// get the filename using the standard Qt file dialog
	QString fileName = QFileDialog::getOpenFileName(this, "Load", QDir::currentPath(), "Models (*.stl);;Models (*.off)");
	// extract the extension
	QString ext = fileName.right(3);

	// if the file is STL
	if (QString::compare(ext, "stl", Qt::CaseInsensitive) == 0) {
		// check the type of STL (ascii or binary)
		int type = getStlFileFormat(fileName);
		char * offName;
		// if type is binary
		if (type == STL_BINARY)
			// convert binary STL to OFF
			offName = binStl2Off(fileName.toLatin1().data());
		// if type is ascii
		else if (type == STL_ASCII)
			// convert ascii STL to OFF
			offName = ascStl2Off(fileName.toLatin1().data());
		// unrecognized type of STL
		else {
			// show the error message
			QMessageBox msgBox;
			msgBox.setText("This file corrupt and cannot be loaded");
			msgBox.setInformativeText("STL Load");
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.exec();
			return;
		}
		// load OFF into mesh using CGAL
		std::ifstream input(offName);
		input >> mesh;
	}
	// if the file is OFF
	else {
		// load OFF into mesh using CGAL
		QByteArray ba = fileName.toLatin1();
		std::ifstream input(ba.data());
		input >> mesh;
	}
	// add the loaded mesh as a new part to Scene3D object
	widget->add(mesh);
	// refresh the menu
	setFigureOn();
}

// Set the initial statement of menu items
void MainWindow::setFigureOn()
{
	// use the 'Elements' menu
	QList<QAction*> actions = menuOptions->actions();
	// enable and set on 'Wireframe' checker
	actions.at(2)->setChecked(true);
	actions.at(2)->setEnabled(true);
	// enable and set on 'Facets' checker
	actions.at(3)->setChecked(true);
	actions.at(3)->setEnabled(true);
	// enable and set on 'Parts' checker
	actions.at(4)->setChecked(true);
	actions.at(4)->setEnabled(true);

	// use the 'Process' menu
	actions = menuActions->actions();
	// enable 'Create Skeleton'
	actions.at(0)->setEnabled(true);
	// disable 'Save Parts'
	actions.at(1)->setEnabled(false);

	// refresh the 'elements visibility' variable
	setDockOptions();
}

// Create the 'elements visibility' variable according to checkers of 'Elements' menu
void MainWindow::setDockOptions()
{
	// use the 'Elements' menu
	QList<QAction*> actions = menuOptions->actions();
	// initiate variable with zero
	widget->showElem = 0;
	
	// set the mask of 'Axis' item
	if (actions.at(0)->isChecked())
		widget->showElem |= shAxis;
	// set the mask of 'Skeleton' item
	if (actions.at(1)->isChecked())
		widget->showElem |= shSkeleton;
	// set the mask of 'Wireframe' item
	if (actions.at(2)->isChecked())
		widget->showElem |= shWireframe;
	// set the mask of 'Facets' item
	if (actions.at(3)->isChecked())
		widget->showElem |= shFacets;
	// set the mask of 'Parts' item
	if (actions.at(4)->isChecked())
		widget->showElem |= shParts;
	
	// update the showed elements
	widget->update();
}
