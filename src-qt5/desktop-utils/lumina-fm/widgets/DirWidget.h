//===========================================
//  Lumina-DE source code
//  Copyright (c) 2015, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _LUMINA_FM_DIRECTORY_BROWSER_WIDGET_H
#define _LUMINA_FM_DIRECTORY_BROWSER_WIDGET_H

#include <QList>
#include <QWidget>
#include <QObject>
#include <QMenu>
#include <QToolBar>
#include <QLineEdit>
#include <QShortcut>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QFuture>

#include "../DirData.h"
#include "DDListWidgets.h"

#define ZSNAPDIR QString("/.zfs/snapshot/")

namespace Ui{
	class DirWidget;
};

class DirWidget : public QWidget{
	Q_OBJECT
public:
	enum DETAILTYPES{ NAME, SIZE, TYPE, DATEMOD, DATECREATE};
	DirWidget(QString objID, QWidget *parent = 0); //needs a unique ID (to distinguish from other DirWidgets)
	~DirWidget();
	
	void cleanup(); //called before the browser is closed down
	
	//Directory Managment
	void ChangeDir(QString dirpath);
	void setDirCompleter(QCompleter *comp);
	
	//Information
	QString id();
	QString currentDir();

	//View Settings
	void setShowDetails(bool show);
	void setShowSidebar(bool show);
	void setShowThumbnails(bool show);
	void setDetails(QList<DETAILTYPES> list); //Which details to show and in which order (L->R)
	void setThumbnailSize(int px);
	void setShowCloseButton(bool show);
	void setFocusLineDir();

    //Date format for show items
    QStringList getDateFormat();
    void setDateFormat();
	
public slots:
	void LoadDir(QString dir, LFileInfoList list);
	void LoadSnaps(QString basedir, QStringList snaps);
	
	//Refresh options
	void refresh(); //Refresh current directory
	void refreshButtons(); //Refresh action buttons only

	//Theme change functions
	void UpdateIcons();
	void UpdateText();
	
	//Button updates
	void UpdateButtons();

	//Keyboard Shortcuts triggered
	void TryRenameSelection();
	void TryCutSelection();
	void TryCopySelection();
	void TryPasteSelection();
	void TryDeleteSelection();

private:
	Ui::DirWidget *ui;
	QString ID, CDIR; //unique ID assigned by the parent and the current dir path
	LFileInfoList CLIST; //current item list (snap or not)
	QString normalbasedir, snapbasedir, snaprelpath; //for maintaining directory context while moving between snapshots
	QStringList snapshots, needThumbs, tmpSel;
	bool showDetails, showThumbs, canmodify, stopload; //which widget to use for showing items
	QList<DETAILTYPES> listDetails;
	QMenu *contextMenu;
	QFuture<void> thumbThread;
	//The Toolbar and associated items
	QToolBar *toolbar;
	QLineEdit *line_dir;
	QStringList history;
	//The drag and drop brower widgets
	DDListWidget *listWidget;
	DDTreeWidget *treeWidget;

	//Keyboard Shortcuts
	//QShortcut *copyFilesShort, *cutFilesShort, *pasteFilesShort, *deleteFilesShort;
	//Watcher to determine when the dir changes
	QFileSystemWatcher *watcher;
	QTimer *synctimer;

	//Functions for internal use
	void setupConnections();
	QStringList currentSelection();
	QStringList date_format;

private slots:
	//Internal loading of thumbnails
	void startLoadThumbs();
	void showThumb(QString file, QIcon ico);

	//UI BUTTONS/Actions
	// -- Left Action Buttons
	void on_tool_act_copy_clicked();
	void on_tool_act_cut_clicked();
	void on_tool_act_fav_clicked();
	void on_tool_act_paste_clicked();
	void on_tool_act_rename_clicked();
	void on_tool_act_rm_clicked();
	void on_tool_act_run_clicked();
	void on_tool_act_runwith_clicked();
	// -- Bottom Action Buttons
	void on_tool_goToImages_clicked();
	void on_tool_goToPlayer_clicked();
	void on_tool_new_file_clicked();
	void on_tool_new_dir_clicked();
	// -- Top Snapshot Buttons
	void on_tool_snap_newer_clicked();
	void on_tool_snap_older_clicked();
	void on_slider_snap_valueChanged(int val = -1);
	void direct_snap_selected(QAction*);
	
	//Top Toolbar buttons
	void on_actionBack_triggered();
	void on_actionUp_triggered();
	void on_actionHome_triggered();
	void on_actionStopLoad_triggered();
	void dir_changed(); //user manually changed the directory
	void on_actionClose_Browser_triggered();
	
	// - Other Actions without a specific button on the side
	void fileCheckSums();
	void fileProperties();
	void openTerminal();
	

	//Browser Functions
	void OpenContextMenu();
	void SelectionChanged();
	void startSync(const QString &file); //used internally to collect/pause before updating the dir

signals:
	//Directory loading/finding signals
	void OpenDirectories(QStringList); //Directories to open in other tabs/columns
	void LoadDirectory(QString, QString); //ID, dirpath (Directory to load here)
	void findSnaps(QString, QString); //ID, dirpath (Request snapshot information for a directory)
	void CloseBrowser(QString); //ID (Request that this browser be closed)
	
	//External App/Widget launching
	void PlayFiles(LFileInfoList); //open in multimedia player
	void ViewFiles(LFileInfoList); //open in slideshow
	void LaunchTerminal(QString); //dirpath
	
	//System Interactions
	void CutFiles(QStringList); //file selection
	void CopyFiles(QStringList); //file selection
	void PasteFiles(QString, QStringList); //current dir
	void FavoriteFiles(QStringList); //file selection
	void RenameFiles(QStringList); //file selection
	void RemoveFiles(QStringList); //file selection
	
	//Internal thumbnail loading system (multi-threaded)
	void ThumbLoaded(QString, QIcon);
	
protected:
	void mouseReleaseEvent(QMouseEvent *);
	
};

#endif
