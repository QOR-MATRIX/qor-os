// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "extensions.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QUuid>
#include <QtGui/QCursor>
#include <QtQuick/QQuickItem>
#include <QtWaylandCompositor/QWaylandClient>
#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandQuickShellIntegration>
#include <QtWaylandCompositor/QWaylandQuickShellSurfaceItem>
#include <QtWaylandCompositor/QWaylandSeat>
#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandView>
#include <QtWaylandCompositor/QWaylandXdgShell>

#include <cmath>

Q_LOGGING_CATEGORY(lcExtensions, "qtos.prism.extensions")

namespace qtos::prism {

namespace {

QWaylandCompositor *compositorOf(QWaylandCompositorExtension *extension)
{
    return qobject_cast<QWaylandCompositor *>(extension->extensionContainer());
}

} // namespace

// ── Fractional scale ─────────────────────────────────────────────────────────

// One wp_fractional_scale_v1 per surface. It outlives its surface if the
// client keeps it; it then simply receives no further scale changes.
class FractionalScale : public QtWaylandServer::wp_fractional_scale_v1
{
public:
    FractionalScale(FractionalScaleManager *manager, QWaylandSurface *surface, wl_client *client, uint32_t id,
                    int version)
        : wp_fractional_scale_v1(client, id, version), m_manager(manager), m_surface(surface)
    { }

    QWaylandSurface *surface() const { return m_surface; }

    void sendScale(qreal scale)
    {
        // The protocol carries the scale as a numerator over 120.
        send_preferred_scale(uint32_t(std::lround(scale * 120.0)));
    }

protected:
    void wp_fractional_scale_v1_destroy(Resource *resource) override { wl_resource_destroy(resource->handle); }

    void wp_fractional_scale_v1_destroy_resource(Resource *) override
    {
        if (m_manager)
            m_manager->forget(this);
        delete this;
    }

private:
    QPointer<FractionalScaleManager> m_manager;
    QPointer<QWaylandSurface> m_surface;
};

FractionalScaleManager::~FractionalScaleManager() = default;

void FractionalScaleManager::setPreferredScale(qreal scale)
{
    if (scale <= 0 || qFuzzyCompare(scale, m_preferredScale))
        return;
    m_preferredScale = scale;
    for (FractionalScale *fractional : std::as_const(m_scales))
        fractional->sendScale(m_preferredScale);
    Q_EMIT preferredScaleChanged();
}

void FractionalScaleManager::initialize()
{
    QWaylandCompositorExtensionTemplate::initialize();
    QWaylandCompositor *compositor = compositorOf(this);
    if (!compositor) {
        qCWarning(lcExtensions, "FractionalScaleManagerV1 must be declared inside a WaylandCompositor");
        return;
    }
    init(compositor->display(), 1);
}

void FractionalScaleManager::componentComplete()
{
    if (!isInitialized())
        initialize();
}

void FractionalScaleManager::wp_fractional_scale_manager_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void FractionalScaleManager::wp_fractional_scale_manager_v1_get_fractional_scale(Resource *resource, uint32_t id,
                                                                                 wl_resource *surfaceResource)
{
    QWaylandSurface *surface = QWaylandSurface::fromResource(surfaceResource);
    if (!surface)
        return;
    if (m_scales.contains(surface)) {
        wl_resource_post_error(resource->handle, error_fractional_scale_exists,
                               "the surface already has a fractional_scale object");
        return;
    }
    auto *fractional = new FractionalScale(this, surface, resource->client(), id, resource->version());
    m_scales.insert(surface, fractional);
    connect(surface, &QWaylandSurface::surfaceDestroyed, this, [this, surface] { m_scales.remove(surface); });
    fractional->sendScale(m_preferredScale);
}

void FractionalScaleManager::forget(FractionalScale *scale)
{
    for (auto it = m_scales.begin(); it != m_scales.end(); ++it) {
        if (it.value() == scale) {
            m_scales.erase(it);
            return;
        }
    }
}

// ── Cursor shape ─────────────────────────────────────────────────────────────

class CursorShapeDevice : public QtWaylandServer::wp_cursor_shape_device_v1
{
public:
    CursorShapeDevice(CursorShapeManager *manager, bool tabletTool, wl_client *client, uint32_t id, int version)
        : wp_cursor_shape_device_v1(client, id, version), m_manager(manager), m_tabletTool(tabletTool)
    { }

protected:
    void wp_cursor_shape_device_v1_destroy(Resource *resource) override { wl_resource_destroy(resource->handle); }
    void wp_cursor_shape_device_v1_destroy_resource(Resource *) override { delete this; }

