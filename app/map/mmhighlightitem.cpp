/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "mmhighlightitem.h"
#include "mmstyle.h"

#include <QPainter>
#include <QPainterPath>
#include <QSGSimpleTextureNode>
#include <QQuickWindow>

#include "qgslinestring.h"
#include "qgspolygon.h"
#include "qgsmultipoint.h"
#include "qgsmultilinestring.h"
#include "qgsmultipolygon.h"
#include "qgsgeometrycollection.h"
#include "qgswkbtypes.h"

MMHighlightItem::MMHighlightItem( QQuickItem *parent )
  : QQuickItem( parent )
{
  setFlag( QQuickItem::ItemHasContents, true );

  // Pull default colors from MMStyle (color methods are pure constants, dp is unused for colors)
  MMStyle style( nullptr, 1.0 );
  mMarkerColor = style.grapeColor();
  mMarkerBorderColor = style.polarColor();
  mLineColor = style.grapeColor();
  mLineBorderColor = style.polarColor();
  mPolygonFillColor = style.grapeTransparentColor();
  mPolygonRingColor = style.grapeColor();
  mPolygonRingBorderColor = style.polarColor();

  mPinRenderer.load( QStringLiteral( ":/images/MapPin.svg" ) );
}

QPointF MMHighlightItem::mapToScreen( double x, double y ) const
{
  if ( !mMapSettings )
    return QPointF();

  double scale = 1.0 / mMapSettings->mapUnitsPerPixel();
  double dpr = mMapSettings->devicePixelRatio();
  double offsetX = -mMapSettings->visibleExtent().xMinimum();
  double offsetY = -mMapSettings->visibleExtent().yMaximum();

  return QPointF(
           ( x + offsetX ) * scale / dpr,
           -( y + offsetY ) * scale / dpr
         );
}

qreal MMHighlightItem::resolvedLineWidth() const
{
  return mLineWidth == NormalLine ? 8.0 : 4.0;
}

qreal MMHighlightItem::resolvedMarkerSize() const
{
  return mMarkerSize == Normal ? 18.0 : 21.0;
}

void MMHighlightItem::markDirty()
{
  mDirty = true;
  update();
}

void MMHighlightItem::renderHighlight()
{
  qreal dpr = mMapSettings ? mMapSettings->devicePixelRatio() : 1.0;
  int w = static_cast<int>( width() * dpr );
  int h = static_cast<int>( height() * dpr );

  if ( w <= 0 || h <= 0 )
  {
    mImage = QImage();
    return;
  }

  if ( mGeometry.isEmpty() || mGeometry.isNull() || !mMapSettings )
  {
    mImage = QImage();
    return;
  }

  mImage = QImage( w, h, QImage::Format_ARGB32_Premultiplied );
  mImage.setDevicePixelRatio( dpr );
  mImage.fill( Qt::transparent );

  QPainter painter( &mImage );
  painter.setRenderHint( QPainter::Antialiasing, true );

  Qgis::GeometryType geomType = mGeometry.type();

  if ( geomType == Qgis::GeometryType::Point )
  {
    paintPoints( painter );
  }
  else if ( geomType == Qgis::GeometryType::Line )
  {
    paintLines( painter );
  }
  else if ( geomType == Qgis::GeometryType::Polygon )
  {
    paintPolygons( painter );
  }

  painter.end();
}

void MMHighlightItem::paintMarkerAt( QPainter &painter, const QPointF &screenPos )
{
  painter.save();

  if ( mMarkerType == Circle )
  {
    qreal r = resolvedMarkerSize();
    QRectF circleRect( screenPos.x() - r / 2, screenPos.y() - r / 2, r, r );

    QPen pen( mMarkerBorderColor, mMarkerBorderWidth );
    painter.setPen( pen );
    painter.setBrush( mMarkerColor );
    painter.drawEllipse( circleRect );
  }
  else // Image marker (pin)
  {
    QRectF pinRect( screenPos.x() - mMarkerWidth / 2,
                    screenPos.y() - mMarkerHeight,
                    mMarkerWidth,
                    mMarkerHeight );

    if ( mPinRenderer.isValid() )
    {
      mPinRenderer.render( &painter, pinRect );
    }
  }

  painter.restore();
}

void MMHighlightItem::paintPoints( QPainter &painter )
{
  const QgsAbstractGeometry *geom = mGeometry.constGet();
  if ( !geom )
    return;

  QgsVertexId vid;
  QgsPoint pt;

  while ( geom->nextVertex( vid, pt ) )
  {
    QPointF screenPt = mapToScreen( pt.x(), pt.y() );
    paintMarkerAt( painter, screenPt );
  }
}

