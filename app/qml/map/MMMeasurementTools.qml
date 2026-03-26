/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick
import QtQuick.Shapes

import mm 1.0 as MM

import "../components"
import "./components"
import "../gps"
import "../dialogs"

Item {
  id: root

  required property MMMapCanvas map
  required property MMPositionMarker positionMarkerComponent

  property var mapTool: mapTool

  signal finishMeasurement()

  MM.MeasurementMapTool {
    id: mapTool

    mapSettings: root.map.mapSettings
    crosshairPoint: crosshair.screenPoint
  }

  MM.GuidelineController {
    id: guidelineController

    allowed: !mapTool.measurementFinalized
    mapSettings: root.map.mapSettings
    crosshairPosition: crosshair.screenPoint
    realGeometry: mapTool.recordedGeometry
  }

  MM.MMHighlightItem {
    id: guideline

    height: root.map.height
    width: root.map.width

    markerColor: __style.deepOceanColor
    lineColor: __style.deepOceanColor
    lineStrokeStyle: ShapePath.DashLine
    lineWidth: MM.MMHighlightItem.Narrow

    mapSettings: root.map.mapSettings
    geometry: guidelineController.guidelineGeometry
  }

  MM.MMHighlightItem {
    id: highlight

    height: map.height
    width: map.width

    markerColor: __style.deepOceanColor
    lineColor: __style.deepOceanColor
    lineWidth: MM.MMHighlightItem.Narrow

    mapSettings: root.map.mapSettings
    geometry: mapTool.recordedGeometry
  }

  MM.MMHighlightItem {
    id: existingVerticesHighlight

    height: root.map.height
    width: root.map.width

    mapSettings: root.map.mapSettings
    geometry: mapTool.existingVertices

    markerType: MM.MMHighlightItem.Circle
    markerSize: MM.MMHighlightItem.Bigger
  }

  MMMeasureCrosshair {
    id: crosshair

    anchors.fill: parent
    qgsProject: __activeProject.qgsProject
    mapSettings: root.map.mapSettings
    visible: !mapTool.measurementFinalized

    text: __inputUtils.formatDistanceInProjectUnit( mapTool.lengthWithGuideline, 1, __activeProject.qgsProject )
    canCloseShape: mapTool.canCloseShape

    onCloseShapeClicked: root.mapTool.finalizeMeasurement( true )
  }
}
