/***************************************************************************
 *  qgsvbssearchbox.cpp                                                    *
 *  -------------------                                                    *
 *  begin                : Jul 09, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsapplication.h"
#include "qgscircularstringv2.h"
#include "qgscurvepolygonv2.h"
#include "qgsvbssearchbox.h"
#include "qgsmapcanvas.h"
#include "qgsmaptool.h"
#include "qgscoordinatetransform.h"
#include "qgsvbssearchprovider.h"
#include "qgsvbscoordinatesearchprovider.h"
#include "qgsvbslocationsearchprovider.h"
#include "qgsvbslocaldatasearchprovider.h"
#include "qgsvbsremotedatasearchprovider.h"
#include "qgsvbsworldlocationsearchprovider.h"
#include "qgsrubberband.h"
#include <QCheckBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QShortcut>
#include <QStyle>
#include <QToolButton>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QImageReader>


const int QgsVBSSearchBox::sEntryTypeRole = Qt::UserRole;
const int QgsVBSSearchBox::sCatNameRole = Qt::UserRole + 1;
const int QgsVBSSearchBox::sCatCountRole = Qt::UserRole + 2;
const int QgsVBSSearchBox::sResultDataRole = Qt::UserRole + 3;

// Overridden to make event() public
class QgsVBSSearchBox::LineEdit : public QLineEdit
{
  public:
    LineEdit( QWidget* parent ) : QLineEdit( parent ) {}
    bool event( QEvent *e ) override { return QLineEdit::event( e ); }
};

// Overridden to make event() public
class QgsVBSSearchBox::TreeWidget: public QTreeWidget
{
  public:
    TreeWidget( QWidget* parent ) : QTreeWidget( parent ) {}
    bool event( QEvent *e ) override { return QTreeWidget::event( e ); }
};


class QgsVBSSearchBox::FilterTool : public QgsMapTool
{
  public:
    enum Mode { Circle, Rect, Poly };

    FilterTool( QgsMapCanvas* canvas, Mode mode, QgsRubberBand* rubberBand )
        : QgsMapTool( canvas ), mMode( mode ), mRubberBand( rubberBand ), mCapturing( false )
    {
      setCursor( Qt::CrossCursor );
    }

    void canvasPressEvent( QMouseEvent * e ) override
    {
      if ( mCapturing && mMode == Poly )
      {
        if ( e->button() == Qt::RightButton )
        {
          mCapturing = false;
          mCanvas->unsetMapTool( this );
        }
        else
        {
          mRubberBand->addPoint( toMapCoordinates( e->pos() ) );
        }
      }
      else if ( e->button() == Qt::LeftButton )
      {
        mPressPos = e->pos();
        mCapturing = true;
        if ( mMode == Poly )
        {
          mRubberBand->addPoint( toMapCoordinates( e->pos() ) );
          mRubberBand->addPoint( toMapCoordinates( e->pos() ) );
        }
      }
    }
    void canvasMoveEvent( QMouseEvent * e ) override
    {
      if ( mCapturing )
      {
        if ( mMode == Circle )
        {
          QPoint diff = mPressPos - e->pos();
          double r = qSqrt( diff.x() * diff.x() + diff.y() + diff.y() );
          QgsCircularStringV2* exterior = new QgsCircularStringV2();
          exterior->setPoints(
            QList<QgsPointV2>() << toMapCoordinates( QPoint( mPressPos.x() + r, mPressPos.y() ) )
            << toMapCoordinates( mPressPos )
            << toMapCoordinates( QPoint( mPressPos.x() + r, mPressPos.y() ) ) );
          QgsCurvePolygonV2 geom;
          geom.setExteriorRing( exterior );
          QgsGeometry g( geom.segmentize() );
          mRubberBand->setToGeometry( &g, 0 );
        }
        else if ( mMode == Rect )
        {
          mRubberBand->setToCanvasRectangle( QRect( mPressPos, e->pos() ).normalized() );
        }
        else if ( mMode == Poly )
        {
          mRubberBand->movePoint( mRubberBand->partSize( 0 ) - 1, toMapCoordinates( e->pos() ) );
        }
      }
    }
    void canvasReleaseEvent( QMouseEvent * /*e*/ ) override
    {
      if ( mMode != Poly )
      {
        mCapturing = false;
        mCanvas->unsetMapTool( this );
      }
    }
    void deactivate() override
    {
      // Deactivated while capture, clear rubberband
      if ( mCapturing )
        mRubberBand->reset( QGis::Polygon );
      QgsMapTool::deactivate();
    }
    bool isEditTool() override { return true; }

  private:
    Mode mMode;
    QgsRubberBand* mRubberBand;
    bool mCapturing;
    QPoint mPressPos;
};


