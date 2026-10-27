// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "scenerect.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

namespace qor {

SceneRect::SceneRect(QObject *parent) : QObject(parent) { }

void SceneRect::setItem(QQuickItem *item)
{
    if (m_item == item)
        return;
    if (m_item)
        disconnect(m_item, nullptr, this, nullptr);
    m_item = item;
    if (m_item) {
        connect(m_item, &QQuickItem::windowChanged, this, &SceneRect::attachWindow);
        connect(m_item, &QQuickItem::widthChanged, this, &SceneRect::refresh);
        connect(m_item, &QQuickItem::heightChanged, this, &SceneRect::refresh);
        attachWindow(m_item->window());
    } else {
        attachWindow(nullptr);
    }
    Q_EMIT itemChanged();
    refresh();
}

void SceneRect::setRelativeTo(QQuickItem *item)
{
    if (m_relativeTo == item)
        return;
    m_relativeTo = item;
    Q_EMIT relativeToChanged();
    refresh();
}

void SceneRect::attachWindow(QQuickWindow *window)
{
    if (m_window == window)
        return;
    QObject::disconnect(m_frameConnection);
    m_window = window;
    if (m_window)
        m_frameConnection = connect(m_window, &QQuickWindow::afterAnimating, this, &SceneRect::refresh);
}

void SceneRect::refresh()
{
    if (!m_item)
        return;
    const QRectF local(0, 0, m_item->width(), m_item->height());
    const QRectF mapped = m_relativeTo ? m_item->mapRectToItem(m_relativeTo, local) : m_item->mapRectToScene(local);
    if (mapped == m_rect)
        return;
    m_rect = mapped;
    Q_EMIT rectChanged();
}

} // namespace qor
