/***************************************************************************
    qgsrubberband.cpp - Rubberband widget for drawing multilines and polygons
     --------------------------------------
    Date                 : 07-Jan-2006
    Copyright            : (C) 2006 by Tom Elwertowski
    Email                : telwertowski at users dot sourceforge dot net
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsrubberband.h"
#include "qgsfeature.h"
#include "qgsgeometry.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmaprenderer.h"
#include "qgsvectorlayer.h"
#include <QPainter>
#include <QSvgRenderer>

/*!
  \class QgsRubberBand
  \brief The QgsRubberBand class provides a transparent overlay widget
  for tracking the mouse while drawing polylines or polygons.
*/
QgsRubberBand::QgsRubberBand( QgsMapCanvas* mapCanvas, QGis::GeometryType geometryType )
    : QgsMapCanvasItem( mapCanvas )
    , mIconSize( 5 )
    , mIconType( ICON_CIRCLE )
    , mSvgRenderer( 0 )
    , mGeometryType( geometryType )
    , mTranslationOffsetX( 0.0 )
    , mTranslationOffsetY( 0.0 )
{
  reset( geometryType );
  QColor color( Qt::lightGray );
  color.setAlpha( 63 );
  setColor( color );
  setWidth( 1 );
  setLineStyle( Qt::SolidLine );
  setBrushStyle( Qt::SolidPattern );
}

QgsRubberBand::QgsRubberBand( QgsMapCanvas* mapCanvas, bool isPolygon )
    : QgsMapCanvasItem( mapCanvas )
    , mIconSize( 5 )
    , mIconType( ICON_CIRCLE )
    , mSvgRenderer( 0 )
    , mTranslationOffsetX( 0.0 )
    , mTranslationOffsetY( 0.0 )
{
  reset( isPolygon ? QGis::Polygon : QGis::Line );
  QColor color( Qt::lightGray );
  color.setAlpha( 63 );
  setColor( color );
  setWidth( 1 );
  setLineStyle( Qt::SolidLine );
  setBrushStyle( Qt::SolidPattern );
}

QgsRubberBand::QgsRubberBand()
    : QgsMapCanvasItem( 0 )
    , mIconSize( 5 )
    , mIconType( ICON_CIRCLE )
    , mGeometryType( QGis::Polygon )
    , mTranslationOffsetX( 0.0 )
    , mTranslationOffsetY( 0.0 )
{
}

QgsRubberBand::~QgsRubberBand()
{
  delete mSvgRenderer;
}

/*!
  Set the outline and fill color.
  */
void QgsRubberBand::setColor( const QColor & color )
{
  setBorderColor( color );
  setFillColor( color );
}

/*!
  Set the fill color.
  */
void QgsRubberBand::setFillColor( const QColor & color )
{
  QColor fillColor( color.red(), color.green(), color.blue(), color.alpha() );
  mBrush.setColor( fillColor );
}

/*!
  Set the outline
  */
void QgsRubberBand::setBorderColor( const QColor & color )
{
  QColor penColor( color.red(), color.green(), color.blue(), color.alpha() );
  mPen.setColor( penColor );
}


/*!
  Set the outline width.
  */
void QgsRubberBand::setWidth( int width )
{
  mPen.setWidth( width );
}

void QgsRubberBand::setIcon( IconType icon )
{
  delete mSvgRenderer;
  mSvgRenderer = 0;
  mIconType = icon;
}

void QgsRubberBand::setSvgIcon( const QString &path, const QPoint &drawOffset )
{
  setIcon( ICON_SVG );
  mSvgRenderer = new QSvgRenderer( path );;
  mSvgOffset = drawOffset;
}

void QgsRubberBand::setIconSize( int iconSize )
{
  mIconSize = iconSize;
}

void QgsRubberBand::setLineStyle( Qt::PenStyle penStyle )
{
  mPen.setStyle( penStyle );
}

void QgsRubberBand::setBrushStyle( Qt::BrushStyle brushStyle )
{
  mBrush.setStyle( brushStyle );
}

/*!
  Remove all points from the shape being created.
  */
void QgsRubberBand::reset( QGis::GeometryType geometryType )
{
  mPoints.clear();
  mGeometryType = geometryType;
  updateRect();
  update();
}

void QgsRubberBand::reset( bool isPolygon )
{
  mPoints.clear();
  mGeometryType = isPolygon ? QGis::Polygon : QGis::Line;
  updateRect();
  update();
}

