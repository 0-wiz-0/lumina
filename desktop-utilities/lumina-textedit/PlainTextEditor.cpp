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

//==============
//       PUBLIC
//==============
PlainTextEditor::PlainTextEditor(QWidget *parent) : QPlainTextEdit(parent){
  LNW = new LNWidget(this);
  showLNW = true;
  connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(LNW_updateWidth()) );
  connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(LNW_highlightLine()) );
  connect(this, SIGNAL(updateRequest(const QRect&, int)), this, SLOT(LNW_update(const QRect&, int)) );
	
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

//Functions for managing the line number widget
int PlainTextEditor::LNWWidth(){
  //Get the number of chars we need for line numbers
  int lines = this->blockCount();
  if(lines<1){ lines = 1; }
  int chars = 1;
  while(lines>=10){ chars++; lines/=10; }
  return (this->fontMetrics().width("9")*chars + 4); //make sure to add a tiny bit of padding
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

//==================
//       PROTECTED
//==================
void PlainTextEditor::resizeEvent(QResizeEvent *ev){
  QPlainTextEdit::resizeEvent(ev); //do the normal resize processing
  //Now re-adjust the placement of the LNW (within the left margin area)
  QRect cGeom = this->contentsRect();
  LNW->setGeometry( QRect(cGeom.left(), cGeom.top(), LNWWidth(), cGeom.height()) );
}
