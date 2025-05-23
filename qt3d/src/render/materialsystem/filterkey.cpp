/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "filterkey_p.h"
#include <Qt3DRender/private/qfilterkey_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace Qt3DRender {
namespace Render {

FilterKey::FilterKey()
    : BackendNode()
{
}

FilterKey::~FilterKey()
{
    cleanup();
}

void FilterKey::cleanup()
{
    QBackendNode::setEnabled(false);
    m_name.clear();
    m_value.clear();
}

void FilterKey::syncFromFrontEnd(const QNode *frontEnd, bool firstTime)
{
    const QFilterKey *node = qobject_cast<const QFilterKey *>(frontEnd);
    if (!node)
        return;

    BackendNode::syncFromFrontEnd(frontEnd, firstTime);

    if (node->name() != m_name) {
        m_name = node->name();
        markDirty(AbstractRenderer::AllDirty);
    }

    if (node->value() != m_value) {
        m_value = node->value();
        markDirty(AbstractRenderer::AllDirty);
    }
}

bool FilterKey::equals(const FilterKey &other) const
{
    if (&other == this)
        return true;
    // TODO create a QVaraint::fastCompare function that returns false
    // if types are not equal. For now, applying
    // https://codereview.qt-project.org/#/c/204484/
    // and adding the following early comparison of the types should give
    // an equivalent performance gain:
    return (other.value().type() == value().type() &&
            other.name() == name() &&
            other.value() == value());
}

} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE
