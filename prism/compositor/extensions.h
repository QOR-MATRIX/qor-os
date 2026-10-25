// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtQml/QQmlParserStatus>
#include <QtQml/qqmlregistration.h>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtWaylandCompositor/QWaylandCompositorExtensionTemplate>

#include "qwayland-server-cursor-shape-v1.h"
#include "qwayland-server-fractional-scale-v1.h"
#include "qwayland-server-xdg-activation-v1.h"

#include <ctime>
#include <optional>

class QWaylandSurface;

// Wayland protocol extensions Prism adds to what Qt Wayland Compositor
// provides. Each is a QML type declared inside the WaylandCompositor in
// Compositor.qml and initialised when that component completes, like Qt's own
// extensions. Protocol XML is vendored in protocols/ (wayland-protocols 1.47).
namespace qtos::prism {

class FractionalScale;

// wp_fractional_scale_manager_v1: tells clients the scale to render at, in
// 1/120 steps (1.25 → 150), so text and icons stay crisp on fractional
// display scaling instead of being resampled. Clients pair it with
// wp_viewporter, which Qt Wayland Compositor already provides.
class FractionalScaleManager : public QWaylandCompositorExtensionTemplate<FractionalScaleManager>,
                               public QtWaylandServer::wp_fractional_scale_manager_v1,
                               public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(FractionalScaleManagerV1)
    Q_PROPERTY(qreal preferredScale READ preferredScale WRITE setPreferredScale NOTIFY preferredScaleChanged FINAL)

public:
    ~FractionalScaleManager() override;

    qreal preferredScale() const { return m_preferredScale; }
    void setPreferredScale(qreal scale);

    void initialize() override;
    void classBegin() override { }
    void componentComplete() override;

    static const wl_interface *interface() { return wp_fractional_scale_manager_v1::interface(); }

Q_SIGNALS:
    void preferredScaleChanged();

protected:
    void wp_fractional_scale_manager_v1_destroy(Resource *resource) override;
    void wp_fractional_scale_manager_v1_get_fractional_scale(Resource *resource, uint32_t id,
                                                             wl_resource *surface) override;

private:
    friend class FractionalScale;
    void forget(FractionalScale *scale);

    qreal m_preferredScale = 1.0;
    QHash<QWaylandSurface *, FractionalScale *> m_scales;
};

// wp_cursor_shape_manager_v1: clients name a cursor ("text", "pointer",
// "ew-resize", ...) instead of uploading cursor images. Prism shows the
// matching Qt cursor over that client's surface.
class CursorShapeManager : public QWaylandCompositorExtensionTemplate<CursorShapeManager>,
                           public QtWaylandServer::wp_cursor_shape_manager_v1,
                           public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(CursorShapeManagerV1)

public:
    void initialize() override;
    void classBegin() override { }
    void componentComplete() override;

    static const wl_interface *interface() { return wp_cursor_shape_manager_v1::interface(); }

    // The Qt cursor for a protocol shape, or nothing for an unknown value.
    static std::optional<Qt::CursorShape> toQtCursor(uint32_t shape, int version);

    // Shows `shape` over the surface under the pointer if it belongs to
    // `client`. The request serial is not checked: a stale request can only
    // change the cursor over the requesting client's own surface.
    void applyShape(wl_client *client, Qt::CursorShape shape);

protected:
    void wp_cursor_shape_manager_v1_destroy(Resource *resource) override;
    void wp_cursor_shape_manager_v1_get_pointer(Resource *resource, uint32_t id, wl_resource *pointer) override;
    void wp_cursor_shape_manager_v1_get_tablet_tool_v2(Resource *resource, uint32_t id,
                                                       wl_resource *tabletTool) override;
};

class ActivationToken;

// xdg_activation_v1: a client asks for one of its surfaces to be focused,
// presenting a token. Focus-stealing prevention decides what happens:
//   - trusted tokens, issued while the requesting client had keyboard focus or
//     by Prism itself (createToken(), for launches), activate the window;
//   - any other token only asks for attention, and focus stays where it is.
// Tokens are single-use and expire after a minute.
class ActivationManager : public QWaylandCompositorExtensionTemplate<ActivationManager>,
                          public QtWaylandServer::xdg_activation_v1,
                          public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(XdgActivationV1)

public:
    void initialize() override;
    void classBegin() override { }
    void componentComplete() override;

    static const wl_interface *interface() { return xdg_activation_v1::interface(); }

    // A trusted token for an app Prism launches (XDG_ACTIVATION_TOKEN).
    Q_INVOKABLE QString createToken();

Q_SIGNALS:
    // `surface` is a QWaylandSurface; `trusted` decides focus or attention.
    void activationRequested(QObject *surface, bool trusted);

protected:
    void xdg_activation_v1_destroy(Resource *resource) override;
    void xdg_activation_v1_get_activation_token(Resource *resource, uint32_t id) override;
    void xdg_activation_v1_activate(Resource *resource, const QString &token, wl_resource *surface) override;

private:
    friend class ActivationToken;
    QString issue(bool trusted);

    struct Issued
    {
        bool trusted = false;
        QElapsedTimer age;
    };
    QHash<QString, Issued> m_tokens;
};

// QML singleton `WindowManagement`: makes Prism the only window manager for
// a client surface item.
//
// Qt's xdg-shell integration for a ShellSurfaceItem reacts to the client's
// move, resize, maximise and fullscreen requests itself (it moves the item,
// sends its own resize and maximise configures). Prism handles all of these
// with its own springs, snapping, work area and size limits, and two
// handlers fighting over one window give inconsistent sizes. takeOver()
// disconnects Qt's handlers for those requests; popups and activation stay
// with Qt.
class WindowManagement : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit WindowManagement(QObject *parent = nullptr);

    Q_INVOKABLE void takeOver(QQuickItem *shellSurfaceItem);
};

// Feeds wp_presentation (Qt's PresentationTime) with every frame of `window`,
// timestamped on CLOCK_MONOTONIC when the swap completes. Nested, Prism has no
// vblank timestamps, so the swap time is the closest estimate available.
//
// It also initialises the PresentationTime element: in Qt 6.12 that element
// looks for its compositor before it is attached to one, so declared in QML it
// never initialises on its own.
class PresentationFeedback : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        QObject *presentationTime READ presentationTime WRITE setPresentationTime NOTIFY presentationTimeChanged FINAL)
    Q_PROPERTY(QQuickWindow *window READ window WRITE setWindow NOTIFY windowChanged FINAL)

public:
    explicit PresentationFeedback(QObject *parent = nullptr);

    QObject *presentationTime() const { return m_presentationTime; }
    void setPresentationTime(QObject *presentationTime);
    QQuickWindow *window() const { return m_window; }
    void setWindow(QQuickWindow *window);

Q_SIGNALS:
    void presentationTimeChanged();
    void windowChanged();

private:
    void report(timespec presented);

    QPointer<QObject> m_presentationTime;
    QPointer<QQuickWindow> m_window;
    QMetaObject::Connection m_connection;
    quint64 m_sequence = 0;
};

} // namespace qtos::prism
