//===========================================
//  Lumina-DE source code
//  Copyright (c) 2015, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "MainUI.h"
#include "ui_MainUI.h"

#include "syntaxSupport.h"

#include <LuminaXDG.h>
#include <LuminaUtils.h>

#include <QFileDialog>
#include <QDir>
#include <QKeySequence>
#include <QTimer>

MainUI::MainUI() : QMainWindow(), ui(new Ui::MainUI){
  ui->setupUi(this);
  settings = new QSettings("lumina-desktop","lumina-textedit");
  Custom_Syntax::SetupDefaultColors(settings); //pre-load any color settings as needed
  colorDLG = new ColorDialog(settings, this);
  this->setWindowTitle(tr("Text Editor"));
  ui->tabWidget->clear();
  closeFindS = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(closeFindS, SIGNAL(activated()), this, SLOT(closeFindReplace()) );
  ui->groupReplace->setVisible(false);
  //Update the menu of available syntax highlighting modes
  QStringList smodes = Custom_Syntax::availableRules();
  for(int i=0; i<smodes.length(); i++){
    ui->menuSyntax_Highlighting->addAction(smodes[i]);
  }
  ui->actionLine_Numbers->setChecked( settings->value("showLineNumbers",true).toBool() );
  ui->actionWrap_Lines->setChecked( settings->value("wrapLines",true).toBool() );
  //Setup any connections
  connect(ui->actionClose, SIGNAL(triggered()), this, SLOT(close()) );
  connect(ui->actionNew_File, SIGNAL(triggered()), this, SLOT(NewFile()) );
  connect(ui->actionOpen_File, SIGNAL(triggered()), this, SLOT(OpenFile()) );
  connect(ui->actionClose_File, SIGNAL(triggered()), this, SLOT(CloseFile()) );
  connect(ui->actionSave_File, SIGNAL(triggered()), this, SLOT(SaveFile()) );
  connect(ui->actionSave_File_As, SIGNAL(triggered()), this, SLOT(SaveFileAs()) );
  connect(ui->menuSyntax_Highlighting, SIGNAL(triggered(QAction*)), this, SLOT(UpdateHighlighting(QAction*)) );
  connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged()) );
  connect(ui->tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(tabClosed(int)) );
  connect(ui->actionLine_Numbers, SIGNAL(toggled(bool)), this, SLOT(showLineNumbers(bool)) );
  connect(ui->actionWrap_Lines, SIGNAL(toggled(bool)), this, SLOT(wrapLines(bool)) );
  connect(ui->actionCustomize_Colors, SIGNAL(triggered()), this, SLOT(ModifyColors()) );
  connect(ui->actionFind, SIGNAL(triggered()), this, SLOT(openFind()) );
  connect(ui->actionReplace, SIGNAL(triggered()), this, SLOT(openReplace()) );
  connect(ui->tool_find_next, SIGNAL(clicked()), this, SLOT(findNext()) );
  connect(ui->tool_find_prev, SIGNAL(clicked()), this, SLOT(findPrev()) );
  connect(ui->tool_replace, SIGNAL(clicked()), this, SLOT(replaceOne()) );
  connect(ui->tool_replace_all, SIGNAL(clicked()), this, SLOT(replaceAll()) );
  connect(ui->line_find, SIGNAL(returnPressed()), this, SLOT(findNext()) );
  connect(ui->line_replace, SIGNAL(returnPressed()), this, SLOT(replaceOne()) );
  connect(colorDLG, SIGNAL(colorsChanged()), this, SLOT(UpdateHighlighting()) );
  updateIcons();
  //Now load the initial size of the window
  QSize lastSize = settings->value("lastSize",QSize()).toSize();
  if(lastSize.width() > this->sizeHint().width() && lastSize.height() > this->sizeHint().height() ){
    this->resize(lastSize);
  }
}

MainUI::~MainUI(){
	
}

void MainUI::LoadArguments(QStringList args){ //CLI arguments
  for(int i=0; i<args.length(); i++){
    OpenFile( LUtils::PathToAbsolute(args[i]) );
  ui->line_find->setFocus();	
  }
}

// =================
//      PUBLIC SLOTS
//=================
void MainUI::updateIcons(){
  this->setWindowIcon( LXDG::findIcon("document-edit") );
  ui->actionClose->setIcon(LXDG::findIcon("application-exit") );
  ui->actionNew_File->setIcon(LXDG::findIcon("document-new") );
  ui->actionOpen_File->setIcon(LXDG::findIcon("document-open") );
  ui->actionClose_File->setIcon(LXDG::findIcon("document-close") );
  ui->actionSave_File->setIcon(LXDG::findIcon("document-save") );
  ui->actionSave_File_As->setIcon(LXDG::findIcon("document-save-as") );
  ui->actionFind->setIcon(LXDG::findIcon("edit-find") );
  ui->actionReplace->setIcon(LXDG::findIcon("edit-find-replace") );
  ui->menuSyntax_Highlighting->setIcon( LXDG::findIcon("format-text-color") );
  ui->actionCustomize_Colors->setIcon( LXDG::findIcon("format-fill-color") );
  //icons for the special find/replace groupbox
  ui->tool_find_next->setIcon(LXDG::findIcon("go-down-search"));
  ui->tool_find_prev->setIcon(LXDG::findIcon("go-up-search"));
  ui->tool_find_casesensitive->setIcon(LXDG::findIcon("format-text-italic"));
  ui->tool_replace->setIcon(LXDG::findIcon("arrow-down"));
  ui->tool_replace_all->setIcon(LXDG::findIcon("arrow-down-double"));
  //ui->tool_find_next->setIcon(LXDG::findIcon(""));
	
  QTimer::singleShot(0,colorDLG, SLOT(updateIcons()) );
}

