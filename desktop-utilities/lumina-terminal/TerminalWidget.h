//===========================================
//  Lumina-DE source code
//  Copyright (c) 2015, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _LUMINA_DESKTOP_UTILITIES_TERMINAL_PROCESS_WIDGET_H
#define _LUMINA_DESKTOP_UTILITIES_TERMINAL_PROCESS_WIDGET_H

#include <QTextEdit>
#include <QKeyEvent>
#include <QSocketNotifier>
#include <QTimer>

#include "TtyProcess.h"

class TerminalWidget : public QTextEdit{
	Q_OBJECT
public:
	TerminalWidget(QWidget *parent =0, QString dir="");
	~TerminalWidget();

	void aboutToClose();

private:
	TTYProcess *PROC;
	//QProcess *PROC;

	QSocketNotifier *sn;
	QTimer *upTimer;

private slots:
	void UpdateText();
	void ShellClosed();

signals:
	void ProcessClosed(QString);

protected:
	void keyPressEvent(QKeyEvent *ev);
	void mousePressEvent(QMouseEvent *ev);
	void mouseDoubleClickEvent(QMouseEvent *ev);
	void contextMenuEvent(QContextMenuEvent *ev);
};

#endif