/*!
  Add a point to the shape being created.
  */
void QgsRubberBand::addPoint( const QgsPoint & p, bool doUpdate /* = true */, int geometryIndex )
{
  if ( geometryIndex < 0 )
  {
    geometryIndex = mPoints.size() - 1;
  }

  if ( geometryIndex < 0 || geometryIndex > mPoints.size() )
  {
    return;
  }

  if ( geometryIndex == mPoints.size() )
  {
    mPoints.push_back( QList<QgsPoint>() << p );
  }

  if ( mPoints[geometryIndex].size() == 2 &&
       mPoints[geometryIndex][0] == mPoints[geometryIndex][1] )
  {
    mPoints[geometryIndex].last() = p;
  }
  else
  {
    mPoints[geometryIndex] << p;
  }


  if ( doUpdate )
  {
    setVisible( true );
    updateRect();
    update();
  }
}


void QgsRubberBand::removePoint( int index, bool doUpdate/* = true*/, int geometryIndex/* = 0*/ )
{
  if ( mPoints.size() < geometryIndex + 1 )
  {
    return;
  }


  if ( mPoints[geometryIndex].size() > 0 )
  {
    // negative index removes from end, eg -1 removes last one
    if ( index < 0 )
    {
      index = mPoints[geometryIndex].size() + index;
    }
    mPoints[geometryIndex].removeAt( index );
  }

  if ( doUpdate )
  {
    updateRect();
    update();
  }
}

void QgsRubberBand::removeLastPart( bool doUpdate )
{
  if ( mPoints.isEmpty() )
  {
    mPoints.removeLast();
  }
  if ( doUpdate )
  {
    updateRect();
    update();
  }
}

void QgsRubberBand::removeLastPoint( int geometryIndex, bool doUpdate/* = true*/ )
{
  removePoint( -1, doUpdate, geometryIndex );
}

/*!
  Update the line between the last added point and the mouse position.
  */
void QgsRubberBand::movePoint( const QgsPoint & p, int geometryIndex, bool doUpdate )
{
  if ( mPoints.size() < geometryIndex + 1 )
  {
    return;
  }

  if ( mPoints.at( geometryIndex ).size() < 1 )
  {
    return;
  }

  mPoints[geometryIndex].last() = p;

  if ( doUpdate )
  {
    updateRect();
    update();
  }
}

void QgsRubberBand::movePoint( int index, const QgsPoint& p, int geometryIndex, bool doUpdate )
{
  if ( mPoints.size() < geometryIndex + 1 )
  {
    return;
  }

  if ( mPoints.at( geometryIndex ).size() < index )
  {
    return;
  }

  mPoints[geometryIndex][index] = p;

  if ( doUpdate )
  {
    updateRect();
    update();
  }
}

void QgsRubberBand::setToGeometry( QgsGeometry* geom, QgsVectorLayer* layer )
{
  if ( !geom )
  {
    reset( mGeometryType );
    return;
  }

  reset( geom->type() );
  addGeometry( geom, layer );
}

