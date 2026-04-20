/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MMHIGHLIGHTITEM_H
#define MMHIGHLIGHTITEM_H

#include "inputmapsettings.h"

#include <QQuickItem>
#include <QImage>
#include <QSvgRenderer>
#include <qgsgeometry.h>

/**
 * \brief C++ QQuickItem that renders geometry highlights using QPainter,
 * replacing the QML-based MMHighlight.qml for better performance on mobile.
 *
 * Geometry is transformed from map CRS to screen coordinates and painted
 * onto a QImage which is uploaded as a QSGSimpleTextureNode.
 *
 * \note QML Type: MMHighlightItem (registered in "mm" module)
 */
class MMHighlightItem : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY( QVariant geometry READ geometryVariant WRITE setGeometryVariant NOTIFY geometryChanged )
    Q_PROPERTY( InputMapSettings *mapSettings READ mapSettings WRITE setMapSettings NOTIFY mapSettingsChanged )

    // Marker properties
    Q_PROPERTY( MarkerType markerType READ markerType WRITE setMarkerType NOTIFY markerTypeChanged )
    Q_PROPERTY( MarkerSize markerSize READ markerSize WRITE setMarkerSize NOTIFY markerSizeChanged )
    Q_PROPERTY( QColor markerColor READ markerColor WRITE setMarkerColor NOTIFY markerColorChanged )
    Q_PROPERTY( QColor markerBorderColor READ markerBorderColor WRITE setMarkerBorderColor NOTIFY markerBorderColorChanged )
    Q_PROPERTY( qreal markerWidth READ markerWidth WRITE setMarkerWidth NOTIFY markerWidthChanged )
    Q_PROPERTY( qreal markerHeight READ markerHeight WRITE setMarkerHeight NOTIFY markerHeightChanged )
    Q_PROPERTY( qreal markerBorderWidth READ markerBorderWidth WRITE setMarkerBorderWidth NOTIFY markerBorderWidthChanged )

    // Line properties
    Q_PROPERTY( LineWidth lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged )
    Q_PROPERTY( qreal lineBorderWidth READ lineBorderWidth WRITE setLineBorderWidth NOTIFY lineBorderWidthChanged )
    Q_PROPERTY( QColor lineColor READ lineColor WRITE setLineColor NOTIFY lineColorChanged )
    Q_PROPERTY( QColor lineBorderColor READ lineBorderColor WRITE setLineBorderColor NOTIFY lineBorderColorChanged )
    Q_PROPERTY( int lineStrokeStyle READ lineStrokeStyle WRITE setLineStrokeStyle NOTIFY lineStrokeStyleChanged )

    // Polygon properties
    Q_PROPERTY( QColor polygonFillColor READ polygonFillColor WRITE setPolygonFillColor NOTIFY polygonFillColorChanged )
    Q_PROPERTY( QColor polygonRingColor READ polygonRingColor WRITE setPolygonRingColor NOTIFY polygonRingColorChanged )
    Q_PROPERTY( QColor polygonRingBorderColor READ polygonRingBorderColor WRITE setPolygonRingBorderColor NOTIFY polygonRingBorderColorChanged )
    Q_PROPERTY( qreal polygonRingWidth READ polygonRingWidth WRITE setPolygonRingWidth NOTIFY polygonRingWidthChanged )
    Q_PROPERTY( qreal polygonRingBorderWidth READ polygonRingBorderWidth WRITE setPolygonRingBorderWidth NOTIFY polygonRingBorderWidthChanged )

  public:

    enum MarkerType
    {
      Circle,
      Image
    };
    Q_ENUM( MarkerType )

    enum MarkerSize
    {
      Normal,
      Bigger
    };
    Q_ENUM( MarkerSize )

    enum LineWidth
    {
      NormalLine,
      Narrow
    };
    Q_ENUM( LineWidth )

    explicit MMHighlightItem( QQuickItem *parent = nullptr );
    ~MMHighlightItem() override = default;

    QSGNode *updatePaintNode( QSGNode *oldNode, UpdatePaintNodeData * ) override;

    // Property accessors
    QVariant geometryVariant() const;
    void setGeometryVariant( const QVariant &variant );

    InputMapSettings *mapSettings() const;
    void setMapSettings( InputMapSettings *mapSettings );

    MarkerType markerType() const;
    void setMarkerType( MarkerType type );

    MarkerSize markerSize() const;
    void setMarkerSize( MarkerSize size );

    QColor markerColor() const;
    void setMarkerColor( const QColor &color );

    QColor markerBorderColor() const;
    void setMarkerBorderColor( const QColor &color );

    qreal markerWidth() const;
    void setMarkerWidth( qreal width );

    qreal markerHeight() const;
    void setMarkerHeight( qreal height );

    qreal markerBorderWidth() const;
    void setMarkerBorderWidth( qreal width );

    LineWidth lineWidth() const;
    void setLineWidth( LineWidth width );

    qreal lineBorderWidth() const;
    void setLineBorderWidth( qreal width );

    QColor lineColor() const;
    void setLineColor( const QColor &color );

    QColor lineBorderColor() const;
    void setLineBorderColor( const QColor &color );

    int lineStrokeStyle() const;
    void setLineStrokeStyle( int style );

    QColor polygonFillColor() const;
    void setPolygonFillColor( const QColor &color );

    QColor polygonRingColor() const;
    void setPolygonRingColor( const QColor &color );

    QColor polygonRingBorderColor() const;
    void setPolygonRingBorderColor( const QColor &color );

    qreal polygonRingWidth() const;
    void setPolygonRingWidth( qreal width );

    qreal polygonRingBorderWidth() const;
    void setPolygonRingBorderWidth( qreal width );

  signals:
    void geometryChanged();
    void mapSettingsChanged();
    void markerTypeChanged();
    void markerSizeChanged();
    void markerColorChanged();
    void markerBorderColorChanged();
    void markerWidthChanged();
    void markerHeightChanged();
    void markerBorderWidthChanged();
    void lineWidthChanged();
    void lineBorderWidthChanged();
    void lineColorChanged();
    void lineBorderColorChanged();
    void lineStrokeStyleChanged();
    void polygonFillColorChanged();
    void polygonRingColorChanged();
    void polygonRingBorderColorChanged();
    void polygonRingWidthChanged();
    void polygonRingBorderWidthChanged();

  private slots:
    void markDirty();

  private:
    void renderHighlight();
    QPointF mapToScreen( double x, double y ) const;
    qreal resolvedLineWidth() const;
    qreal resolvedMarkerSize() const;
    void paintPoints( QPainter &painter );
    void paintLines( QPainter &painter );
    void paintPolygons( QPainter &painter );
    void paintMarkerAt( QPainter &painter, const QPointF &screenPos );

    QgsGeometry mGeometry;
    InputMapSettings *mMapSettings = nullptr;

    // Marker — defaults set in constructor from MMStyle
    MarkerType mMarkerType = Image;
    MarkerSize mMarkerSize = Normal;
    QColor mMarkerColor;
    QColor mMarkerBorderColor;
    qreal mMarkerWidth = 40;
    qreal mMarkerHeight = 53;
    qreal mMarkerBorderWidth = 2;

    // Line — defaults set in constructor from MMStyle
    LineWidth mLineWidth = NormalLine;
    qreal mLineBorderWidth = 4;
    QColor mLineColor;
    QColor mLineBorderColor;
    int mLineStrokeStyle = Qt::SolidLine;

    // Polygon — defaults set in constructor from MMStyle
    QColor mPolygonFillColor;
    QColor mPolygonRingColor;
    QColor mPolygonRingBorderColor;
    qreal mPolygonRingWidth = 8;
    qreal mPolygonRingBorderWidth = 0;

    QImage mImage;
    bool mDirty = true;

    QSvgRenderer mPinRenderer;
};

#endif // MMHIGHLIGHTITEM_H