QgsVBSSearchBox::QgsVBSSearchBox( QgisInterface *iface, QWidget *parent )
    : QWidget( parent ), mIface( iface )
{
  mNumRunningProviders = 0;
  mRubberBand = 0;
  mFilterRubberBand = 0;

  mSearchBox = new LineEdit( this );

  mSearchBox->installEventFilter( this );

  mTreeWidget = new TreeWidget( mSearchBox );
  mTreeWidget->setWindowFlags( Qt::Popup );
  mTreeWidget->setFocusPolicy( Qt::NoFocus );
  mTreeWidget->setFrameStyle( QFrame::Box );
  mTreeWidget->setRootIsDecorated( false );
  mTreeWidget->setColumnCount( 1 );
  mTreeWidget->setEditTriggers( QTreeWidget::NoEditTriggers );
  mTreeWidget->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  mTreeWidget->setMouseTracking( true );
  mTreeWidget->setUniformRowHeights( true );
  mTreeWidget->header()->hide();
  mTreeWidget->installEventFilter( this );
  mTreeWidget->hide();

  mTimer.setSingleShot( true );
  mTimer.setInterval( 500 );

  mSearchButton = new QToolButton( mSearchBox );
  mSearchButton->setIcon( QIcon( ":/vbsfunctionality/icons/search.svg" ) );
  mSearchButton->setIconSize( QSize( 16, 16 ) );
  mSearchButton->setCursor( Qt::PointingHandCursor );
  mSearchButton->setStyleSheet( "QToolButton { border: none; padding: 0px; }" );
  mSearchButton->setToolTip( tr( "Search" ) );

  mClearButton = new QToolButton( mSearchBox );
  mClearButton->setIcon( QIcon( ":/vbsfunctionality/icons/clear.svg" ) );
  mClearButton->setIconSize( QSize( 16, 16 ) );
  mClearButton->setCursor( Qt::PointingHandCursor );
  mClearButton->setStyleSheet( "QToolButton { border: none; padding: 0px; }" );
  mClearButton->setToolTip( tr( "Clear" ) );
  mClearButton->setVisible( false );
  mClearButton->installEventFilter( this );

  QMenu* filterMenu = new QMenu( mSearchBox );
  QActionGroup* filterActionGroup = new QActionGroup( filterMenu );
  QAction* noFilterAction = new QAction( QIcon( ":/vbsfunctionality/icons/search_filter_none.svg" ), tr( "No filter" ), filterMenu );
  filterActionGroup->addAction( noFilterAction );
  connect( noFilterAction, SIGNAL( triggered( bool ) ), this, SLOT( clearFilter() ) );

  QAction* circleFilterAction = new QAction( QIcon( ":/vbsfunctionality/icons/search_filter_circle.svg" ), tr( "Filter by radius" ), filterMenu );
  circleFilterAction->setData( QVariant::fromValue( static_cast<int>( FilterTool::Circle ) ) );
  filterActionGroup->addAction( circleFilterAction );
  connect( circleFilterAction, SIGNAL( triggered( bool ) ), this, SLOT( setFilterTool() ) );

  QAction* rectangleFilterAction = new QAction( QIcon( ":/vbsfunctionality/icons/search_filter_rect.svg" ), tr( "Filter by rectangle" ), filterMenu );
  rectangleFilterAction->setData( QVariant::fromValue( static_cast<int>( FilterTool::Rect ) ) );
  filterActionGroup->addAction( rectangleFilterAction );
  connect( rectangleFilterAction, SIGNAL( triggered( bool ) ), this, SLOT( setFilterTool() ) );

  QAction* polygonFilterAction = new QAction( QIcon( ":/vbsfunctionality/icons/search_filter_poly.svg" ), tr( "Filter by polygon" ), filterMenu );
  polygonFilterAction->setData( QVariant::fromValue( static_cast<int>( FilterTool::Poly ) ) );
  filterActionGroup->addAction( polygonFilterAction );
  connect( polygonFilterAction, SIGNAL( triggered( bool ) ), this, SLOT( setFilterTool() ) );

  filterMenu->addActions( QList<QAction*>() << noFilterAction << circleFilterAction << rectangleFilterAction << polygonFilterAction );

  mFilterButton = new QToolButton( this );
  mFilterButton->setDefaultAction( noFilterAction );
  mFilterButton->setIconSize( QSize( 16, 16 ) );
  mFilterButton->setPopupMode( QToolButton::InstantPopup );
  mFilterButton->setCursor( Qt::PointingHandCursor );
  mFilterButton->setToolTip( tr( "Select Filter" ) );
  mFilterButton->setMenu( filterMenu );
  connect( filterMenu, SIGNAL( triggered( QAction* ) ), mFilterButton, SLOT( setDefaultAction( QAction* ) ) );

  setLayout( new QHBoxLayout );
  layout()->addWidget( mSearchBox );
  layout()->addWidget( mFilterButton );
  layout()->setContentsMargins( 0, 0, 0, 0 );
  layout()->setSpacing( 2 );

  connect( mSearchBox, SIGNAL( textEdited( QString ) ), this, SLOT( textChanged() ) );
  connect( mSearchButton, SIGNAL( clicked() ), this, SLOT( startSearch() ) );
  connect( &mTimer, SIGNAL( timeout() ), this, SLOT( startSearch() ) );
  connect( mTreeWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( resultSelected() ) );
  connect( mTreeWidget, SIGNAL( itemClicked( QTreeWidgetItem*, int ) ), this, SLOT( resultActivated() ) );
  connect( mTreeWidget, SIGNAL( itemActivated( QTreeWidgetItem*, int ) ), this, SLOT( resultActivated() ) );
  connect( mIface, SIGNAL( newProjectCreated() ), this, SLOT( clearSearch() ) );

  int frameWidth = mSearchBox->style()->pixelMetric( QStyle::PM_DefaultFrameWidth );
  mSearchBox->setStyleSheet( QString( "QLineEdit { padding-right: %1px; } " ).arg( mSearchButton->sizeHint().width() + frameWidth + 5 ) );
  QSize msz = mSearchBox->minimumSizeHint();
  mSearchBox->setMinimumSize( std::max( msz.width(), mSearchButton->sizeHint().height() + frameWidth * 2 + 2 ),
                              std::max( msz.height(), mSearchButton->sizeHint().height() + frameWidth * 2 + 2 ) );
  mSearchBox->setPlaceholderText( tr( "Search" ) );

  qRegisterMetaType<QgsVBSSearchProvider::SearchResult>( "QgsVBSSearchProvider::SearchResult" );
  addSearchProvider( new QgsVBSCoordinateSearchProvider( mIface ) );
  addSearchProvider( new QgsVBSLocationSearchProvider( mIface ) );
  addSearchProvider( new QgsVBSLocalDataSearchProvider( mIface ) );
  addSearchProvider( new QgsVBSRemoteDataSearchProvider( mIface ) );
  addSearchProvider( new QgsVBSWorldLocationSearchProvider( mIface ) );
}

