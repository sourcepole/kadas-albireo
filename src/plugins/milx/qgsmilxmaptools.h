/***************************************************************************
 *  qgsmilxmaptoolS.h                                                      *
 *  -----------------                                                      *
 *  begin                : Oct 01, 2015                                    *
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

#ifndef QGSMILXMAPTOOLS_H
#define QGSMILXMAPTOOLS_H

#include "qgsbottombar.h"
#include "qgsmaptool.h"
#include "qgsmilxlibrary.h"
#include <QAction>
#include <QPointer>

class QgsBottomBar;
class QGraphicsRectItem;
class QPushButton;
class QWidgetAction;
class QgisInterface;
class QgsFloatingInputWidget;
class QgsMilXAnnotationItem;
class QgsMilXCreateTool;
class QgsMilXEditTool;
class QgsMilXItem;
class QgsMilXLayer;

class QgsMilxCreateBottomBar : public QgsBottomBar
{
    Q_OBJECT
  public:
    QgsMilxCreateBottomBar( QgsMilXCreateTool* tool, QgsMilXLibrary* library );

  private:
    QgsMilXCreateTool* mTool;
    QToolButton* mSymbolButton;
    QgsMilXLibrary* mLibrary;

  private slots:
    void symbolSelected( const QgsMilxSymbolTemplate& symbolTemplate );
    void toggleLibrary( bool visible );
};

class QgsMilXCreateTool : public QgsMapTool
{
    Q_OBJECT
  public:
    QgsMilXCreateTool( QgisInterface *iface, QgsMilXLibrary *library );
    ~QgsMilXCreateTool();
    void canvasPressEvent( QMouseEvent * e ) override;
    void canvasMoveEvent( QMouseEvent * e ) override;
    void keyPressEvent( QKeyEvent *e ) override;

  private:
    friend class QgsMilxCreateBottomBar;
    QgsMilxCreateBottomBar* mBottomBar;
    QgisInterface* mIface;
    QgsMilxSymbolTemplate mSymbolTemplate;
    int mNPressedPoints;
    QgsMilXAnnotationItem* mItem;
    QgsMilXLayer* mLayer;
    QgsFloatingInputWidget* mInputWidget;

    void setSymbolTemplate( const QgsMilxSymbolTemplate& symbolTemplate );
    void initializeItem( const QgsPoint &position );
    void finalizeItem();

  private slots:
    void reset();
    void updateAttribute();
    void updatePoint();
    void confirmPoint();
    void setTargetLayer( QgsMilXLayer* layer );
};

class QgsMilxEditBottomBar : public QgsBottomBar
{
    Q_OBJECT
  public:
    QgsMilxEditBottomBar( QgsMilXEditTool* tool );

  private:
    QgsMilXEditTool* mTool;
    QLabel* mStatusLabel;
    QPushButton* mCopyButton;
    QPushButton* mMoveButton;
    QMenu* mCopyMenu;
    QMenu* mMoveMenu;

  private:
    QString createLayer();
    void copyMoveSymbols( const QString& targetLayerId, bool move );

  private slots:
    void onClose();
    void repopulateLayers();
    void updateStatus();

    void copyToLayer() { copyMoveSymbols( qobject_cast<QAction*>( sender() )->data().toString(), false ); }
    void copyToNewLayer() { copyMoveSymbols( createLayer(), false ); }
    void moveToLayer() { copyMoveSymbols( qobject_cast<QAction*>( sender() )->data().toString(), true ); }
    void moveToNewLayer() { copyMoveSymbols( createLayer(), true ); }
};

class QgsMilXEditTool : public QgsMapTool
{
    Q_OBJECT
  public:
    QgsMilXEditTool( QgisInterface* iface, QgsMilXLayer* layer, QgsMilXItem* milxItem = 0 );
    ~QgsMilXEditTool();
    void activate() override;
    void canvasPressEvent( QMouseEvent* e ) override;
    void canvasMoveEvent( QMouseEvent * e ) override;
    void canvasReleaseEvent( QMouseEvent * e ) override;
    void keyPressEvent( QKeyEvent *e ) override;
    void canvasDoubleClickEvent( QMouseEvent *e ) override;
    void paste( const QByteArray& data, const QgsPoint *mapPos = 0 );
    int itemCount() const { return mItems.size(); }
    QgsMilXLayer* targetLayer() const;

  signals:
    void itemsChanged();

  private:
    friend class QgsMilxEditBottomBar;
    QgisInterface* mIface;
    QgsMilxEditBottomBar* mBottomBar;
    QList<QgsMilXAnnotationItem*> mItems;
    QPointer<QgsMilXLayer> mLayer;
    QGraphicsRectItem* mRectItem;
    QPointF mMouseMoveLastXY;
    bool mDraggingRect;
    QgsMilXAnnotationItem* mActiveAnnotation;
    int mAnnotationMoveAction;
    QgsFloatingInputWidget* mInputWidget;
    bool mMoving;

    void setLayer( QgsMilXLayer* layer );

  private slots:
    void copy();
    void cut() { copy(); deleteAll(); }
    void deleteAll();
    void removeItemFromList();
    void updateRect();
    void checkLayerHidden();
    void updatePoint();
    void updateAttribute();
};

#endif // QGSMILXMAPTOOLS_H