static void buildPathFromLineString( const QgsLineString *ls, QPainterPath &path,
                                     const std::function<QPointF( double, double )> &toScreen )
{
  if ( !ls || ls->numPoints() == 0 )
    return;

  const double *xData = ls->xData();
  const double *yData = ls->yData();
  int count = ls->numPoints();

  QPointF first = toScreen( xData[0], yData[0] );
  path.moveTo( first );

  for ( int i = 1; i < count; ++i )
  {
    path.lineTo( toScreen( xData[i], yData[i] ) );
  }
}

static void collectLinePaths( const QgsAbstractGeometry *geom, QPainterPath &path,
                              const std::function<QPointF( double, double )> &toScreen )
{
  if ( const QgsLineString *ls = qgsgeometry_cast<const QgsLineString *>( geom ) )
  {
    buildPathFromLineString( ls, path, toScreen );
  }
  else if ( const QgsGeometryCollection *collection = qgsgeometry_cast<const QgsGeometryCollection *>( geom ) )
  {
    for ( int i = 0; i < collection->numGeometries(); ++i )
    {
      collectLinePaths( collection->geometryN( i ), path, toScreen );
    }
  }
}

void MMHighlightItem::paintLines( QPainter &painter )
{
  const QgsAbstractGeometry *geom = mGeometry.constGet();
  if ( !geom )
    return;

  // If this is a single point, draw a temporary marker instead
  QgsVertexId vid;
  QgsPoint testPt;
  int vertexCount = 0;
  QgsPoint firstPt;
  while ( geom->nextVertex( vid, testPt ) )
  {
    if ( vertexCount == 0 )
      firstPt = testPt;
    vertexCount++;
    if ( vertexCount > 1 )
      break;
  }

  if ( vertexCount <= 1 && vertexCount > 0 )
  {
    QPointF screenPt = mapToScreen( firstPt.x(), firstPt.y() );
    paintMarkerAt( painter, screenPt );
    return;
  }

  QPainterPath path;
  auto toScreen = [this]( double x, double y ) { return mapToScreen( x, y ); };
  collectLinePaths( geom, path, toScreen );

  qreal lw = resolvedLineWidth();

  // Draw border first (thicker)
  if ( mLineBorderWidth > 0 )
  {
    QPen borderPen( mLineBorderColor, lw + mLineBorderWidth );
    borderPen.setCapStyle( Qt::RoundCap );
    borderPen.setJoinStyle( Qt::BevelJoin );
    painter.setPen( borderPen );
    painter.setBrush( Qt::NoBrush );
    painter.drawPath( path );
  }

  // Draw main line
  QPen linePen( mLineColor, lw );
  linePen.setStyle( static_cast<Qt::PenStyle>( mLineStrokeStyle ) );
  linePen.setCapStyle( Qt::RoundCap );
  linePen.setJoinStyle( Qt::BevelJoin );
  painter.setPen( linePen );
  painter.setBrush( Qt::NoBrush );
  painter.drawPath( path );
}

static void collectPolygonPaths( const QgsAbstractGeometry *geom, QPainterPath &path,
                                 const std::function<QPointF( double, double )> &toScreen )
{
  if ( const QgsPolygon *poly = qgsgeometry_cast<const QgsPolygon *>( geom ) )
  {
    // Exterior ring
    if ( const QgsLineString *exterior = qgsgeometry_cast<const QgsLineString *>( poly->exteriorRing() ) )
    {
      buildPathFromLineString( exterior, path, toScreen );
      path.closeSubpath();
    }

    // Interior rings (holes)
    for ( int i = 0; i < poly->numInteriorRings(); ++i )
    {
      if ( const QgsLineString *interior = qgsgeometry_cast<const QgsLineString *>( poly->interiorRing( i ) ) )
      {
        buildPathFromLineString( interior, path, toScreen );
        path.closeSubpath();
      }
    }
  }
  else if ( const QgsGeometryCollection *collection = qgsgeometry_cast<const QgsGeometryCollection *>( geom ) )
  {
    for ( int i = 0; i < collection->numGeometries(); ++i )
    {
      collectPolygonPaths( collection->geometryN( i ), path, toScreen );
    }
  }
}

