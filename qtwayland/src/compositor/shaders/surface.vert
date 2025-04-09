/****************************************************************************
**
** Copyright (C) 2023 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWaylandCompositor module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
****************************************************************************/

uniform highp mat4 qt_Matrix;
attribute highp vec2 qt_VertexPosition;
attribute highp vec2 qt_VertexTexCoord;
varying highp vec2 v_texcoord;

void main() {
   gl_Position = qt_Matrix * vec4(qt_VertexPosition, 0.0, 1.0);
   v_texcoord = qt_VertexTexCoord;
}
