/***************************************************************************
 *  qgscoordinateformat.h                                                  *
 *  ---------------------                                                  *
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

#ifndef QGSCOORDINATEFORMAT_H
#define QGSCOORDINATEFORMAT_H

#include <QObject>
#include "qgscoordinatereferencesystem.h"

class QgsPoint;
class QgsCoordinateReferenceSystem;

class CORE_EXPORT QgsCoordinateFormat : public QObject
{
    Q_OBJECT
  public:
    enum Format
    {
      Default,
      DegMinSec,
      DegMin,
      DecDeg,
      UTM,
      MGRS
    };

    static QgsCoordinateFormat* instance();
    void getCoordinateDisplayFormat( QgsCoordinateFormat::Format& format, QString& epsg ) const;
    QGis::UnitType getHeightDisplayUnit() const { return mHeightUnit; }

    QString getDisplayString( const QgsPoint& p , const QgsCoordinateReferenceSystem &sSrs ) const;
    static QString getDisplayString( const QgsPoint& p , const QgsCoordinateReferenceSystem &sSrs, Format format, const QString& epsg );

    double getHeightAtPos( const QgsPoint& p, const QgsCoordinateReferenceSystem& crs, QString* errMsg = 0 );
    static double getHeightAtPos( const QgsPoint& p, const QgsCoordinateReferenceSystem& crs, QGis::UnitType unit, QString* errMsg = 0 );

    QgsPoint parseCoordinate( const QString& text, Format format, bool& valid ) const;

  public slots:
    void setCoordinateDisplayFormat( Format format, const QString& epsg );
    void setHeightDisplayUnit( QGis::UnitType heightUnit );

  signals:
    void coordinateDisplayFormatChanged( QgsCoordinateFormat::Format format, const QString& epsg );
    void heightDisplayUnitChanged( QGis::UnitType heightUnit );

  private:
    QgsCoordinateFormat();

    Format mFormat;
    QString mEpsg;
    QGis::UnitType mHeightUnit;
};

#endif // QGSCOORDINATEFORMAT_H