void QgsRubberBand::addGeometry( QgsGeometry* geom, QgsVectorLayer* layer )
{
  if ( !geom )
  {
    return;
  }

  //maprender object of canvas
  const QgsMapSettings& ms = mMapCanvas->mapSettings();

  int idx = mPoints.size();

  switch ( QgsWKBTypes::flatType( geom->geometry()->wkbType() ) )
  {

    case QgsWKBTypes::Point:
    {
      QgsPoint pt;
      if ( layer )
      {
        pt = ms.layerToMapCoordinates( layer, geom->asPoint() );
      }
      else
      {
        pt = geom->asPoint();
      }
      addPoint( pt, false, idx );
      removeLastPoint( idx, false );
    }
    break;

    case QgsWKBTypes::MultiPoint:
    {
      QgsMultiPoint mpt = geom->asMultiPoint();
      for ( int i = 0; i < mpt.size(); ++i, ++idx )
      {
        QgsPoint pt = mpt[i];
        if ( layer )
        {
          addPoint( ms.layerToMapCoordinates( layer, pt ), false, idx );
          removeLastPoint( idx, false );
        }
        else
        {
          addPoint( pt, false, idx );
          removeLastPoint( idx, false );
        }
      }
    }
    break;

    case QgsWKBTypes::LineString:
    case QgsWKBTypes::CircularString:
    case QgsWKBTypes::CompoundCurve:
    {
      QgsPolyline line = geom->asPolyline();
      for ( int i = 0; i < line.count(); i++ )
      {
        if ( layer )
        {
          addPoint( ms.layerToMapCoordinates( layer, line[i] ), false, idx );
        }
        else
        {
          addPoint( line[i], false, idx );
        }
      }
    }
    break;

    case QgsWKBTypes::MultiLineString:
    case QgsWKBTypes::MultiCurve:
    {

      QgsMultiPolyline mline = geom->asMultiPolyline();
      for ( int i = 0; i < mline.size(); ++i, ++idx )
      {
        QgsPolyline line = mline[i];

        if ( line.size() == 0 )
        {
          --idx;
        }

        for ( int j = 0; j < line.size(); ++j )
        {
          if ( layer )
          {
            addPoint( ms.layerToMapCoordinates( layer, line[j] ), false, idx );
          }
          else
          {
            addPoint( line[j], false, idx );
          }
        }
      }
    }
    break;

    case QgsWKBTypes::Polygon:
    case QgsWKBTypes::CurvePolygon:
    {
      QgsPolygon poly = geom->asPolygon();
      QgsPolyline line = poly[0];
      for ( int i = 0; i < line.count(); i++ )
      {
        if ( layer )
        {
          addPoint( ms.layerToMapCoordinates( layer, line[i] ), false, idx );
        }
        else
        {
          addPoint( line[i], false, idx );
        }
      }
    }
    break;

    case QgsWKBTypes::MultiPolygon:
    {

      QgsMultiPolygon multipoly = geom->asMultiPolygon();
      for ( int i = 0; i < multipoly.size(); ++i, ++idx )
      {
        QgsPolygon poly = multipoly[i];
        QgsPolyline line = poly[0];
        for ( int j = 0; j < line.count(); ++j )
        {
          if ( layer )
          {
            addPoint( ms.layerToMapCoordinates( layer, line[j] ), false, idx );
          }
          else
          {
            addPoint( line[j], false, idx );
          }
        }
      }
    }
    break;

    case QGis::WKBUnknown:
    default:
      return;
  }

  setVisible( true );
  updateRect();
  update();
}

void QgsRubberBand::setToCanvasRectangle( const QRect& rect )
{
  if ( !mMapCanvas )
  {
    return;
  }

  const QgsMapToPixel* transform = mMapCanvas->getCoordinateTransform();
  QgsPoint ll = transform->toMapCoordinates( rect.left(), rect.bottom() );
  QgsPoint lr = transform->toMapCoordinates( rect.right(), rect.bottom() );
  QgsPoint ul = transform->toMapCoordinates( rect.left(), rect.top() );
  QgsPoint ur = transform->toMapCoordinates( rect.right(), rect.top() );

  reset( QGis::Polygon );
  addPoint( ll, false );
  addPoint( lr, false );
  addPoint( ur, false );
  addPoint( ul, true );
}

/*!
  Draw the shape in response to an update event.
  */
void QgsRubberBand::paint( QPainter* p )
{
  if ( mPoints.size() > 0 )
  {
    p->setBrush( mBrush );
    p->setPen( mPen );

    Q_FOREACH ( const QList<QgsPoint>& line, mPoints )
    {
      QVector<QPointF> pts;
      Q_FOREACH ( const QgsPoint& pt, line )
      {
        const QPointF cur = toCanvasCoordinates( QgsPoint( pt.x() + mTranslationOffsetX, pt.y() + mTranslationOffsetY ) ) - pos();
        if ( pts.empty() || std::abs( pts.back().x() - cur.x() ) > 1 ||  std::abs( pts.back().y() - cur.y() ) > 1 )
          pts.append( cur );
      }

      switch ( mGeometryType )
      {
        case QGis::Polygon:
        {
          p->drawPolygon( pts );
        }
        break;

        case QGis::Point:
        {
          Q_FOREACH ( const QPointF& pt, pts )
          {
            double x = pt.x();
            double y = pt.y();

            qreal s = ( mIconSize - 1 ) / 2.0;

            switch ( mIconType )
            {
              case ICON_NONE:
                break;

              case ICON_CROSS:
                p->drawLine( QLineF( x - s, y, x + s, y ) );
                p->drawLine( QLineF( x, y - s, x, y + s ) );
                break;

              case ICON_X:
                p->drawLine( QLineF( x - s, y - s, x + s, y + s ) );
                p->drawLine( QLineF( x - s, y + s, x + s, y - s ) );
                break;

              case ICON_BOX:
                p->drawLine( QLineF( x - s, y - s, x + s, y - s ) );
                p->drawLine( QLineF( x + s, y - s, x + s, y + s ) );
                p->drawLine( QLineF( x + s, y + s, x - s, y + s ) );
                p->drawLine( QLineF( x - s, y + s, x - s, y - s ) );
                break;

              case ICON_FULL_BOX:
                p->drawRect( x - s, y - s, mIconSize, mIconSize );
                break;

              case ICON_CIRCLE:
                p->drawEllipse( x - s, y - s, mIconSize, mIconSize );
                break;

              case ICON_SVG:
                QRectF viewBox = mSvgRenderer->viewBoxF();
                QRectF r( mSvgOffset.x(), mSvgOffset.y(), viewBox.width(), viewBox.height() );
                p->save();
                p->translate( pt );
                mSvgRenderer->render( p, r );
                p->restore();
                break;
            }
          }
        }
        break;

        case QGis::Line:
        default:
        {
          p->drawPolyline( pts );
        }
        break;
      }
    }
  }
}