QgsVBSSearchBox::~QgsVBSSearchBox()
{
  qDeleteAll( mSearchProviders );
}

void QgsVBSSearchBox::addSearchProvider( QgsVBSSearchProvider* provider )
{
  mSearchProviders.append( provider );
  connect( provider, SIGNAL( searchFinished() ), this, SLOT( searchProviderFinished() ) );
  connect( provider, SIGNAL( searchResultFound( QgsVBSSearchProvider::SearchResult ) ), this, SLOT( searchResultFound( QgsVBSSearchProvider::SearchResult ) ) );
}

void QgsVBSSearchBox::removeSearchProvider( QgsVBSSearchProvider* provider )
{
  mSearchProviders.removeAll( provider );
  disconnect( provider, SIGNAL( searchFinished() ), this, SLOT( searchProviderFinished() ) );
  disconnect( provider, SIGNAL( searchResultFound( QgsVBSSearchProvider::SearchResult ) ), this, SLOT( searchResultFound( QgsVBSSearchProvider::SearchResult ) ) );
}

bool QgsVBSSearchBox::eventFilter( QObject* obj, QEvent* ev )
{
  if ( obj == mSearchBox && ev->type() == QEvent::FocusIn )
  {
    mTreeWidget->resize( mSearchBox->width(), 200 );
    mTreeWidget->move( mSearchBox->mapToGlobal( QPoint( 0, mSearchBox->height() ) ) );
    mTreeWidget->show();
    if ( !mClearButton->isVisible() )
      resultSelected();
    if ( mFilterRubberBand )
      mFilterRubberBand->setVisible( true );
    return true;
  }
  else if ( obj == mSearchBox && ev->type() == QEvent::MouseButtonPress )
  {
    mSearchBox->selectAll();
    return true;
  }
  else if ( obj == mSearchBox && ev->type() == QEvent::Resize )
  {
    int frameWidth = mSearchBox->style()->pixelMetric( QStyle::PM_DefaultFrameWidth );
    QRect r = mSearchBox->rect();
    QSize sz = mSearchButton->sizeHint();
    mSearchButton->move(( r.right() - frameWidth - sz.width() - 4 ),
                        ( r.bottom() + 1 - sz.height() ) / 2 );
    sz = mClearButton->sizeHint();
    mClearButton->move(( r.right() - frameWidth - sz.width() - 4 ),
                       ( r.bottom() + 1 - sz.height() ) / 2 );
    return true;
  }
  else if ( obj == mClearButton && ev->type() == QEvent::MouseButtonPress )
  {
    clearSearch();
    return true;
  }
  else if ( obj == mTreeWidget && ev->type() == QEvent::Close )
  {
    cancelSearch();
    mSearchBox->clearFocus();
    if ( mFilterRubberBand )
      mFilterRubberBand->setVisible( false );
    return true;
  }
  else if ( obj == mTreeWidget && ev->type() == QEvent::MouseButtonPress )
  {
    mTreeWidget->close();
    return true;
  }
  else if ( obj == mTreeWidget && ev->type() == QEvent::KeyPress )
  {
    int key = static_cast<QKeyEvent*>( ev )->key();
    if ( key == Qt::Key_Escape )
    {
      mTreeWidget->close();
      return true;
    }
    else if ( key == Qt::Key_Enter || key == Qt::Key_Return )
    {
      if ( mTimer.isActive() )
      {
        // Search text was changed
        startSearch();
        return true;
      }
    }
    else if ( key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_PageUp || key == Qt::Key_PageDown )
      return mTreeWidget->event( ev );
    else
      return mSearchBox->event( ev );
  }
  else
  {
    return QWidget::eventFilter( obj, ev );
  }
  return false;
}

