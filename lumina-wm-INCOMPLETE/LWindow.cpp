//===========================================
//  Lumina-DE source code
//  Copyright (c) 2015, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "LWindow.h"

LWindow::LWindow(WId client) : QFrame(){
  activeState = LWindow::Normal;
  CID = client;
  qDebug() << "New Window:" << CID << "Frame:" << this->winId();
  this->setMouseTracking(true); //need this to determine mouse location when not clicked
  this->setObjectName("LWindowFrame");
  this->setStyleSheet("LWindow#LWindowFrame{ border: 2px solid white; border-radius:3px; } QWidget#TitleBar{background: grey; } QLabel{ color: black; }");
  //this->setAttribute(Qt::WA_TranslucentBackground, true);
  //this->setAttribute(Qt::WA_StyledBackground, true);
  this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint); //Qt::X11BypassWindowManagerHint); //ensure that this frame does not get a frame itself
  InitWindow(); //initially create all the child widgets
  updateAppearance(); //this loads the appearance based on window/theme settings
  QApplication::processEvents();
  //Now set the frame size on this window
  SyncSize();
  SyncText();
  //LWM::SYSTEM->RestoreWindow(CID);
  this->show();
}

LWindow::~LWindow(){
	
}

// =================
//         PUBLIC
// =================
//Return the ID of the managed window for the current graphics system (X11/Wayland/other)
WId LWindow::clientID(){ return CID; }

bool LWindow::hasFrame(){ return this->isEnabled(); }

// =================
//        PRIVATE
// =================
void LWindow::InitWindow(){
	anim = new QPropertyAnimation(this); //For simple window animations
	  anim->setTargetObject(this);
	  anim->setDuration(ANIMTIME); //In milliseconds
	titleBar = new QLabel(this); //This is the "container" for all the title buttons/widgets
	  titleBar->setObjectName("TitleBar");
	  titleBar->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
	title = new QLabel(this); //Shows the window title/text
	  title->setObjectName("Title");
	  title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	  title->setCursor(Qt::ArrowCursor);
	icon = new QLabel(this); //Contains the window icon
	  icon->setObjectName("Icon");
	  icon->setCursor(Qt::ArrowCursor);
	minB = new QToolButton(this); //Minimize Button
	  minB->setObjectName("Minimize");
	  minB->setCursor(Qt::ArrowCursor);
	  connect(minB, SIGNAL(clicked()), this, SLOT(minClicked()) );
	maxB = new QToolButton(this); //Maximize Button
	  maxB->setObjectName("Maximize");
	  maxB->setCursor(Qt::ArrowCursor);
	  connect(maxB, SIGNAL(clicked()), this, SLOT(maxClicked()) );
	closeB = new QToolButton(this);
	  closeB->setObjectName("Close");
	  closeB->setCursor(Qt::ArrowCursor);
	  connect(closeB, SIGNAL(clicked()), this, SLOT(closeClicked()) );
	otherB = new QToolButton(this); //Button to place any other actions
	  otherB->setObjectName("Options");
	  otherB->setCursor(Qt::ArrowCursor);
	  otherB->setPopupMode(QToolButton::InstantPopup);
	  otherB->setStyleSheet("QToolButton::menu-indicator{ image: none; }");
	otherM = new QMenu(this); //menu of "other" actions for the window
	  otherB->setMenu(otherM);
	  connect(otherM, SIGNAL(triggered(QAction*)), this, SLOT(otherClicked(QAction*)) );
	//Now assemble the titlebar
	QHBoxLayout *HL = new QHBoxLayout(this);
	HL->setContentsMargins(0,0,0,0);
	HL->addWidget(otherB);
	HL->addWidget(icon);
	HL->addWidget(title);
	HL->addWidget(minB);
	HL->addWidget(maxB);
	HL->addWidget(closeB);
	titleBar->setLayout(HL);
	QVBoxLayout *VL = new QVBoxLayout(this);
	this->setLayout(VL);
	VL->addWidget(titleBar);
	VL->setAlignment(titleBar, Qt::AlignTop);
	VL->setContentsMargins(0,0,0,0);
	VL->setSpacing(0);
	//Now embed the native window into the frame
	WIN = QWindow::fromWinId(CID);
	//WINBACK = new QBackingStore(WIN); //create a data backup for the widget
	this->layout()->addWidget( QWidget::createWindowContainer( WIN, this) );
	//this->layout()-> //set the container as expanding
	VL->setStretch(1,1);
}