void QgsRubberBand::updateRect()
{
  if ( mPoints.empty() )
  {
    setRect( QgsRectangle() );
    setVisible( false );
    return;
  }

  qreal mupp = mMapCanvas->getCoordinateTransform()->mapUnitsPerPixel();

  double iconSize = ( mIconSize + 1 ) / 2.;
  if ( mSvgRenderer )
  {
    QRectF viewBox = mSvgRenderer->viewBoxF();
    iconSize = qMax( qAbs( mSvgOffset.x() ) + .5 * viewBox.width(), qAbs( mSvgOffset.y() ) + .5 * viewBox.height() );
  }
  qreal w = qMax( iconSize, ( mPen.width() * qSqrt( 2 ) ) / 2. + mPen.width() ) * mupp;

  QgsRectangle newRect;
  for ( int i = 0; i < mPoints.size(); ++i )
  {
    foreach ( const QgsPoint& point, mPoints[i] )
    {
      QgsPoint p( point.x() + mTranslationOffsetX, point.y() + mTranslationOffsetY );
      QgsRectangle pRect( p.x() - w, p.y() - w, p.x() + w, p.y() + w );

      if ( newRect.isEmpty() )
      {
        // Get rectangle of the first point
        newRect = pRect;
      }
      else
      {
        newRect.combineExtentWith( &pRect );
      }
    }
  }
  setRect( newRect );
}

void QgsRubberBand::updatePosition( )
{
  // re-compute rectangle
  // See http://hub.qgis.org/issues/12392
  // NOTE: could be optimized by saving map-extent
  //       of rubberband and simply re-projecting
  //       that to device-rectange on "updatePosition"
  updateRect();
}

void QgsRubberBand::setTranslationOffset( double dx, double dy )
{
  mTranslationOffsetX = dx;
  mTranslationOffsetY = dy;
  updateRect();
}

void QgsRubberBand::translationOffset( double &dx, double &dy ) const
{
  dx = mTranslationOffsetX;
  dy = mTranslationOffsetY;
}

int QgsRubberBand::size() const
{
  return mPoints.size();
}

int QgsRubberBand::partSize( int geometryIndex ) const
{
  if ( geometryIndex < 0 || geometryIndex >= mPoints.size() ) return 0;
  return mPoints[geometryIndex].size();
}

