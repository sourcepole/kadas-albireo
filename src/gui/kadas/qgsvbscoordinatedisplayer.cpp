/***************************************************************************
 *  qgsvbscoordinatedisplayer.cpp                                          *
 *  -------------------                                                    *
 *  begin                : Jul 13, 2015                                    *
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

#include "qgsvbscoordinatedisplayer.h"
#include "qgsvbscoordinateconverter.h"
#include "qgsmapcanvas.h"
#include "qgsmapsettings.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QStatusBar>


template<class T>
static inline QVariant ptr2variant( T* ptr )
{
  return QVariant::fromValue<void*>( reinterpret_cast<void*>( ptr ) );
}

template<class T>
static inline T* variant2ptr( const QVariant& v )
{
  return reinterpret_cast<T*>( v.value<void*>() );
}


QgsVBSCoordinateDisplayer::QgsVBSCoordinateDisplayer( QComboBox* crsComboBox, QLineEdit* coordLineEdit, QgsMapCanvas* mapCanvas,
    QWidget *parent ) : QWidget( parent ), mMapCanvas( mapCanvas ),
    mCRSSelectionCombo( crsComboBox ), mCoordinateLineEdit( coordLineEdit )
{
  setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );

  mIconLabel = new QLabel( this );
  mIconLabel->setPixmap( QPixmap( ":/vbsfunctionality/icons/mousecoordinates.svg" ) );

  mCRSSelectionCombo->addItem( "LV03", ptr2variant( new QgsEPSGCoordinateConverter( "EPSG:21781", mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "LV95", ptr2variant( new QgsEPSGCoordinateConverter( "EPSG:2056", mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "DMS", ptr2variant( new QgsWGS84CoordinateConverter( QgsWGS84CoordinateConverter::DegMinSec, mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "DM", ptr2variant( new QgsWGS84CoordinateConverter( QgsWGS84CoordinateConverter::DegMin, mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "DD", ptr2variant( new QgsWGS84CoordinateConverter( QgsWGS84CoordinateConverter::DecDeg, mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "UTM", ptr2variant( new QgsUTMCoordinateConverter( mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "MGRS", ptr2variant( new QgsMGRSCoordinateConverter( mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );
  mCRSSelectionCombo->setCurrentIndex( 0 );

  QFont font = mCoordinateLineEdit->font();
  font.setPointSize( 9 );
  mCoordinateLineEdit->setFont( font );
  mCoordinateLineEdit->setReadOnly( true );
  mCoordinateLineEdit->setAlignment( Qt::AlignCenter );
  mCoordinateLineEdit->setFixedWidth( 200 );
  mCoordinateLineEdit->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );

  connect( mMapCanvas, SIGNAL( xyCoordinates( QgsPoint ) ), this, SLOT( displayCoordinates( QgsPoint ) ) );
  connect( mMapCanvas, SIGNAL( destinationCrsChanged() ), this, SLOT( syncProjectCrs() ) );
  connect( mCRSSelectionCombo, SIGNAL( currentIndexChanged( int ) ), mCoordinateLineEdit, SLOT( clear() ) );
  connect( mCRSSelectionCombo, SIGNAL( currentIndexChanged( int ) ), this, SIGNAL( displayFormatChanged() ) );

  syncProjectCrs();
}

QgsVBSCoordinateDisplayer::~QgsVBSCoordinateDisplayer()
{
  disconnect( mMapCanvas, SIGNAL( xyCoordinates( QgsPoint ) ), this, SLOT( displayCoordinates( QgsPoint ) ) );
  disconnect( mMapCanvas, SIGNAL( destinationCrsChanged() ), this, SLOT( syncProjectCrs() ) );
}

QString QgsVBSCoordinateDisplayer::getDisplayString( const QgsPoint& p, const QgsCoordinateReferenceSystem& crs )
{
  if ( mCRSSelectionCombo )
  {
    QVariant v = mCRSSelectionCombo->itemData( mCRSSelectionCombo->currentIndex() );
    QgsVBSCoordinateConverter* conv = variant2ptr<QgsVBSCoordinateConverter>( v );
    if ( conv )
    {
      return conv->convert( p, crs );
    }
  }
  return QString();
}

void QgsVBSCoordinateDisplayer::displayCoordinates( const QgsPoint &p )
{
  if ( mCoordinateLineEdit )
  {
    mCoordinateLineEdit->setText( getDisplayString( p, mMapCanvas->mapSettings().destinationCrs() ) );
  }
}

void QgsVBSCoordinateDisplayer::syncProjectCrs()
{
  const QgsCoordinateReferenceSystem& crs = mMapCanvas->mapSettings().destinationCrs();
  if ( crs.srsid() == 4326 )
  {
    mCRSSelectionCombo->setCurrentIndex( mCRSSelectionCombo->findText( "DMS" ) );
  }
  else if ( crs.srsid() == 21781 )
  {
    mCRSSelectionCombo->setCurrentIndex( mCRSSelectionCombo->findText( "LV03" ) );
  }
  else if ( crs.srsid() == 2056 )
  {
    mCRSSelectionCombo->setCurrentIndex( mCRSSelectionCombo->findText( "LV95" ) );
  }
}