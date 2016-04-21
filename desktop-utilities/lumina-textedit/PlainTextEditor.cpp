//===========================================
//  Lumina-DE source code
//  Copyright (c) 2015, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "PlainTextEditor.h"

#include <QColor>
#include <QPainter>
#include <QTextBlock>
#include <QFileDialog>
#include <QDebug>

#include <LuminaUtils.h>

//==============
//       PUBLIC
//==============
PlainTextEditor::PlainTextEditor(QSettings *set, QWidget *parent) : QPlainTextEdit(parent){
  settings = set;
  LNW = new LNWidget(this);
  showLNW = true;
  hasChanges = false;
  lastSaveContents.clear();
  matchleft = matchright = -1;
  this->setTabStopWidth( 8 * this->fontMetrics().width(" ") ); //8 character spaces per tab (UNIX standard)
  //this->setObjectName("PlainTextEditor");
  //this->setStyleSheet("QPlainTextEdit#PlainTextEditor{ }");
  SYNTAX = new Custom_Syntax(settings, this->document());
  connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(LNW_updateWidth()) );
  connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(LNW_highlightLine()) );
  connect(this, SIGNAL(updateRequest(const QRect&, int)), this, SLOT(LNW_update(const QRect&, int)) );
  connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(checkMatchChar()) );
  connect(this, SIGNAL(textChanged()), this, SLOT(textChanged()) );
  LNW_updateWidth();
  LNW_highlightLine();
}

PlainTextEditor::~PlainTextEditor(){
	
}

void PlainTextEditor::showLineNumbers(bool show){
  showLNW = show;
  LNW->setVisible(show);
  LNW_updateWidth();
}

void PlainTextEditor::LoadSyntaxRule(QString type){
  SYNTAX->loadRules(type);
  SYNTAX->rehighlight();
}

void PlainTextEditor::updateSyntaxColors(){
  SYNTAX->reloadRules();
  SYNTAX->rehighlight();	
}

//File loading/setting options
void PlainTextEditor::LoadFile(QString filepath){
  this->setWhatsThis(filepath);
  this->clear();
  SYNTAX->loadRules( Custom_Syntax::ruleForFile(filepath.section("/",-1)) );
  lastSaveContents = LUtils::readFile(filepath).join("\n");
  this->setPlainText( lastSaveContents );
  hasChanges = false;
  emit FileLoaded(this->whatsThis());
}

void PlainTextEditor::SaveFile(bool newname){
  //qDebug() << "Save File:" << this->whatsThis();
  if( !this->whatsThis().startsWith("/") || newname ){
    //prompt for a filename/path
    QString file = QFileDialog::getSaveFileName(this, tr("Save File"), this->whatsThis(), tr("Text File (*)"));
    if(file.isEmpty()){ return; }
    this->setWhatsThis(file);
    SYNTAX->loadRules( Custom_Syntax::ruleForFile(this->whatsThis().section("/",-1)) );
    SYNTAX->rehighlight();
  }
  bool ok = LUtils::writeFile(this->whatsThis(), this->toPlainText().split("\n"), true);
  hasChanges = !ok;
  if(ok){ lastSaveContents = this->toPlainText(); emit FileLoaded(this->whatsThis()); }
  //qDebug() << " - Success:" << ok << hasChanges;
}

QString PlainTextEditor::currentFile(){
  return this->whatsThis();
}

bool PlainTextEditor::hasChange(){
  return hasChanges;	
}

//Functions for managing the line number widget
int PlainTextEditor::LNWWidth(){
  //Get the number of chars we need for line numbers
  int lines = this->blockCount();
  if(lines<1){ lines = 1; }
  int chars = 1;
  while(lines>=10){ chars++; lines/=10; }
  return (this->fontMetrics().width("9")*chars); //make sure to add a tiny bit of padding
}

void PlainTextEditor::paintLNW(QPaintEvent *ev){
  QPainter P(LNW);
  //First set the background color
  P.fillRect(ev->rect(), QColor("lightgrey"));
  //Now determine which line numbers to show (based on the current viewport)
  QTextBlock block = this->firstVisibleBlock();
  int bTop = blockBoundingGeometry(block).translated(contentOffset()).top();
  int bBottom;
  //Now loop over the blocks (lines) and write in the numbers
  P.setPen(Qt::black); //setup the font color
  while(block.isValid() && bTop<=ev->rect().bottom()){ //ensure block below top of viewport
    bBottom = bTop+blockBoundingRect(block).height();
    if(block.isVisible() && bBottom >= ev->rect().top()){ //ensure block above bottom of viewport
      P.drawText(0,bTop, LNW->width(), this->fontMetrics().height(), Qt::AlignRight, QString::number(block.blockNumber()+1) );
    }
    //Go to the next block
    block = block.next();
    bTop = bBottom;
  }
}
	
//==============
//       PRIVATE
//==============
void PlainTextEditor::clearMatchData(){
  if(matchleft>=0 || matchright>=0){
    QList<QTextEdit::ExtraSelection> sel = this->extraSelections();
    for(int i=0; i<sel.length(); i++){
      if(sel[i].cursor.selectedText().length()==1){ sel.takeAt(i); i--; }
    }
    this->setExtraSelections(sel);
    matchleft = -1;
    matchright = -1;
  }
}

void PlainTextEditor::highlightMatch(QChar ch, bool forward, int fromPos){
  if(forward){ 
    matchleft = fromPos; 
    QTextCursor cur = this->document()->find(ch, fromPos);
    if(!cur.isNull()){ matchright = cur.position(); }
  }else{ 
    matchright = fromPos; 
    QTextCursor cur = this->document()->find(ch, fromPos, QTextDocument::FindBackward);
    if(!cur.isNull()){ matchleft = cur.position(); }	  
  }
  
  //Now highlight the two characters
  QList<QTextEdit::ExtraSelection> sels = this->extraSelections();	
  if(matchleft>=0){ 
    QTextEdit::ExtraSelection sel;
    if(matchright>=0){ sel.format.setBackground( QColor(settings->value("colors/bracket-found").toString()) ); }
    else{ sel.format.setBackground( QColor(settings->value("colors/bracket-missing").toString()) ); }
    QTextCursor cur = this->textCursor();
      cur.setPosition(matchleft);
      if(forward){ cur.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor); }
      else{ cur.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor); }
    sel.cursor = cur;
    sels << sel;
  }
  if(matchright>=0){
    QTextEdit::ExtraSelection sel;
    if(matchleft>=0){ sel.format.setBackground( QColor(settings->value("colors/bracket-found").toString()) ); }
    else{ sel.format.setBackground( QColor(settings->value("colors/bracket-missing").toString()) ); }
    QTextCursor cur = this->textCursor();
      cur.setPosition(matchright);
      if(!forward){ cur.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor); }
      else{ cur.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor); }
    sel.cursor = cur;
    sels << sel;	  
  }
  this->setExtraSelections(sels);
}

//===================
//       PRIVATE SLOTS
//===================
//Functions for managing the line number widget
void PlainTextEditor::LNW_updateWidth(){
  if(showLNW){
    this->setViewportMargins( LNWWidth(), 0, 0, 0); //the LNW is contained within the left margin
  }else{
    this->setViewportMargins( 0, 0, 0, 0); //the LNW is contained within the left margin
  }
}

void PlainTextEditor::LNW_highlightLine(){
  if(this->isReadOnly()){ return; }
  QColor highC = QColor(0,0,0,50); //just darken the line a bit
  QTextEdit::ExtraSelection sel;
  sel.format.setBackground(highC);
  sel.format.setProperty(QTextFormat::FullWidthSelection, true);
  sel.cursor = this->textCursor();
  sel.cursor.clearSelection(); //just in case it already has one
  setExtraSelections( QList<QTextEdit::ExtraSelection>() << sel );
}

void PlainTextEditor::LNW_update(const QRect &rect, int dy){
  if(dy!=0){ LNW->scroll(0,dy); } //make sure to scroll the line widget the same amount as the editor
  else{
    //Some other reason we need to repaint the widget
    LNW->update(0,rect.y(), LNW->width(), rect.height()); //also repaint the LNW in the same area
  }
  if(rect.contains(this->viewport()->rect())){
    //Something in the currently-viewed area needs updating - make sure the LNW width is still correct
    LNW_updateWidth();
  }
}

//Function for running the matching routine
void PlainTextEditor::checkMatchChar(){
  clearMatchData();
  int pos = this->textCursor().position();
  QChar ch = this->document()->characterAt(pos);
  if(ch==QChar('(')){ highlightMatch(QChar(')'),true, pos); }
  else if(ch==QChar(')')){ highlightMatch(QChar('('),false, pos); }
  else if(ch==QChar('{')){ highlightMatch(QChar('}'),true, pos); }
  else if(ch==QChar('}')){ highlightMatch(QChar('{'),false, pos); }
  else if(ch==QChar('[')){ highlightMatch(QChar(']'),true, pos); }
  else if(ch==QChar(']')){ highlightMatch(QChar('['),false, pos); }
}

//Functions for notifying the parent widget of changes
void PlainTextEditor::textChanged(){
  //qDebug() << " - Got Text Changed signal";
  bool changed = (lastSaveContents != this->toPlainText());
  if(changed == hasChanges){ return; } //no change
  hasChanges = changed; //save for reading later
  if(hasChanges){  emit UnsavedChanges( this->whatsThis() ); }
  else{ emit FileLoaded(this->whatsThis()); }
}

//==================
//       PROTECTED
//==================
void PlainTextEditor::resizeEvent(QResizeEvent *ev){
  QPlainTextEdit::resizeEvent(ev); //do the normal resize processing
  //Now re-adjust the placement of the LNW (within the left margin area)
  QRect cGeom = this->contentsRect();
  LNW->setGeometry( QRect(cGeom.left(), cGeom.top(), LNWWidth(), cGeom.height()) );
}
