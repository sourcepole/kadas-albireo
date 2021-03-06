/***************************************************************************
 *  qgsmilxlibrary.h                                                       *
 *  ----------------                                                       *
 *  begin                : Sep 29, 2015                                    *
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

#ifndef QGSMILXLIBRARY_H
#define QGSMILXLIBRARY_H

#include <QWidget>
#include <QThread>

class QModelIndex;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class QgsFilterLineEdit;
class QgsMilXLibraryLoader;

class QgisInterface;


struct QgsMilxSymbolTemplate
{
  QString symbolXml;
  QString symbolMilitaryName;
  int minNPoints;
  bool hasVariablePoints;
  QPixmap pixmap;
};


class QgsMilXLibrary : public QWidget
{
    Q_OBJECT
  public:
    QgsMilXLibrary( QgisInterface* iface, QWidget* parent = 0 );
    ~QgsMilXLibrary();
    void focusFilter();

  signals:
    void symbolSelected( const QgsMilxSymbolTemplate& symbolTemplate );
    void visibilityChanged( bool visible );

  private:
    class TreeFilterProxyModel;
    friend class QgsMilXLibraryLoader;

    static const int SymbolXmlRole;
    static const int SymbolMilitaryNameRole;
    static const int SymbolPointCountRole;
    static const int SymbolVariablePointsRole;

    QgisInterface* mIface;
    QgsMilXLibraryLoader* mLoader;
    QgsFilterLineEdit* mFilterLineEdit;
    QTreeView* mTreeView;
    QStandardItemModel* mGalleryModel;
    QStandardItemModel* mLoadingModel;
    TreeFilterProxyModel* mFilterProxyModel;

    void showEvent( QShowEvent * ) { emit visibilityChanged( true ); }
    void hideEvent( QHideEvent * ) { emit visibilityChanged( false ); }

  private slots:
    void filterChanged( const QString& text );
    void itemClicked( const QModelIndex& index );
    void loaderFinished();
    QStandardItem* addItem( QStandardItem* parent, const QString& value, const QImage &image = QImage(), bool isLeaf = false, const QString& symbolXml = QString(), const QString &symbolMilitaryName = QString(), int symbolPointCount = 0, bool symbolHasVariablePoints = false );
};


class QgsMilXLibraryLoader : public QThread
{
    Q_OBJECT
  public:
    QgsMilXLibraryLoader( QgsMilXLibrary* library, QObject* parent = 0 ) : QThread( parent ), mAborted( false ), mLibrary( library ) {}
    void abort() { mAborted = true; }

  private:
    bool mAborted;
    QgsMilXLibrary* mLibrary;

    void run() override;
    QStandardItem* addItem( QStandardItem* parent, const QString& value, const QImage &image = QImage(), bool isLeaf = false, const QString& symbolXml = QString(), const QString &symbolMilitaryName = QString(), int symbolPointCount = 0, bool symbolHasVariablePoints = false );
};

#endif // QGSMILXLIBRARY_H