void MMHighlightItem::paintPolygons( QPainter &painter )
{
  const QgsAbstractGeometry *geom = mGeometry.constGet();
  if ( !geom )
    return;

  // If this is a single point, draw a temporary marker instead
  QgsVertexId vid;
  QgsPoint testPt;
  int vertexCount = 0;
  QgsPoint firstPt;
  while ( geom->nextVertex( vid, testPt ) )
  {
    if ( vertexCount == 0 )
      firstPt = testPt;
    vertexCount++;
    if ( vertexCount > 1 )
      break;
  }

  if ( vertexCount <= 1 && vertexCount > 0 )
  {
    QPointF screenPt = mapToScreen( firstPt.x(), firstPt.y() );
    paintMarkerAt( painter, screenPt );
    return;
  }

  QPainterPath path;
  auto toScreen = [this]( double x, double y ) { return mapToScreen( x, y ); };
  collectPolygonPaths( geom, path, toScreen );

  // Fill polygon
  painter.setPen( Qt::NoPen );
  painter.setBrush( mPolygonFillColor );
  painter.drawPath( path );

  // Draw ring border (thicker, behind ring)
  if ( mPolygonRingBorderWidth > 0 )
  {
    QPen ringBorderPen( mPolygonRingBorderColor, mPolygonRingWidth + mPolygonRingBorderWidth );
    ringBorderPen.setStyle( Qt::SolidLine );
    ringBorderPen.setCapStyle( Qt::RoundCap );
    ringBorderPen.setJoinStyle( Qt::BevelJoin );
    painter.setPen( ringBorderPen );
    painter.setBrush( Qt::NoBrush );
    painter.drawPath( path );
  }

  // Draw ring
  if ( mPolygonRingWidth > 0 )
  {
    QPen ringPen( mPolygonRingColor, mPolygonRingWidth );
    ringPen.setCapStyle( Qt::FlatCap );
    ringPen.setJoinStyle( Qt::BevelJoin );
    painter.setPen( ringPen );
    painter.setBrush( Qt::NoBrush );
    painter.drawPath( path );
  }
}

QSGNode *MMHighlightItem::updatePaintNode( QSGNode *oldNode, UpdatePaintNodeData * )
{
  if ( mDirty )
  {
    renderHighlight();

    delete oldNode;
    oldNode = nullptr;
    mDirty = false;
  }

  if ( mImage.isNull() )
    return nullptr;

  QSGSimpleTextureNode *node = static_cast<QSGSimpleTextureNode *>( oldNode );
  if ( !node )
  {
    node = new QSGSimpleTextureNode();
    QSGTexture *texture = window()->createTextureFromImage( mImage, QQuickWindow::TextureHasAlphaChannel );
    node->setTexture( texture );
    node->setOwnsTexture( true );
  }

  node->setRect( boundingRect() );
  return node;
}

// --- Property accessors ---

QVariant MMHighlightItem::geometryVariant() const
{
  return QVariant::fromValue( mGeometry );
}

void MMHighlightItem::setGeometryVariant( const QVariant &variant )
{
  if ( variant.isNull() || !variant.isValid() )
    mGeometry = QgsGeometry();
  else
    mGeometry = variant.value<QgsGeometry>();

  markDirty();
  emit geometryChanged();
}

InputMapSettings *MMHighlightItem::mapSettings() const
{
  return mMapSettings;
}

void MMHighlightItem::setMapSettings( InputMapSettings *mapSettings )
{
  if ( mapSettings == mMapSettings )
    return;

  if ( mMapSettings )
    disconnect( mMapSettings, &InputMapSettings::visibleExtentChanged, this, &MMHighlightItem::markDirty );

  mMapSettings = mapSettings;

  if ( mMapSettings )
    connect( mMapSettings, &InputMapSettings::visibleExtentChanged, this, &MMHighlightItem::markDirty );

  markDirty();
  emit mapSettingsChanged();
}

MMHighlightItem::MarkerType MMHighlightItem::markerType() const
{
  return mMarkerType;
}

void MMHighlightItem::setMarkerType( MarkerType type )
{
  if ( type == mMarkerType )
    return;
  mMarkerType = type;
  markDirty();
  emit markerTypeChanged();
}

MMHighlightItem::MarkerSize MMHighlightItem::markerSize() const
{
  return mMarkerSize;
}

void MMHighlightItem::setMarkerSize( MarkerSize size )
{
  if ( size == mMarkerSize )
    return;
  mMarkerSize = size;
  markDirty();
  emit markerSizeChanged();
}

QColor MMHighlightItem::markerColor() const
{
  return mMarkerColor;
}

void MMHighlightItem::setMarkerColor( const QColor &color )
{
  if ( color == mMarkerColor )
    return;
  mMarkerColor = color;
  markDirty();
  emit markerColorChanged();
}

QColor MMHighlightItem::markerBorderColor() const
{
  return mMarkerBorderColor;
}