LWindow::ModState LWindow::getStateAtPoint(QPoint pt, bool setoffset){
  //Note: pt should be in widget-relative coordinates, not global
  if(!this->layout()->geometry().contains(pt)){
    //above the frame itself - need to figure out which quadrant it is in (8-directions)
    if(pt.y() < this->height()/3){
      //One of the top options
      if(pt.x() < this->width()/3){ 
	if(setoffset){ offset.setX(pt.x()); offset.setY(pt.y()); } //difference from top-left corner
	return ResizeTopLeft;
      }else if(pt.x() > (2*this->width()/3)){ 
	if(setoffset){ offset.setX(this->width()-pt.x()); offset.setY(pt.y()); } //difference from top-right corner
	return ResizeTopRight;
      }else{ 
	if(setoffset){ offset.setX(0); offset.setY(pt.y()); } //difference from top edge (X does not matter)
	return ResizeTop; 
      }		    
    }else if(pt.y() > (2*this->height())/3){
      //One of the bottom options
      if(pt.x() < this->width()/3){ 
	if(setoffset){ offset.setX(pt.x()); offset.setY(this->height()-pt.y()); } //difference from bottom-left corner
	return ResizeBottomLeft;
      }else if(pt.x() > (2*this->width()/3)){ 
	if(setoffset){ offset.setX(this->width()-pt.x()); offset.setY(this->height()-pt.y()); } //difference from bottom-right corner
	return ResizeBottomRight;
      }else{ 
	if(setoffset){ offset.setX(0); offset.setY(this->height() - pt.y()); } //difference from bottom edge (X does not matter)
	return ResizeBottom; 
      }	
    }else{
      //One of the side options
      if(pt.x() < this->width()/2){ 
	if(setoffset){ offset.setX(pt.x()); offset.setY(0); } //difference from left edge (Y does not matter)
	return ResizeLeft;
      }else{ 
	if(setoffset){ offset.setX(this->width()-pt.x()); offset.setY(0); } //difference from right edge (Y does not matter)
	return ResizeRight;
      }
    }
  }
  return Normal;
}

void LWindow::setMouseCursor(ModState state, bool override){
  Qt::CursorShape shape;
  switch(state){
    case Normal:
      shape = Qt::ArrowCursor;
      break;
    case Move:
      shape = Qt::SizeAllCursor;
      break;
    case ResizeTop:
      shape = Qt::SizeVerCursor;
      break;
    case ResizeTopRight:
      shape = Qt::SizeBDiagCursor;
      break;
    case ResizeRight:
      shape = Qt::SizeHorCursor;
      break;
    case ResizeBottomRight:
      shape = Qt::SizeFDiagCursor;
      break;
    case ResizeBottom:
      shape = Qt::SizeVerCursor;
      break;
    case ResizeBottomLeft:
      shape = Qt::SizeBDiagCursor;
      break;
    case ResizeLeft:
      shape = Qt::SizeHorCursor;
      break;
    case ResizeTopLeft:
      shape = Qt::SizeFDiagCursor;
      break;	    
  }
  if(override){
    QApplication::setOverrideCursor(QCursor(shape));
  }else{
    this->setCursor(shape);
  }
}