void QgsVBSSearchBox::textChanged()
{
  mSearchButton->setVisible( true );
  mClearButton->setVisible( false );
  cancelSearch();
  mTimer.start();
}

void QgsVBSSearchBox::startSearch()
{
  mTimer.stop();

  mTreeWidget->blockSignals( true );
  mTreeWidget->clear();
  mTreeWidget->blockSignals( false );

  QString searchtext = mSearchBox->text();
  if ( searchtext.size() < 3 )
    return;

  mNumRunningProviders = mSearchProviders.count();

  QgsVBSSearchProvider::SearchRegion searchRegion;
  if ( mFilterRubberBand )
  {
    for ( int i = 0, n = mFilterRubberBand->partSize( 0 ); i < n; ++i )
    {
      searchRegion.polygon.append( *mFilterRubberBand->getPoint( 0, i ) );
    }
    searchRegion.polygon.append( *mFilterRubberBand->getPoint( 0, 0 ) );
    searchRegion.crs = mIface->mapCanvas()->mapSettings().destinationCrs();
  }

  foreach ( QgsVBSSearchProvider* provider, mSearchProviders )
    provider->startSearch( searchtext, searchRegion );
}

void QgsVBSSearchBox::clearSearch()
{
  mSearchBox->clear();
  mSearchButton->setVisible( true );
  mClearButton->setVisible( false );
  mIface->mapCanvas()->scene()->removeItem( mRubberBand );
  delete mRubberBand;
  mRubberBand = 0;
  mTreeWidget->close();
  mTreeWidget->blockSignals( true );
  mTreeWidget->clear();
  mTreeWidget->blockSignals( false );
}

