// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QRectF>
#include <QtQml/qqmlregistration.h>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

namespace qor {

// Tracks an item's bounding rectangle in another item's (or the scene's)
// coordinate system, including every ancestor transform.
//
// QML bindings on mapToItem() do not re-evaluate when an ancestor moves or
// scales. Lumen materials need the exact on-screen rectangle every frame to
// sample their backdrop, so SceneRect re-maps on QQuickWindow::afterAnimating —
// once per rendered frame, on the GUI thread, before the scene graph syncs —
// and never wakes the render loop on its own.
class SceneRect : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQuickItem *item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(QQuickItem *relativeTo READ relativeTo WRITE setRelativeTo NOTIFY relativeToChanged FINAL)
    Q_PROPERTY(QRectF rect READ rect NOTIFY rectChanged FINAL)

public:
    explicit SceneRect(QObject *parent = nullptr);

    QQuickItem *item() const { return m_item; }
    void setItem(QQuickItem *item);

    QQuickItem *relativeTo() const { return m_relativeTo; }
    void setRelativeTo(QQuickItem *item);

    QRectF rect() const { return m_rect; }

    Q_INVOKABLE void refresh();

Q_SIGNALS:
    void itemChanged();
    void relativeToChanged();
    void rectChanged();

private:
    void attachWindow(QQuickWindow *window);

    QPointer<QQuickItem> m_item;
    QPointer<QQuickItem> m_relativeTo;
    QPointer<QQuickWindow> m_window;
    QMetaObject::Connection m_frameConnection;
    QRectF m_rect;
};

} // namespace qor