// ==========================
//   WINDOW INTERACTIONS
//==========================
void LWindow::SyncSize(){ 
  //sync the window/frame geometries (generally only done before embedding the client window)
    int frame =  this->frameWidth();
    int TH = titleBar->height();
    //SYSTEM->SetFrameValues(CID, frame, frame, frame, frame);
    //Now load the information about the window and adjust the frame to match
    lastGeom = LWM::SYSTEM->WindowGeometry(CID,false);
    qDebug() << "Initial Size:" << lastGeom << frame << TH;
    //Add in the frame size
    lastGeom.moveTop(lastGeom.y()-frame-TH);
    lastGeom.setHeight(lastGeom.height()+(2*frame)+TH);
    lastGeom.moveLeft(lastGeom.x()-frame);
    lastGeom.setWidth( lastGeom.width()+(2*frame));
    qDebug() << " - With Frame:" << lastGeom;
    //Now adjust for a out-of-bounds location
    if(lastGeom.x() < 0){ lastGeom.moveLeft(0); }
    if(lastGeom.y() < 0){ lastGeom.moveTop(0); }
    qDebug() << " - Adjusted:" << lastGeom;
    this->setGeometry(lastGeom);
}

void LWindow::SyncText(){
  QString txt = WIN->title();
  if(txt.isEmpty()){ txt = LWM::SYSTEM->WindowName(CID); }
  if(txt.isEmpty()){ txt = LWM::SYSTEM->OldWindowName(CID); }
  if(txt.isEmpty()){ txt = LWM::SYSTEM->WindowVisibleName(CID); }
  if(txt.isEmpty()){ txt = LWM::SYSTEM->WindowIconName(CID); }
  if(txt.isEmpty()){ txt = LWM::SYSTEM->WindowVisibleIconName(CID); }
  title->setText(txt);
}

//  SIMPLE ANIMATIONS
void LWindow::showAnimation(LWM::WindowAction act){
  bool useanimation = true; //placeholder for the actual setting check
  //Setup the animation routine
  if(act==LWM::Show){
    if(useanimation){
      lastGeom = this->geometry();
      //Expand out from center point
      anim->setPropertyName("geometry");
      anim->setStartValue( QRect(lastGeom.center(), QSize(0,0) ) );
      anim->setEndValue( this->geometry() );
      //Fade in gradually
      //anim->setPropertyName("windowOpacity");
      //anim->setStartValue( 0.0 );
      //anim->setEndValue( 1.0 );
    }else{
      this->show(); //just show it right away
    }
    
  }else if(act==LWM::Hide){
    if(useanimation){
       //Collapse in on center point
      lastGeom = this->geometry();
      anim->setPropertyName("geometry");
      anim->setStartValue( QRect(this->geometry()) );
      anim->setEndValue( QRect(this->geometry().center(), QSize(0,0) ) );
    }else{
      this->hide(); //just hide it right away
    }
  }else if(act==LWM::Closed){
    //Need to clean up the container widget first to prevent XCB errors
    //qDebug() << "Window Closed:" << WIN->winId() << CID;
    //WIN->destroy(); //clean up any data
    if(this->layout()->count()>1){ delete this->layout()->takeAt(1); }
    if(useanimation){
       //Collapse in on center line
      lastGeom = this->geometry();
      anim->setPropertyName("geometry");
      anim->setStartValue( QRect(this->geometry()) );
      anim->setEndValue( QRect(this->geometry().x(), this->geometry().center().y(), this->width(), 0 ) );
    }else{
      this->close(); //just hide it right away
    }
  }
  if(useanimation){ 
    this->show();
    anim->start(); 
    //Also set any final values
    if(act==LWM::Hide){ QTimer::singleShot(ANIMTIME, this, SLOT(hide()) ); }
    if(act==LWM::Closed){ QTimer::singleShot(ANIMTIME, this, SLOT(close()) ); }
  };
}
// =================
//    PUBLIC SLOTS
// =================
void LWindow::updateAppearance(){
  //Reload any button icons and such
  minB->setIcon(LXDG::findIcon("window-suppressed",""));
  maxB->setIcon(LXDG::findIcon("view-fullscreen",""));
  closeB->setIcon(LXDG::findIcon("application-exit",""));
  otherB->setIcon(LXDG::findIcon("configure",""));
}