void MMHighlightItem::setMarkerBorderColor( const QColor &color )
{
  if ( color == mMarkerBorderColor )
    return;
  mMarkerBorderColor = color;
  markDirty();
  emit markerBorderColorChanged();
}

qreal MMHighlightItem::markerWidth() const
{
  return mMarkerWidth;
}

void MMHighlightItem::setMarkerWidth( qreal w )
{
  if ( qFuzzyCompare( w, mMarkerWidth ) )
    return;
  mMarkerWidth = w;
  markDirty();
  emit markerWidthChanged();
}

qreal MMHighlightItem::markerHeight() const
{
  return mMarkerHeight;
}

void MMHighlightItem::setMarkerHeight( qreal h )
{
  if ( qFuzzyCompare( h, mMarkerHeight ) )
    return;
  mMarkerHeight = h;
  markDirty();
  emit markerHeightChanged();
}

qreal MMHighlightItem::markerBorderWidth() const
{
  return mMarkerBorderWidth;
}

void MMHighlightItem::setMarkerBorderWidth( qreal w )
{
  if ( qFuzzyCompare( w, mMarkerBorderWidth ) )
    return;
  mMarkerBorderWidth = w;
  markDirty();
  emit markerBorderWidthChanged();
}

MMHighlightItem::LineWidth MMHighlightItem::lineWidth() const
{
  return mLineWidth;
}

void MMHighlightItem::setLineWidth( LineWidth w )
{
  if ( w == mLineWidth )
    return;
  mLineWidth = w;
  markDirty();
  emit lineWidthChanged();
}

qreal MMHighlightItem::lineBorderWidth() const
{
  return mLineBorderWidth;
}

void MMHighlightItem::setLineBorderWidth( qreal w )
{
  if ( qFuzzyCompare( w, mLineBorderWidth ) )
    return;
  mLineBorderWidth = w;
  markDirty();
  emit lineBorderWidthChanged();
}

QColor MMHighlightItem::lineColor() const
{
  return mLineColor;
}

void MMHighlightItem::setLineColor( const QColor &color )
{
  if ( color == mLineColor )
    return;
  mLineColor = color;
  markDirty();
  emit lineColorChanged();
}

QColor MMHighlightItem::lineBorderColor() const
{
  return mLineBorderColor;
}

void MMHighlightItem::setLineBorderColor( const QColor &color )
{
  if ( color == mLineBorderColor )
    return;
  mLineBorderColor = color;
  markDirty();
  emit lineBorderColorChanged();
}

int MMHighlightItem::lineStrokeStyle() const
{
  return mLineStrokeStyle;
}

void MMHighlightItem::setLineStrokeStyle( int style )
{
  if ( style == mLineStrokeStyle )
    return;
  mLineStrokeStyle = style;
  markDirty();
  emit lineStrokeStyleChanged();
}

QColor MMHighlightItem::polygonFillColor() const
{
  return mPolygonFillColor;
}

void MMHighlightItem::setPolygonFillColor( const QColor &color )
{
  if ( color == mPolygonFillColor )
    return;
  mPolygonFillColor = color;
  markDirty();
  emit polygonFillColorChanged();
}

QColor MMHighlightItem::polygonRingColor() const
{
  return mPolygonRingColor;
}

void MMHighlightItem::setPolygonRingColor( const QColor &color )
{
  if ( color == mPolygonRingColor )
    return;
  mPolygonRingColor = color;
  markDirty();
  emit polygonRingColorChanged();
}

QColor MMHighlightItem::polygonRingBorderColor() const
{
  return mPolygonRingBorderColor;
}

void MMHighlightItem::setPolygonRingBorderColor( const QColor &color )
{
  if ( color == mPolygonRingBorderColor )
    return;
  mPolygonRingBorderColor = color;
  markDirty();
  emit polygonRingBorderColorChanged();
}

qreal MMHighlightItem::polygonRingWidth() const
{
  return mPolygonRingWidth;
}

void MMHighlightItem::setPolygonRingWidth( qreal w )
{
  if ( qFuzzyCompare( w, mPolygonRingWidth ) )
    return;
  mPolygonRingWidth = w;
  markDirty();
  emit polygonRingWidthChanged();
}

qreal MMHighlightItem::polygonRingBorderWidth() const
{
  return mPolygonRingBorderWidth;
}

void MMHighlightItem::setPolygonRingBorderWidth( qreal w )
{
  if ( qFuzzyCompare( w, mPolygonRingBorderWidth ) )
    return;
  mPolygonRingBorderWidth = w;
  markDirty();
  emit polygonRingBorderWidthChanged();
}