// =================
//          PRIVATE
//=================
PlainTextEditor* MainUI::currentEditor(){
  if(ui->tabWidget->count()<1){ return 0; }
  return static_cast<PlainTextEditor*>( ui->tabWidget->currentWidget() );
}

QString MainUI::currentFileDir(){
  PlainTextEditor* cur = currentEditor();
  QString dir;
  if(cur!=0){
    if(cur->currentFile().startsWith("/")){
      dir = cur->currentFile().section("/",0,-2);
    }
  }
  return dir;
}

// =================
//    PRIVATE SLOTS
//=================
//Main Actions
void MainUI::NewFile(){
  OpenFile(QString::number(ui->tabWidget->count()+1)+"/"+tr("New File"));
}

void MainUI::OpenFile(QString file){
  QStringList files;
  if(file.isEmpty()){
    //Prompt for a file to open
    files = QFileDialog::getOpenFileNames(this, tr("Open File(s)"), currentFileDir(), tr("Text Files (*)") );
    if(files.isEmpty()){ return; } //cancelled
  }else{
    files << file;
  }
  for(int i=0; i<files.length(); i++){
    PlainTextEditor *edit = new PlainTextEditor(settings, this);
      connect(edit, SIGNAL(FileLoaded(QString)), this, SLOT(updateTab(QString)) );
      connect(edit, SIGNAL(UnsavedChanges(QString)), this, SLOT(updateTab(QString)) );
    ui->tabWidget->addTab(edit, files[i].section("/",-1));
    edit->showLineNumbers(ui->actionLine_Numbers->isChecked());
    edit->setLineWrapMode( ui->actionWrap_Lines->isChecked() ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    edit->setFocusPolicy(Qt::ClickFocus); //no "tabbing" into this widget
    ui->tabWidget->setCurrentWidget(edit);
    edit->LoadFile(files[i]);
    edit->setFocus();
    QApplication::processEvents(); //to catch the fileLoaded() signal
  }
}

void MainUI::CloseFile(){
  int index = ui->tabWidget->currentIndex();
  if(index>=0){ tabClosed(index); }
}

void MainUI::SaveFile(){
  PlainTextEditor *cur = currentEditor();
  if(cur==0){ return; }
  cur->SaveFile();
}

void MainUI::SaveFileAs(){
  PlainTextEditor *cur = currentEditor();
  if(cur==0){ return; }
  cur->SaveFile(true);	
}

void MainUI::UpdateHighlighting(QAction *act){
  if(act!=0){
    //Single-editor change
    PlainTextEditor *cur = currentEditor();
    if(cur==0){ return; }
    cur->LoadSyntaxRule(act->text());
  }else{
    //Have every editor reload the syntax rules (color changes)
    for(int i=0; i<ui->tabWidget->count(); i++){
      static_cast<PlainTextEditor*>(ui->tabWidget->widget(i))->updateSyntaxColors();
    }
  }
}

void MainUI::showLineNumbers(bool show){
  settings->setValue("showLineNumbers",show);
  for(int i=0; i<ui->tabWidget->count(); i++){
    PlainTextEditor *edit = static_cast<PlainTextEditor*>(ui->tabWidget->widget(i));
    edit->showLineNumbers(show);
  }
}

void MainUI::wrapLines(bool wrap){
  settings->setValue("wrapLines",wrap);
  for(int i=0; i<ui->tabWidget->count(); i++){
    PlainTextEditor *edit = static_cast<PlainTextEditor*>(ui->tabWidget->widget(i));
    edit->setLineWrapMode( wrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
  }	
}

void MainUI::ModifyColors(){
  colorDLG->LoadColors();
  colorDLG->showNormal();
}

void MainUI::updateTab(QString file){
  PlainTextEditor *cur = 0;
  int index = -1;
  for(int i=0; i<ui->tabWidget->count(); i++){
    PlainTextEditor *tmp = static_cast<PlainTextEditor*>(ui->tabWidget->widget(i));
    if(tmp->currentFile()==file){
	cur = tmp;
	index = i;
	break;
    }
  }
  if(cur==0){ return; } //should never happen
  bool changes = cur->hasChange();
  //qDebug() << "Update Tab:" << file << cur << changes;
  ui->tabWidget->setTabText(index,(changes ? "*" : "") + file.section("/",-1));
  ui->actionSave_File->setEnabled(changes);
  ui->actionSave_File_As->setEnabled(changes);
  this->setWindowTitle( ui->tabWidget->tabText( ui->tabWidget->currentIndex() ) );
}

void MainUI::tabChanged(){
  //update the buttons/menus based on the current widget
  PlainTextEditor *cur = currentEditor();
  if(cur==0){ return; } //should never happen though
  bool changes = cur->hasChange();
  ui->actionSave_File->setEnabled(changes);
  ui->actionSave_File_As->setEnabled(changes);
  this->setWindowTitle( ui->tabWidget->tabText( ui->tabWidget->currentIndex() ) );
  if(!ui->line_find->hasFocus() && !ui->line_replace->hasFocus()){ ui->tabWidget->currentWidget()->setFocus(); }
}

void MainUI::tabClosed(int tab){
  PlainTextEditor *edit = static_cast<PlainTextEditor*>(ui->tabWidget->widget(tab));
  if(edit==0){ return; } //should never happen
  if(edit->hasChange()){
    //Verify if the user wants to lose any unsaved changes
	  
  }
  ui->tabWidget->removeTab(tab);
  edit->deleteLater();
}

//Find/Replace functions
void MainUI::closeFindReplace(){
  ui->groupReplace->setVisible(false);
  PlainTextEditor *cur = currentEditor();
  if(cur!=0){ cur->setFocus(); }	
}

void MainUI::openFind(){
  PlainTextEditor *cur = currentEditor();
  if(cur==0){ return; }
  ui->groupReplace->setVisible(true);
  ui->line_find->setText( cur->textCursor().selectedText() );
  ui->line_replace->setText(""); 
  ui->line_find->setFocus();	
}

void MainUI::openReplace(){
  PlainTextEditor *cur = currentEditor();
  if(cur==0){ return; }
  ui->groupReplace->setVisible(true);
  ui->line_find->setText( cur->textCursor().selectedText() );
  ui->line_replace->setText(""); 
  ui->line_replace->setFocus();
}

void MainUI::findNext(){
  PlainTextEditor *cur = currentEditor();
  if(cur==0){ return; }
  bool found = cur->find( ui->line_find->text(), ui->tool_find_casesensitive->isChecked() ? QTextDocument::FindCaseSensitively : QTextDocument::FindFlags() );
  if(!found){
    //Try starting back at the top of the file
    cur->moveCursor(QTextCursor::Start);
    cur->find( ui->line_find->text(), ui->tool_find_casesensitive->isChecked() ? QTextDocument::FindCaseSensitively : QTextDocument::FindFlags() );	  
  }
}

void MainUI::findPrev(){
  PlainTextEditor *cur = currentEditor();
  if(cur==0){ return; }
  bool found = cur->find( ui->line_find->text(), ui->tool_find_casesensitive->isChecked() ? QTextDocument::FindCaseSensitively | QTextDocument::FindBackward : QTextDocument::FindBackward );
  if(!found){
    //Try starting back at the bottom of the file
    cur->moveCursor(QTextCursor::End);
    cur->find( ui->line_find->text(), ui->tool_find_casesensitive->isChecked() ? QTextDocument::FindCaseSensitively | QTextDocument::FindBackward : QTextDocument::FindBackward );	  
  }
}

void MainUI::replaceOne(){
  PlainTextEditor *cur = currentEditor();
  if(cur==0){ return; }
  //See if the current selection matches the find field first
  bool done = false;
  if(cur->textCursor().selectedText()==ui->line_find->text()){
    cur->insertPlainText(ui->line_replace->text());
    done = true;
  }else{
    //Find/replace the next occurance of the string
    bool found = cur->find( ui->line_find->text(), ui->tool_find_casesensitive->isChecked() ? QTextDocument::FindCaseSensitively : QTextDocument::FindFlags() );
    if(found){ cur->insertPlainText(ui->line_replace->text()); done = true;}
  }
  if(done){
    //Re-highlight the newly-inserted text
    cur->find( ui->line_replace->text(), QTextDocument::FindCaseSensitively | QTextDocument::FindBackward);
  }
}

void MainUI::replaceAll(){
PlainTextEditor *cur = currentEditor();
  if(cur==0){ return; }
  //See if the current selection matches the find field first
  bool done = false;
  if(cur->textCursor().selectedText()==ui->line_find->text()){
    cur->insertPlainText(ui->line_replace->text());
    done = true;
  }
  while( cur->find( ui->line_find->text(), ui->tool_find_casesensitive->isChecked() ? QTextDocument::FindCaseSensitively : QTextDocument::FindFlags() ) ){
    //Find/replace every occurance of the string
    cur->insertPlainText(ui->line_replace->text());
    done = true;
  }
  if(done){
    //Re-highlight the newly-inserted text
    cur->find( ui->line_replace->text(), QTextDocument::FindCaseSensitively | QTextDocument::FindBackward);
  }
}
