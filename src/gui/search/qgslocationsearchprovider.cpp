/***************************************************************************
 *  qgslocationsearchprovider.cpp                                       *
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

#include "qgslocationsearchprovider.h"
#include "qgsnetworkaccessmanager.h"
#include "qgscoordinatetransform.h"
#include "qgslogger.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSettings>
#include <qjson/parser.h>


const int QgsLocationSearchProvider::sSearchTimeout = 2000;
const int QgsLocationSearchProvider::sResultCountLimit = 50;


QgsLocationSearchProvider::QgsLocationSearchProvider( QgsMapCanvas* mapCanvas )
    : QgsSearchProvider( mapCanvas )
{
  mNetReply = 0;

  mCategoryMap.insert( "sn25", tr( "Places" ) );
  mCategoryMap.insert( "gg25", tr( "Municipalities" ) );
  mCategoryMap.insert( "kantone", tr( "Cantons" ) );
  mCategoryMap.insert( "district", tr( "Districts" ) );
  mCategoryMap.insert( "address", tr( "Address" ) );
  mCategoryMap.insert( "zipcode", tr( "Zip Codes" ) );

  mPatBox = QRegExp( "^BOX\\s*\\(\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*,\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*\\)$" );

  mTimeoutTimer.setSingleShot( true );
  connect( &mTimeoutTimer, SIGNAL( timeout() ), this, SLOT( replyFinished() ) );
}

void QgsLocationSearchProvider::startSearch( const QString &searchtext , const SearchRegion &/*searchRegion*/ )
{
  QUrl url( QSettings().value( "search/locationsearchurl", "https://api3.geo.admin.ch/rest/services/api/SearchServer" ).toString() );
  url.addQueryItem( "type", "locations" );
  url.addQueryItem( "searchText", searchtext );
  url.addQueryItem( "limit", QString::number( sResultCountLimit ) );

  QNetworkRequest req( url );
  req.setRawHeader( "Referer", QSettings().value( "search/referrer", "http://localhost" ).toByteArray() );
  mNetReply = QgsNetworkAccessManager::instance()->get( req );
  connect( mNetReply, SIGNAL( finished() ), this, SLOT( replyFinished() ) );
  mTimeoutTimer.start( sSearchTimeout );
}

void QgsLocationSearchProvider::cancelSearch()
{
  if ( mNetReply )
  {
    mTimeoutTimer.stop();
    disconnect( mNetReply, SIGNAL( finished() ), this, SLOT( replyFinished() ) );
    mNetReply->close();
    mNetReply->deleteLater();
    mNetReply = 0;
  }
}

void QgsLocationSearchProvider::replyFinished()
{
  if ( !mNetReply )
    return;

  if ( mNetReply->error() != QNetworkReply::NoError || !mTimeoutTimer.isActive() )
  {
    mNetReply->deleteLater();
    mNetReply = 0;
    emit searchFinished();
    return;
  }

  QByteArray replyText = mNetReply->readAll();
  QJson::Parser parser;
  QVariant result = parser.parse( replyText );
  if ( result.isNull() )
  {
    QgsDebugMsg( QString( "Error at line %1: %2" ).arg( parser.errorLine() ).arg( parser.errorString() ) );
  }
  foreach ( const QVariant& item, result.toMap()["results"].toList() )
  {
    QVariantMap itemMap = item.toMap();
    QVariantMap itemAttrsMap = itemMap["attrs"].toMap();

    QString origin = itemAttrsMap["origin"].toString();

    SearchResult searchResult;
    if ( mPatBox.exactMatch( itemAttrsMap["geom_st_box2d"].toString() ) )
    {
      searchResult.bbox = QgsRectangle( mPatBox.cap( 1 ).toDouble(), mPatBox.cap( 2 ).toDouble(),
                                        mPatBox.cap( 3 ).toDouble(), mPatBox.cap( 4 ).toDouble() );
    }
    // When bbox is empty, fallback to pos + zoomScale is used
    searchResult.pos = QgsPoint( itemAttrsMap["y"].toDouble(), itemAttrsMap["x"].toDouble() );
    searchResult.zoomScale = 1000;

    searchResult.category = mCategoryMap.contains( origin ) ? mCategoryMap[origin] : origin;
    searchResult.text = itemAttrsMap["label"].toString();
    searchResult.text.replace( QRegExp( "<[^>]+>" ), "" ); // Remove HTML tags
    searchResult.crs = QgsCoordinateReferenceSystem( "EPSG:21781" );
    emit searchResultFound( searchResult );
  }
  mNetReply->deleteLater();
  mNetReply = 0;
  emit searchFinished();
}