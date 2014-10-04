//===========================================
//  Lumina-DE source code
//  Copyright (c) 2014, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "LuminaThemes.h"

#include "LuminaUtils.h"
#include "LuminaOS.h"

QStringList LTHEME::availableSystemThemes(){ 
  //returns: [name::::path] for each item
	
}

QStringList LTHEME::availableLocalThemes(){	//returns: [name::::path] for each item
	
}

QStringList LTHEME::availableSystemColors(){ 	//returns: [name::::path] for each item
	
}

QStringList LTHEME::availableLocalColors(){ 	//returns: [name::::path] for each item
	
}

QStringList LTHEME::availableSystemIcons(){ 	//returns: [name] for each item
	
}
	
  //Return the currently selected Theme/Colors/Icons
QStringList LTHEME::currentSettings(){ //returns [theme path, colorspath, iconsname]
	
}
	
  //Change the current Theme/Colors/Icons
bool LTHEME::setCurrentSettings(QString themepath, QString colorpath, QString iconname){
	
}
	
  //Return the complete stylesheet for a given theme/colors
QString LTHEME::assembleStyleSheet(QString themepath, QString colorpath){
	
}

//==================
//  THEME ENGINE CLASS
//==================
LuminaThemeEngine::LuminaThemeEngine(QApplication *app){
  application=app; //save this pointer for later
  QStringList current = LTHEME::currentSettings();
  theme = current[0]; colors=current[1]; icons=current[2];
  application->setCurrentStyleSheet( LTHEME::assembleStyleSheet(theme, colors) );
  watcher = new QFileSystemWatcher(this);
	watcher->addPath( QDir::homePath()+"/.lumina/currenttheme.conf" );
  connect(watcher, SIGNAL(fileChanged(const &QString)), this, SLOT(watcherChange()) );
}

LuminaThemeEngine::~LuminaThemeEngine(){

}

void LuminaThemeEngine::watcherChange(){
  QStringList current = LTHEME::currentSettings();
  if(theme!=current[0] || colors!=current[1]){
    application->setCurrentStyleSheet( LTHEME::assembleStyleSheet(current[0], current[1]) );
  }
  if(icons!=current[3]){
    emit updateIcons();
  }
  //Now save this for later checking
  theme = current[0]; colors=current[1]; icons=current[2];
}