    void wp_cursor_shape_device_v1_set_shape(Resource *resource, uint32_t /*serial*/, uint32_t requested) override
    {
        const std::optional<Qt::CursorShape> cursor = CursorShapeManager::toQtCursor(requested, resource->version());
        if (!cursor) {
            wl_resource_post_error(resource->handle, error_invalid_shape, "unknown cursor shape %u", requested);
            return;
        }
        // Prism has no tablet input yet, so tablet-tool cursors have nowhere to show.
        if (m_manager && !m_tabletTool)
            m_manager->applyShape(resource->client(), *cursor);
    }

private:
    QPointer<CursorShapeManager> m_manager;
    bool m_tabletTool;
};

void CursorShapeManager::initialize()
{
    QWaylandCompositorExtensionTemplate::initialize();
    QWaylandCompositor *compositor = compositorOf(this);
    if (!compositor) {
        qCWarning(lcExtensions, "CursorShapeManagerV1 must be declared inside a WaylandCompositor");
        return;
    }
    init(compositor->display(), 2);
}

void CursorShapeManager::componentComplete()
{
    if (!isInitialized())
        initialize();
}

std::optional<Qt::CursorShape> CursorShapeManager::toQtCursor(uint32_t shape, int version)
{
    using Device = QtWaylandServer::wp_cursor_shape_device_v1;
    switch (shape) {
    case Device::shape_default:
    case Device::shape_context_menu:
    case Device::shape_zoom_in:
    case Device::shape_zoom_out: return Qt::ArrowCursor;
    case Device::shape_help: return Qt::WhatsThisCursor;
    case Device::shape_pointer: return Qt::PointingHandCursor;
    case Device::shape_progress: return Qt::BusyCursor;
    case Device::shape_wait: return Qt::WaitCursor;
    case Device::shape_cell:
    case Device::shape_crosshair: return Qt::CrossCursor;
    case Device::shape_text:
    case Device::shape_vertical_text: return Qt::IBeamCursor;
    case Device::shape_alias: return Qt::DragLinkCursor;
    case Device::shape_copy: return Qt::DragCopyCursor;
    case Device::shape_move: return Qt::DragMoveCursor;
    case Device::shape_no_drop:
    case Device::shape_not_allowed: return Qt::ForbiddenCursor;
    case Device::shape_grab: return Qt::OpenHandCursor;
    case Device::shape_grabbing: return Qt::ClosedHandCursor;
    case Device::shape_e_resize:
    case Device::shape_w_resize:
    case Device::shape_ew_resize: return Qt::SizeHorCursor;
    case Device::shape_n_resize:
    case Device::shape_s_resize:
    case Device::shape_ns_resize: return Qt::SizeVerCursor;
    case Device::shape_ne_resize:
    case Device::shape_sw_resize:
    case Device::shape_nesw_resize: return Qt::SizeBDiagCursor;
    case Device::shape_nw_resize:
    case Device::shape_se_resize:
    case Device::shape_nwse_resize: return Qt::SizeFDiagCursor;
    case Device::shape_col_resize: return Qt::SplitHCursor;
    case Device::shape_row_resize: return Qt::SplitVCursor;
    case Device::shape_all_scroll: return Qt::SizeAllCursor;
    case Device::shape_dnd_ask: return version >= 2 ? std::optional(Qt::DragCopyCursor) : std::nullopt;
    case Device::shape_all_resize: return version >= 2 ? std::optional(Qt::SizeAllCursor) : std::nullopt;
    default: return std::nullopt;
    }
}

void CursorShapeManager::applyShape(wl_client *client, Qt::CursorShape shape)
{
    QWaylandCompositor *compositor = compositorOf(this);
    QWaylandSeat *seat = compositor ? compositor->defaultSeat() : nullptr;
    QWaylandView *view = seat ? seat->mouseFocus() : nullptr;
    QWaylandSurface *surface = view ? view->surface() : nullptr;
    if (!surface || !surface->client() || surface->client()->client() != client)
        return;
    if (auto *item = qobject_cast<QQuickItem *>(view->renderObject()))
        item->setCursor(QCursor(shape));
}

void CursorShapeManager::wp_cursor_shape_manager_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void CursorShapeManager::wp_cursor_shape_manager_v1_get_pointer(Resource *resource, uint32_t id, wl_resource *)
{
    new CursorShapeDevice(this, false, resource->client(), id, resource->version());
}

void CursorShapeManager::wp_cursor_shape_manager_v1_get_tablet_tool_v2(Resource *resource, uint32_t id, wl_resource *)
{
    new CursorShapeDevice(this, true, resource->client(), id, resource->version());
}

// ── Activation ───────────────────────────────────────────────────────────────

namespace {
constexpr qint64 kTokenLifetimeMs = 60000;
}

// An xdg_activation_token_v1 being built by a client: surface, serial and app
// id, then commit, which answers with the token string.
class ActivationToken : public QtWaylandServer::xdg_activation_token_v1
{
public:
    ActivationToken(ActivationManager *manager, wl_client *client, uint32_t id, int version)
        : xdg_activation_token_v1(client, id, version), m_manager(manager)
    { }

protected:
    // Trust is decided from keyboard focus at commit, so serial, app id and
    // surface only need to arrive before it.
    void xdg_activation_token_v1_set_serial(Resource *resource, uint32_t, wl_resource *) override
    {
        checkNotCommitted(resource);
    }
    void xdg_activation_token_v1_set_app_id(Resource *resource, const QString &) override
    {
        checkNotCommitted(resource);
    }
    void xdg_activation_token_v1_set_surface(Resource *resource, wl_resource *) override
    {
        checkNotCommitted(resource);
    }
    void xdg_activation_token_v1_commit(Resource *resource) override
    {
        if (!checkNotCommitted(resource) || !m_manager)
            return;
        m_committed = true;
        // Trusted when the requesting client holds keyboard focus right now.
        bool trusted = false;
        auto *compositor = qobject_cast<QWaylandCompositor *>(m_manager->extensionContainer());
        if (QWaylandSeat *seat = compositor ? compositor->defaultSeat() : nullptr) {
            QWaylandSurface *focus = seat->keyboardFocus();
            trusted = focus && focus->client() && focus->client()->client() == resource->client();
        }
        send_done(m_manager->issue(trusted));
    }
    void xdg_activation_token_v1_destroy(Resource *resource) override { wl_resource_destroy(resource->handle); }
    void xdg_activation_token_v1_destroy_resource(Resource *) override { delete this; }

private:
    bool checkNotCommitted(Resource *resource)
    {
        if (m_committed)
            wl_resource_post_error(resource->handle, error_already_used, "the activation token was already used");
        return !m_committed;
    }

