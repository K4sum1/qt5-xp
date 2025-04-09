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

uniform highp sampler2D tex0;
uniform highp sampler2D tex1;
varying highp vec2 v_texcoord;
uniform lowp float qt_Opacity;

void main() {
  float y = 1.16438356 * (texture2D(tex0, v_texcoord).x - 0.0625);
  float u = texture2D(tex1, v_texcoord).r - 0.5;
  float v = texture2D(tex1, v_texcoord).g - 0.5;
  y *= qt_Opacity;
  u *= qt_Opacity;
  v *= qt_Opacity;
  gl_FragColor.r = y + 1.59602678 * v;
  gl_FragColor.g = y - 0.39176229 * u - 0.81296764 * v;
  gl_FragColor.b = y + 2.01723214 * u;
  gl_FragColor.a = qt_Opacity;
}