void QgsVBSSearchBox::searchProviderFinished()
{
  --mNumRunningProviders;
}

void QgsVBSSearchBox::searchResultFound( QgsVBSSearchProvider::SearchResult result )
{
  // Search category item
  QTreeWidgetItem* categoryItem = 0;
  QTreeWidgetItem* root = mTreeWidget->invisibleRootItem();
  for ( int i = 0, n = root->childCount(); i < n; ++i )
  {
    if ( root->child( i )->data( 0, sCatNameRole ).toString() == result.category )
      categoryItem = root->child( i );
  }

  // If category does not exist, create it
  if ( !categoryItem )
  {
    int pos = 0;
    for ( int i = 0, n = root->childCount(); i < n; ++i )
    {
      if ( result.category.compare( root->child( i )->data( 0, sCatNameRole ).toString() ) < 0 )
      {
        break;
      }
      ++pos;
    }
    categoryItem = new QTreeWidgetItem();
    categoryItem->setData( 0, sEntryTypeRole, EntryTypeCategory );
    categoryItem->setData( 0, sCatNameRole, result.category );
    categoryItem->setData( 0, sCatCountRole, 0 );
    categoryItem->setFlags( Qt::ItemIsEnabled );
    QFont font = categoryItem->font( 0 );
    font.setBold( true );
    categoryItem->setFont( 0, font );
    root->insertChild( pos, categoryItem );
    categoryItem->setExpanded( true );
  }

  // Insert new result
  QTreeWidgetItem* resultItem = new QTreeWidgetItem();
  resultItem->setData( 0, Qt::DisplayRole, result.text );
  resultItem->setData( 0, sEntryTypeRole, EntryTypeResult );
  resultItem->setData( 0, sResultDataRole, QVariant::fromValue( result ) );
  resultItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );

  categoryItem->addChild( resultItem );
  int categoryCount = categoryItem->data( 0, sCatCountRole ).toInt() + 1;
  categoryItem->setData( 0, sCatCountRole, categoryCount );
  categoryItem->setData( 0, Qt::DisplayRole, QString( "%1 (%2)" ).arg( categoryItem->data( 0, sCatNameRole ).toString() ).arg( categoryCount ) );
}

void QgsVBSSearchBox::resultSelected()
{
  if ( mTreeWidget->currentItem() )
  {
    QTreeWidgetItem* item = mTreeWidget->currentItem();
    if ( item->data( 0, sEntryTypeRole ) != EntryTypeResult )
      return;

    QgsVBSSearchProvider::SearchResult result = item->data( 0, sResultDataRole ).value<QgsVBSSearchProvider::SearchResult>();
    if ( !mRubberBand )
      createRubberBand();
    QgsCoordinateTransform t( result.crs, mIface->mapCanvas()->mapSettings().destinationCrs() );
    mRubberBand->setToGeometry( QgsGeometry::fromPoint( t.transform( result.pos ) ), 0 );
    mSearchBox->blockSignals( true );
    mSearchBox->setText( result.text );
    mSearchBox->blockSignals( false );
    mSearchButton->setVisible( true );
    mClearButton->setVisible( false );
    mIface->mapCanvas()->refresh();
  }
}

