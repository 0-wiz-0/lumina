//===========================================
//  Lumina-DE source code
//  Copyright (c) 2015, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _LUMINA_PLAIN_TEXT_EDITOR_MAIN_UI_H
#define _LUMINA_PLAIN_TEXT_EDITOR_MAIN_UI_H

#include <QMainWindow>
#include <QStringList>

#include "PlainTextEditor.h"

namespace Ui{
	class MainUI;
};

class MainUI : public QMainWindow{
	Q_OBJECT
public:
	MainUI();
	~MainUI();

	void LoadArguments(QStringList args); //CLI arguments

public slots:
	void updateIcons();

private:
	Ui::MainUI *ui;

	//Simplification functions
	PlainTextEditor* currentEditor();
	QString currentFileDir();

private slots:
	//Main Actions
	void NewFile();
	void OpenFile(QString file = "");
	void SaveFile();
	void SaveFileAs();

	//Other Menu Actions
	void UpdateHighlighting(QAction*);
	void showLineNumbers(bool);

	//Tab Interactions
	void updateTab(QString);
	void tabChanged();
	void tabClosed(int);

};
#endif