QgsPoint QgsRubberBand::partMidpoint( int geometryIndex ) const
{
  int n = mPoints.size() <= geometryIndex ? 0 : mPoints[geometryIndex].size();
  if ( n == 0 )
  {
    return QgsPoint( 0, 0 );
  }

  if ( mGeometryType == QGis::Point )
  {
    return mPoints[geometryIndex][0];
  }
  else if ( mGeometryType == QGis::Line )
  {
    if ( n < 2 )
    {
      return mPoints[geometryIndex][0];
    }
    double totLen = 0;
    QList<double> cumLength;
    cumLength.append( 0 );
    for ( int i = 0; i < n - 1; ++i )
    {
      double len = qSqrt( mPoints[geometryIndex][i].sqrDist( mPoints[geometryIndex][i + 1] ) );
      cumLength.append( cumLength.back() + len );
      totLen += len;
    }
    for ( int i = 0; i < n - 1; ++i )
    {
      if ( cumLength[i + 1] > .5 * totLen )
      {
        double k = ( .5 * totLen - cumLength[i] ) / ( cumLength[i + 1] - cumLength[i] );
        return mPoints[geometryIndex][i] + ( mPoints[geometryIndex][i + 1] - mPoints[geometryIndex][i] ) * k;
      }
    }
    return mPoints[geometryIndex][0];
  }
  else if ( mGeometryType == QGis::Polygon )
  {
    if ( n < 2 )
    {
      return mPoints[geometryIndex].front();
    }

    // Reduce numerical instability by doing all the computations relative to first point
    QgsPoint center = mPoints[geometryIndex][0];
    QList<QgsPoint> points;
    for ( int i = 0; i < n; ++i )
    {
      points.append( QgsPoint( mPoints[geometryIndex][i].x() - center.x(), mPoints[geometryIndex][i].y() - center.y() ) );
    }

    double A = 0.;
    double Cx = 0.;
    double Cy = 0.;
    int i = n - 1, j = 0;
    for ( ; j < n; i = j++ )
    {
      const QgsPoint& vi = points[i];
      const QgsPoint& vj = points[j];
      double d = vi.x() * vj.y() - vj.x() * vi.y();
      A += d;
      Cx += ( vi.x() + vj.x() ) * d;
      Cy += ( vi.y() + vj.y() ) * d;
    }

    if ( qAbs( A ) < 1E-12 )
      return mPoints[geometryIndex].front();
    else
      return QgsPoint( center.x() + Cx / ( 3. * A ), center.y() + Cy / ( 3. * A ) );
  }
  return QgsPoint( 0, 0 );
}

int QgsRubberBand::numberOfVertices() const
{
  int count = 0;
  QList<QList<QgsPoint> >::const_iterator it = mPoints.constBegin();
  for ( ; it != mPoints.constEnd(); ++it )
  {
    QList<QgsPoint>::const_iterator iter = it->constBegin();
    for ( ; iter != it->constEnd(); ++iter )
    {
      ++count;
    }
  }
  return count;
}

const QgsPoint *QgsRubberBand::getPoint( int i, int j ) const
{
  if ( i < mPoints.size() && j < mPoints[i].size() )
    return &mPoints[i][j];
  else
    return 0;
}

void QgsRubberBand::setPoints( const QList<QList<QgsPoint> > &points, bool doUpdate )
{
  mPoints = points;
  if ( doUpdate )
  {
    setVisible( true );
    updateRect();
    update();
  }
}

QgsGeometry *QgsRubberBand::asGeometry()
{
  QgsGeometry *geom = NULL;

  switch ( mGeometryType )
  {
    case QGis::Polygon:
    {
      QgsPolygon polygon;
      QList< QList<QgsPoint> >::const_iterator it = mPoints.constBegin();
      for ( ; it != mPoints.constEnd(); ++it )
      {
        polygon.append( getPolyline( *it ) );
      }
      geom = QgsGeometry::fromPolygon( polygon );
      break;
    }

    case QGis::Point:
    {
      QgsMultiPoint multiPoint;

      QList< QList<QgsPoint> >::const_iterator it = mPoints.constBegin();
      for ( ; it != mPoints.constEnd(); ++it )
      {
        multiPoint += getPolyline( *it );
      }
      geom = QgsGeometry::fromMultiPoint( multiPoint );
      break;
    }

    case QGis::Line:
    default:
    {
      if ( mPoints.size() > 0 )
      {
        if ( mPoints.size() > 1 )
        {
          QgsMultiPolyline multiPolyline;
          QList< QList<QgsPoint> >::const_iterator it = mPoints.constBegin();
          for ( ; it != mPoints.constEnd(); ++it )
          {
            multiPolyline.append( getPolyline( *it ) );
          }
          geom = QgsGeometry::fromMultiPolyline( multiPolyline );
        }
        else
        {
          geom = QgsGeometry::fromPolyline( getPolyline( mPoints[0] ) );
        }
      }
      break;
    }
  }
  return geom;
}

QgsPolyline QgsRubberBand::getPolyline( const QList<QgsPoint> & points )
{
  QgsPolyline polyline;
  QList<QgsPoint>::const_iterator iter = points.constBegin();
  for ( ; iter != points.constEnd(); ++iter )
  {
    polyline.append( *iter );
  }
  return polyline;
}
