//===========================================
//  Lumina-DE source code
//  Copyright (c) 2016, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "RSSFeedPlugin.h"
#include "ui_RSSFeedPlugin.h"

#include <LuminaXDG.h>
#include "LSession.h"
#include <LuminaUtils.h>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QtConcurrent>

RSSFeedPlugin::RSSFeedPlugin(QWidget* parent, QString ID) : LDPlugin(parent, ID), ui(new Ui::RSSFeedPlugin()){
  ui->setupUi(this);
  //Load the global settings 
  setprefix = "rssreader/"; //this structure/prefix should be used for *all* plugins of this type
  RSS = new RSSReader(this, setprefix);

  //Create the options menu
  optionsMenu = new QMenu(this);
  ui->tool_options->setMenu(optionsMenu);
  //Setup any signal/slot connections
  connect(ui->push_back1, SIGNAL(clicked()), this, SLOT(backToFeeds()) );
  connect(ui->push_back2, SIGNAL(clicked()), this, SLOT(backToFeeds()) );
  connect(ui->push_back3, SIGNAL(clicked()), this, SLOT(backToFeeds()) );
  connect(ui->push_save_settings, SIGNAL(clicked()), this, SLOT(saveSettings()) );
  connect(RSS, SIGNAL(rssChanged(QString)), this, SLOT(RSSItemChanged(QString)) );
  connect(RSS, SIGNAL(newChannelsAvailable()), this, SLOT(UpdateFeedList()));
  connect(ui->tool_gotosite, SIGNAL(clicked()), this, SLOT(openFeedPage()) );
  connect(ui->push_rm_feed, SIGNAL(clicked()), this, SLOT(removeFeed()) );
  connect(ui->push_add_url, SIGNAL(clicked()), this, SLOT(addNewFeed()) );
  updateOptionsMenu();
  QTimer::singleShot(0,this, SLOT(loadIcons()) );
  //qDebug() << " - Done with init";
  QStringList feeds;
  if( !LSession::handle()->DesktopPluginSettings()->contains(setprefix+"currentfeeds") ){
    //First-time run of the plugin - automatically load the default feeds
    feeds << "http://lumina-desktop.org/?feed=rss2"; //Lumina Desktop blog feed
    LSession::handle()->DesktopPluginSettings()->setValue(setprefix+"currentfeeds", feeds);
  }else{
    feeds = LSession::handle()->DesktopPluginSettings()->value(setprefix+"currentfeeds",QStringList()).toStringList();
  }
  RSS->addUrls(feeds);
}

RSSFeedPlugin::~RSSFeedPlugin(){

}

//================
//     PRIVATE
//================
void RSSFeedPlugin::updateOptionsMenu(){
  optionsMenu->clear();
  optionsMenu->addAction(LXDG::findIcon("list-add",""), tr("Add RSS Feed"), this, SLOT(openFeedNew()) );
  optionsMenu->addAction(LXDG::findIcon("help-about",""), tr("View Feed Details"), this, SLOT(openFeedInfo()) );
  optionsMenu->addAction(LXDG::findIcon("configure",""), tr("Settings"), this, SLOT(openSettings()) );
  optionsMenu->addSeparator();
  optionsMenu->addAction(LXDG::findIcon("download",""), tr("Update Feeds Now"), this, SLOT(resyncFeeds()) );
}

//Simplification functions for loading feed info onto widgets
void RSSFeedPlugin::updateFeed(QString ID){
  //Save the datetime this feed was read
  LSession::handle()->DesktopPluginSettings()->setValue(setprefix+"feedReads/"+ID, QDateTime::currentDateTime() );
  
  //Now clear/update the feed viewer (HTML)
  ui->text_feed->clear();
  QString html;
  RSSchannel data = RSS->dataForID(ID);
  ui->label_lastupdate->setText( data.lastsync.toString(Qt::DefaultLocaleShortDate) );
  // - generate the html
 // html.append("<ul style=\"margin-left: 3px;\">\n");
  for(int i=0; i<data.items.length(); i++){
    //html.append("<li>");
    html.append("<h3><a href=\""+data.items[i].link+"\">"+data.items[i].title+"</a></h3>");
    if(!data.items[i].pubdate.isNull()){html.append("<i>("+data.items[i].pubdate.toString(Qt::DefaultLocaleShortDate)+")</i><br>"); }
    html.append(data.items[i].description);
    //html.append("</li>\n");
    if(i+1 < data.items.length()){ html.append("<br>"); }
  }
  //html.append("</ul>\n");
  // - load that html into the viewer
  ui->text_feed->setHtml(html);
}

