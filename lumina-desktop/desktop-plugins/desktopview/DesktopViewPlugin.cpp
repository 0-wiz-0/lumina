#include "DesktopViewPlugin.h"

#include <QFileInfo>
#include <QDir>

#include <LuminaXDG.h>
#include "LSession.h"


DesktopViewPlugin::DesktopViewPlugin(QWidget* parent, QString ID) : LDPlugin(parent, ID){
  this->setLayout( new QVBoxLayout());
    this->layout()->setContentsMargins(0,0,0,0);
  list = new QListWidget(this);
    list->setUniformItemSizes(true);
    list->setViewMode(QListView::IconMode);
    list->setLayoutMode(QListView::Batched);
    list->setBatchSize(10); //keep it snappy
    list->setSpacing(2);
    list->setSelectionBehavior(QAbstractItemView::SelectItems);
    list->setSelectionMode(QAbstractItemView::NoSelection);
    list->setStyleSheet( "QListWidget{ background: rgba(255,255,255,100); border: none; border-radius: 3px;}" );
    list->setIconSize(QSize(64,64));
  this->layout()->addWidget(list);
  this->setInitialSize(200,300);
  watcher = new QFileSystemWatcher(this);
    watcher->addPath(QDir::homePath()+"/Desktop");
    connect(watcher, SIGNAL(directoryChanged(QString)), this, SLOT(updateContents()) );
	
  connect(list, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(runItem(QListWidgetItem*)) );
  QTimer::singleShot(0,this, SLOT(updateContents()) );
}

DesktopViewPlugin::~DesktopViewPlugin(){

}

void DesktopViewPlugin::runItem(QListWidgetItem *item){
    LSession::LaunchApplication("lumina-open \""+item->whatsThis()+"\"");
}

void DesktopViewPlugin::updateContents(){
  list->clear();
  list->setGridSize(QSize(80,70+this->fontMetrics().height()));
  QDir dir(QDir::homePath()+"/Desktop");
  QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Type | QDir::DirsFirst);
  for(int i=0; i<files.length(); i++){
    QListWidgetItem *it = new QListWidgetItem;
    it->setWhatsThis(files[i].absoluteFilePath());
    if(files[i].isDir()){
	it->setIcon( LXDG::findIcon("folder","") );
	it->setText( files[i].fileName() );
    }else if(files[i].suffix() == "desktop" ){
	bool ok = false;
	XDGDesktop desk = LXDG::loadDesktopFile(files[i].absoluteFilePath(), ok);
	if(ok){
	  it->setIcon( LXDG::findIcon(desk.icon,"") );
          it->setText( desk.name );
	}else{
	  //Revert back to a standard file handling
          it->setIcon( LXDG::findMimeIcon(files[i].suffix()) );
          it->setText( files[i].fileName() );		
	}
    }else{
      it->setIcon( LXDG::findMimeIcon(files[i].suffix()) );
      it->setText( files[i].fileName() );
    }
    list->addItem(it);
  }
}