    QPointer<ActivationManager> m_manager;
    bool m_committed = false;
};

void ActivationManager::initialize()
{
    QWaylandCompositorExtensionTemplate::initialize();
    QWaylandCompositor *compositor = compositorOf(this);
    if (!compositor) {
        qCWarning(lcExtensions, "XdgActivationV1 must be declared inside a WaylandCompositor");
        return;
    }
    init(compositor->display(), 1);
}

void ActivationManager::componentComplete()
{
    if (!isInitialized())
        initialize();
}

QString ActivationManager::createToken() { return issue(true); }

QString ActivationManager::issue(bool trusted)
{
    // Forget expired tokens while issuing new ones.
    for (auto it = m_tokens.begin(); it != m_tokens.end();)
        it = it->age.elapsed() > kTokenLifetimeMs ? m_tokens.erase(it) : std::next(it);
    const QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    Issued issued;
    issued.trusted = trusted;
    issued.age.start();
    m_tokens.insert(token, issued);
    return token;
}

void ActivationManager::xdg_activation_v1_destroy(Resource *resource) { wl_resource_destroy(resource->handle); }

void ActivationManager::xdg_activation_v1_get_activation_token(Resource *resource, uint32_t id)
{
    new ActivationToken(this, resource->client(), id, resource->version());
}

void ActivationManager::xdg_activation_v1_activate(Resource *, const QString &token, wl_resource *surfaceResource)
{
    const auto it = m_tokens.constFind(token);
    if (it == m_tokens.constEnd() || it->age.elapsed() > kTokenLifetimeMs) {
        // Unknown or expired: the request is ignored, as the protocol allows.
        return;
    }
    const bool trusted = it->trusted;
    m_tokens.erase(it);
    if (QWaylandSurface *surface = QWaylandSurface::fromResource(surfaceResource))
        Q_EMIT activationRequested(surface, trusted);
}

// ── Window management ────────────────────────────────────────────────────────

WindowManagement::WindowManagement(QObject *parent) : QObject(parent) { }

void WindowManagement::takeOver(QQuickItem *shellSurfaceItem)
{
    auto *item = qobject_cast<QWaylandQuickShellSurfaceItem *>(shellSurfaceItem);
    auto *xdgSurface = item ? qobject_cast<QWaylandXdgSurface *>(item->shellSurface()) : nullptr;
    QWaylandXdgToplevel *toplevel = xdgSurface ? xdgSurface->toplevel() : nullptr;
    if (!toplevel)
        return;
    // The integration is created with the item as its parent.
    const auto integrations = item->findChildren<QWaylandQuickShellIntegration *>(Qt::FindDirectChildrenOnly);
    for (QWaylandQuickShellIntegration *integration : integrations) {
        QObject::disconnect(toplevel, &QWaylandXdgToplevel::startMove, integration, nullptr);
        QObject::disconnect(toplevel, &QWaylandXdgToplevel::startResize, integration, nullptr);
        QObject::disconnect(toplevel, &QWaylandXdgToplevel::setMaximized, integration, nullptr);
        QObject::disconnect(toplevel, &QWaylandXdgToplevel::unsetMaximized, integration, nullptr);
        QObject::disconnect(toplevel, &QWaylandXdgToplevel::maximizedChanged, integration, nullptr);
        QObject::disconnect(toplevel, &QWaylandXdgToplevel::setFullscreen, integration, nullptr);
        QObject::disconnect(toplevel, &QWaylandXdgToplevel::unsetFullscreen, integration, nullptr);
        QObject::disconnect(toplevel, &QWaylandXdgToplevel::fullscreenChanged, integration, nullptr);
    }
}

// ── Presentation feedback ────────────────────────────────────────────────────

PresentationFeedback::PresentationFeedback(QObject *parent) : QObject(parent) { }

void PresentationFeedback::setPresentationTime(QObject *presentationTime)
{
    if (m_presentationTime == presentationTime)
        return;
    m_presentationTime = presentationTime;

    // Attach PresentationTime to the enclosing compositor and initialise it
    // now; this runs while bindings are set, before its own componentComplete.
    auto *extension = qobject_cast<QWaylandCompositorExtension *>(presentationTime);
    if (extension && !extension->isInitialized()) {
        for (QObject *ancestor = extension->parent(); ancestor; ancestor = ancestor->parent()) {
            if (auto *compositor = qobject_cast<QWaylandCompositor *>(ancestor)) {
                extension->setExtensionContainer(compositor);
                extension->initialize();
                break;
            }
        }
    }
    Q_EMIT presentationTimeChanged();
}

void PresentationFeedback::setWindow(QQuickWindow *window)
{
    if (m_window == window)
        return;
    disconnect(m_connection);
    m_window = window;
    if (m_window) {
        // frameSwapped arrives on the render thread: take the timestamp there,
        // then report from the GUI thread, which owns the Wayland display.
        m_connection = connect(
            m_window, &QQuickWindow::frameSwapped, this,
            [this] {
                timespec now {};
                clock_gettime(CLOCK_MONOTONIC, &now);
                QMetaObject::invokeMethod(this, [this, now] { report(now); }, Qt::QueuedConnection);
            },
            Qt::DirectConnection);
    }
    Q_EMIT windowChanged();
}

void PresentationFeedback::report(timespec presented)
{
    if (!m_presentationTime || !m_window)
        return;
    QMetaObject::invokeMethod(m_presentationTime, "sendFeedback", Q_ARG(QQuickWindow *, m_window.data()),
                              Q_ARG(quint64, ++m_sequence), Q_ARG(quint64, quint64(presented.tv_sec)),
                              Q_ARG(quint32, quint32(presented.tv_nsec)));
}

} // namespace qtos::prism
