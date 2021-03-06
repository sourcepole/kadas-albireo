/***************************************************************************
 *  qgsguidegridlayer.h                                                    *
 *  --------------                                                         *
 *  begin                : March 2018                                      *
 *  copyright            : (C) 2018 by Sandro Mani / Sourcepole AG         *
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

#ifndef QGSGUIDEGRIDLAYER_H
#define QGSGUIDEGRIDLAYER_H

#include "qgspluginlayer.h"
#include "qgspluginlayerregistry.h"

class CORE_EXPORT QgsGuideGridLayer : public QgsPluginLayer
{
    Q_OBJECT
  public:
    static QString layerTypeKey() { return "guide_grid"; }
    enum LabellingMode { LABEL_A_1, LABEL_1_A };

    QgsGuideGridLayer( const QString &name );
    void setup( const QgsRectangle& gridRect, int cols, int rows, const QgsCoordinateReferenceSystem& crs, bool colSizeLocked, bool rowSizeLocked );
    bool writeSymbology( QDomNode &/*node*/, QDomDocument& /*doc*/, QString& /*errorMessage*/ ) const override { return true; }
    bool readSymbology( const QDomNode &/*node*/, QString &/*errorMessage*/ ) override { return true; }
    QgsMapLayerRenderer* createMapRenderer( QgsRenderContext& rendererContext ) override;
    QgsRectangle extent() override { return mGridRect; }
    int cols() const { return mCols; }
    int rows() const { return mRows; }
    bool colSizeLocked() const { return mColSizeLocked; }
    bool rowSizeLocked() const { return mRowSizeLocked; }

    const QColor& color() const { return mColor; }
    int fontSize() const { return mFontSize; }
    LabellingMode labelingMode() const { return mLabellingMode; }

    QList<IdentifyResult> identify( const QgsPoint& mapPos, const QgsMapSettings& mapSettings ) override;

  public slots:
    void setColor( const QColor& color ) { mColor = color; }
    void setFontSize( int fontSize ) { mFontSize = fontSize; }
    void setLabelingMode( LabellingMode labelingMode ) { mLabellingMode = labelingMode; }

  protected:
    bool readXml( const QDomNode& layer_node ) override;
    bool writeXml( QDomNode & layer_node, QDomDocument & document ) override;

  private:
    class Renderer;

    QgsRectangle mGridRect;
    int mCols = 0;
    int mRows = 0;
    bool mColSizeLocked = false;
    bool mRowSizeLocked = false;
    int mFontSize = 30;
    QColor mColor = Qt::red;
    LabellingMode mLabellingMode = LABEL_A_1;
};

class CORE_EXPORT QgsGuideGridLayerType : public QgsPluginLayerType
{
  public:
    QgsGuideGridLayerType( )
        : QgsPluginLayerType( QgsGuideGridLayer::layerTypeKey() ) {}
    QgsPluginLayer* createLayer() override { return new QgsGuideGridLayer( "" ); }
    int hasLayerProperties() const override { return 0; }
};

#endif // QGSGUIDEGRIDLAYER_H
