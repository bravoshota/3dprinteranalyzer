#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_PrinterAnalyzer.h"

class PrinterAnalyzer : public QMainWindow
{
	Q_OBJECT

public:
	PrinterAnalyzer(QWidget *parent = Q_NULLPTR);

private:
	Ui::PrinterAnalyzerClass ui;
};