void LWindow::windowChanged(LWM::WindowAction act){
  //A window property was changed - update accordingly
  switch(act){
    case LWM::Hide:
    case LWM::Show:
    case LWM::Closed:
	showAnimation(act);
	break;
    case LWM::MoveResize:
	//Re-adjust to the new position/size of the window
	
	break;
  }
}
// =================
//    PRIVATE SLOTS
// =================
void LWindow::closeClicked(){
  qDebug() << "Closing Window";
  //First try the close event to let the client app do cleanup/etc
  WIN->hide();
  LWM::SYSTEM->WM_CloseWindow(CID);
  showAnimation(LWM::Closed); //temporary testing line - should be run after destroy event automatically later
}

void LWindow::minClicked(){
  qDebug() << "Minimize Window";
  this->showMinimized();
}

void LWindow::maxClicked(){
  if(normalGeom.isNull()){
    qDebug() << "Maximize Window";
    normalGeom = this->geometry(); //save for later
    this->showMaximized();
  }else{
    qDebug() << "Restore Window";
    this->showNormal();
    this->setGeometry(normalGeom);
    normalGeom = QRect(); //clear it
  }
}

void LWindow::otherClicked(QAction* act){
  QString action = act->whatsThis();
}

// =====================
//         PROTECTED
// =====================
void LWindow::mousePressEvent(QMouseEvent *ev){
  qDebug() << "Mouse Press Event";
  offset.setX(0); offset.setY(0);
  if(activeState != Normal){ return; } // do nothing - already in a state of grabbed mouse
  if(this->childAt(ev->pos())!=0){
    //Check for any non-left-click event and skip it
    if(ev->button()!=Qt::LeftButton){ return; }
    activeState = Move;
    offset.setX(ev->pos().x()); offset.setY(ev->pos().y());
  }else{
    //Clicked on the frame somewhere
    activeState = getStateAtPoint(ev->pos(), true); //also have it set the offset variable
  }
  setMouseCursor(activeState, true); //this one is an override cursor
  
}

void LWindow::mouseMoveEvent(QMouseEvent *ev){
  ev->accept();
  if(activeState == Normal){
    setMouseCursor( getStateAtPoint(ev->pos()) ); //just update the mouse cursor

  }else{
    //Currently in a modification state
    QRect geom = this->geometry();
    switch(activeState){
      case Move:
        geom.moveTopLeft(ev->globalPos()-offset); //will not change size
        break;
      case ResizeTop:
        geom.setTop(ev->globalPos().y()-offset.y());
        break;
      case ResizeTopRight:
        geom.setTopRight(ev->globalPos()-offset);
        break;
      case ResizeRight:
        geom.setRight(ev->globalPos().x()-offset.x());
        break;
      case ResizeBottomRight:
        geom.setBottomRight(ev->globalPos()-offset);
        break;
      case ResizeBottom:
        geom.setBottom(ev->globalPos().y()-offset.y());
        break;
      case ResizeBottomLeft:
        geom.setBottomLeft(ev->globalPos()-offset);
        break;
      case ResizeLeft:
        geom.setLeft(ev->globalPos().x()-offset.x());
        break;
      case ResizeTopLeft:
        geom.setTopLeft(ev->globalPos()-offset);
        break;	    
    }
    this->setGeometry(geom);
  }
}

void LWindow::mouseReleaseEvent(QMouseEvent *ev){
  //Check for a right-click event
  ev->accept();
  if( (activeState==Normal) && (this->childAt(ev->pos())==titleBar) && (ev->button()==Qt::RightButton) ){
    otherM->popup(ev->globalPos());
    return;
  }
  activeState = Normal;
  QApplication::restoreOverrideCursor();
  setMouseCursor( getStateAtPoint(ev->pos()) );
}