void QgsVBSSearchBox::resultActivated()
{
  if ( mTreeWidget->currentItem() )
  {
    QTreeWidgetItem* item = mTreeWidget->currentItem();
    if ( item->data( 0, sEntryTypeRole ) != EntryTypeResult )
      return;

    QgsVBSSearchProvider::SearchResult result = item->data( 0, sResultDataRole ).value<QgsVBSSearchProvider::SearchResult>();
    QgsRectangle zoomExtent;
    if ( result.bbox.isEmpty() )
    {
      zoomExtent = mIface->mapCanvas()->mapSettings().computeExtentForScale( result.pos, result.zoomScale, result.crs );
      if ( !mRubberBand )
        createRubberBand();
      QgsCoordinateTransform t( result.crs, mIface->mapCanvas()->mapSettings().destinationCrs() );
      mRubberBand->setToGeometry( QgsGeometry::fromPoint( t.transform( result.pos ) ), 0 );
    }
    else
    {
      QgsCoordinateTransform t( result.crs, mIface->mapCanvas()->mapSettings().destinationCrs() );
      zoomExtent = t.transform( result.bbox );
    }
    mIface->mapCanvas()->setExtent( zoomExtent );
    mIface->mapCanvas()->refresh();
    mSearchBox->blockSignals( true );
    mSearchBox->setText( result.text );
    mSearchBox->blockSignals( false );
    mSearchButton->setVisible( false );
    mClearButton->setVisible( true );
    mTreeWidget->close();
  }
}

void QgsVBSSearchBox::createRubberBand()
{
  mRubberBand = new QgsRubberBand( mIface->mapCanvas(), QGis::Point );
  QSize imgSize = QImageReader( ":/vbsfunctionality/icons/pin_blue.svg" ).size();
  mRubberBand->setSvgIcon( ":/vbsfunctionality/icons/pin_blue.svg", QPoint( -imgSize.width() / 2., -imgSize.height() ) );
}

void QgsVBSSearchBox::cancelSearch()
{
  foreach ( QgsVBSSearchProvider* provider, mSearchProviders )
  {
    provider->cancelSearch();
  }
  // If the clear button is visible, the rubberband marks an activated search
  // result, which can be cleared by pressing the clear button
  if ( mRubberBand && !mClearButton->isVisible() )
  {
    mIface->mapCanvas()->scene()->removeItem( mRubberBand );
    mIface->mapCanvas()->refresh();
    delete mRubberBand;
    mRubberBand = 0;
  }
}

void QgsVBSSearchBox::clearFilter()
{
  if ( mFilterRubberBand != 0 )
  {
    delete mFilterRubberBand;
    mFilterRubberBand = 0;
    // Trigger a new search since the filter changed
    startSearch();
  }
}

void QgsVBSSearchBox::setFilterTool()
{
  QAction* action = qobject_cast<QAction*>( QObject::sender() );
  FilterTool::Mode mode = static_cast<FilterTool::Mode>( action->data().toInt() );
  delete mFilterRubberBand;
  mFilterRubberBand = new QgsRubberBand( mIface->mapCanvas(), QGis::Polygon );
  mFilterRubberBand->setFillColor( QColor( 254, 178, 76, 63 ) );
  mFilterRubberBand->setBorderColor( QColor( 254, 58, 29, 100 ) );
  FilterTool* tool = new FilterTool( mIface->mapCanvas(), mode, mFilterRubberBand );
  mIface->mapCanvas()->setMapTool( tool );
  action->setCheckable( true );
  action->setChecked( true );
  connect( tool, SIGNAL( deactivated() ), this, SLOT( filterToolFinished() ) );
  connect( tool, SIGNAL( deactivated() ), tool, SLOT( deleteLater() ) );
}

void QgsVBSSearchBox::filterToolFinished()
{
  mFilterButton->defaultAction()->setChecked( false );
  mFilterButton->defaultAction()->setCheckable( false );
  if ( mFilterRubberBand && mFilterRubberBand->partSize( 0 ) > 0 )
  {
    mSearchBox->setFocus();
    // Trigger a new search since the filter changed
    startSearch();
  }
  else
  {
    mFilterButton->setDefaultAction( mFilterButton->menu()->actions().first() );
    clearFilter();
  }
}