#include <QtWidgets/QApplication>
#include "mainWindow.h"
#include "scene3D.h"

// Entry point of application
int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	// Create MainWindow object
	MainWindow window;

	// Set title and size of new window
	window.setWindowTitle("Skeleton Viewer");
	window.resize(800, 600);

	// Show and activate window
	window.show();
	window.activateWindow();
	window.raise();

	// Return error if arise
	return app.exec();
}