void RSSFeedPlugin::updateFeedInfo(QString ID){
  ui->page_feed_info->setWhatsThis(ID);
}

//================
//  PRIVATE SLOTS
//================
void RSSFeedPlugin::loadIcons(){
  ui->tool_options->setIcon( LXDG::findIcon("configure","") );
  ui->tool_gotosite->setIcon( LXDG::findIcon("applications-internet","") );
  ui->push_back1->setIcon( LXDG::findIcon("go-previous","") );
  ui->push_back2->setIcon( LXDG::findIcon("go-previous","") );
  ui->push_back3->setIcon( LXDG::findIcon("go-previous","") );
  ui->push_rm_feed->setIcon( LXDG::findIcon("list-remove","") );
  ui->push_add_url->setIcon( LXDG::findIcon("list-add","") );
  ui->push_save_settings->setIcon( LXDG::findIcon("document-save","") );
}

//GUI slots
// - Page management
void RSSFeedPlugin::backToFeeds(){
  ui->stackedWidget->setCurrentWidget(ui->page_feed);
}

void RSSFeedPlugin::openFeedInfo(){
  QString ID = ui->combo_feed->currentData().toString();
  if(ID.isEmpty()){ return; }
  updateFeedInfo(ID);
  ui->stackedWidget->setCurrentWidget(ui->page_feed_info);
  
}

void RSSFeedPlugin::openFeedNew(){
  ui->line_new_url->setText("");
  ui->stackedWidget->setCurrentWidget(ui->page_new_feed);
}

void RSSFeedPlugin::openSettings(){
  //Sync the widget with the current settings
  QSettings *set = LSession::handle()->DesktopPluginSettings();

  ui->check_manual_sync->setChecked( set->value(setprefix+"manual_sync_only", false).toBool() );
  int DI = set->value(setprefix+"default_interval_minutes", 60).toInt();
  if(DI<1){ DI = 60; }
  if( (DI%60) == 0 ){DI = DI/60; ui->combo_sync_units->setCurrentIndex(1); } //hourly setting
  else{ ui->combo_sync_units->setCurrentIndex(1);  } //minutes setting
  ui->spin_synctime->setValue(DI);

  //Now show the page
  ui->stackedWidget->setCurrentWidget(ui->page_settings);
}

// - Feed Management
void RSSFeedPlugin::addNewFeed(){
  if(ui->line_new_url->text().isEmpty()){ return; } //nothing to add
  //Validate the URL
  QUrl url(ui->line_new_url->text());
  if(!url.isValid()){
    ui->line_new_url->setFocus();
    return;
  }
  //Add the URL to the settings file for next login
  QStringList feeds = LSession::handle()->DesktopPluginSettings()->value(setprefix+"currentfeeds",QStringList()).toStringList();
  feeds << url.toString();
  LSession::handle()->DesktopPluginSettings()->setValue(setprefix+"currentfeeds", feeds);

  //Set this URL as the current selection
  ui->combo_feed->setWhatsThis(url.toString()); //hidden field - will trigger an update in a moment
   //Add the URL to the backend
   RSS->addUrls(QStringList() << url.toString());
  //UpdateFeedList(); //now re-load the feeds which are available

  //Now go back to the main page
  backToFeeds();
}

void RSSFeedPlugin::removeFeed(){
  QString ID = ui->page_feed_info->whatsThis();
  if(ID.isEmpty()){ return; }
  //Remove from the RSS feed object
  RSS->removeUrl(ID);
  //Remove the URL from the settings file for next login
  QStringList feeds = LSession::handle()->DesktopPluginSettings()->value(setprefix+"currentfeeds",QStringList()).toStringList();
  feeds.removeAll(ID);
  LSession::handle()->DesktopPluginSettings()->setValue(setprefix+"currentfeeds", feeds);
  //Now go back to the main page
  backToFeeds();
}

void RSSFeedPlugin::resyncFeeds(){
  RSS->addUrls( LSession::handle()->DesktopPluginSettings()->value(setprefix+"currentfeeds",QStringList()).toStringList() );
  RSS->syncNow();
}

// - Feed Interactions
void RSSFeedPlugin::currentFeedChanged(){
  QString ID = ui->combo_feed->currentData().toString();
  if(ID.isEmpty()){ return; } //no feed selected
  //Remove the "unread" color from the feed
  ui->combo_feed->setItemData( ui->combo_feed->currentIndex(), QBrush(Qt::transparent) , Qt::BackgroundRole);
  updateFeed(ID);
}

void RSSFeedPlugin::openFeedPage(){ //Open main website for feed
  QString ID = ui->combo_feed->currentData().toString();
  //Find the data associated with this feed
  RSSchannel data = RSS->dataForID(ID);
  QString url = data.link;
  qDebug() << "Open Feed Page:" << url;
  //Now launch the browser
  if(!url.isEmpty()){
    LSession::LaunchApplication("lumina-open \""+url+"\"");
  }
}

void RSSFeedPlugin::saveSettings(){
  QSettings *set = LSession::handle()->DesktopPluginSettings();
  set->setValue(setprefix+"manual_sync_only", ui->check_manual_sync->isChecked() );
  int DI = ui->spin_synctime->value();
  if(ui->combo_sync_units->currentIndex()==1){ DI = DI*60; } //convert from hours to minutes
  set->setValue(setprefix+"default_interval_minutes", DI);
  set->sync();
  
  //Now go back to the feeds
  backToFeeds();
}

//Feed Object interactions
void RSSFeedPlugin::UpdateFeedList(){
  
  QString activate = ui->combo_feed->whatsThis();
  if(!activate.isEmpty()){ ui->combo_feed->setWhatsThis(""); }
  if(activate.isEmpty()){ activate = ui->combo_feed->currentData().toString(); } // keep current item selected
  //Now get/list all the available feeds
  QStringList IDS = RSS->channels(); //this is pre-sorted by title of the feed
  //qDebug() << "Update RSS Feed List:" << IDS << activate;
  for(int i=0; i<IDS.length(); i++){
    bool newitem = false;
    if(ui->combo_feed->count()<=i){ newitem = true; }
    else{
      QString cid = ui->combo_feed->itemData(i).toString();
      if(IDS[i]!=cid){
        if(IDS.contains(cid)){ newitem = true; } //this item is just out of order
        else{ ui->combo_feed->removeItem(i); } //item no longer is valid
      }
    }
    if(newitem){
      //Need to add a new item at this point in the menu
      RSSchannel info = RSS->dataForID(IDS[i]);
      if(info.title.isEmpty()){
        //invalid/empty channel
        ui->combo_feed->insertItem(i, IDS[i], IDS[i]); //just show the URL
      }else{
       ui->combo_feed->insertItem(i, info.icon, info.title, IDS[i]);
      }
    }
  }
  //Remove any extra items on the end of the list
  for(int i=IDS.length(); i<ui->combo_feed->count(); i++){
    ui->combo_feed->removeItem(i); i--;
  }
  //Now activate the proper item as needed
  if(IDS.contains(activate)){
    ui->combo_feed->setCurrentIndex( IDS.indexOf(activate) );
  }
}

void RSSFeedPlugin::RSSItemChanged(QString ID){
  for(int i=0; i<ui->combo_feed->count(); i++){
    if(ui->combo_feed->itemData(i).toString()!=ID){ continue; }
    RSSchannel info = RSS->dataForID(ID);
    if(info.title.isEmpty()){ 
      ui->combo_feed->setItemText(i, ID);
      ui->combo_feed->setItemIcon(i, LXDG::findIcon("dialog-cancel","") );
    }else{
      ui->combo_feed->setItemText(i, info.title);
      ui->combo_feed->setItemIcon(i, info.icon );
      QColor color(Qt::transparent);
      if( info.lastBuildDate > LSession::handle()->DesktopPluginSettings()->value(setprefix+"feedReads/"+ID,QDateTime()).toDateTime() ){
        color = QColor(255,10,10,100); //semi-transparent red
      }
      ui->combo_feed->setItemData(i, QBrush(color) , Qt::BackgroundRole);
    }
  }
  if(ID == ui->combo_feed->currentData().toString()){
    currentFeedChanged(); //re-load the current feed
  }
}

//==================
//   PUBLIC SLOTS
//==================
void RSSFeedPlugin::LocaleChange(){
 ui->retranslateUi(this);
  updateOptionsMenu();
}
void RSSFeedPlugin::ThemeChange(){
  QTimer::singleShot(0,this, SLOT(loadIcons()));
  updateOptionsMenu();
}
