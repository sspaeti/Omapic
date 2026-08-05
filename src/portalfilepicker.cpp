#include "portalfilepicker.h"

#include <QDBusConnection>
#include <QDBusArgument>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>

namespace {
struct PortalFilterRule {
    uint type;
    QString pattern;
};

using PortalFilterRules = QList<PortalFilterRule>;

struct PortalFileFilter {
    QString name;
    PortalFilterRules rules;
};

using PortalFileFilters = QList<PortalFileFilter>;

QDBusArgument &operator<<(QDBusArgument &argument, const PortalFilterRule &rule) {
    argument.beginStructure();
    argument << rule.type << rule.pattern;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, PortalFilterRule &rule) {
    argument.beginStructure();
    argument >> rule.type >> rule.pattern;
    argument.endStructure();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const PortalFileFilter &filter) {
    argument.beginStructure();
    argument << filter.name << filter.rules;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, PortalFileFilter &filter) {
    argument.beginStructure();
    argument >> filter.name >> filter.rules;
    argument.endStructure();
    return argument;
}

void registerPortalFilterTypes() {
    static const bool registered = [] {
        qDBusRegisterMetaType<PortalFilterRule>();
        qDBusRegisterMetaType<PortalFilterRules>();
        qDBusRegisterMetaType<PortalFileFilter>();
        qDBusRegisterMetaType<PortalFileFilters>();
        return true;
    }();
    Q_UNUSED(registered);
}

PortalFileFilter imageFilter() {
    return {
        QStringLiteral("Images"),
        {
            {1, QStringLiteral("image/*")},
            {0, QStringLiteral("*.png")},
            {0, QStringLiteral("*.jpg")},
            {0, QStringLiteral("*.jpeg")},
            {0, QStringLiteral("*.webp")},
            {0, QStringLiteral("*.bmp")},
        },
    };
}

PortalFileFilters imageFilters() {
    return {imageFilter(), {QStringLiteral("All files"), {{0, QStringLiteral("*")}}}};
}

QString portalToken() {
    return QStringLiteral("omapic_%1").arg(QRandomGenerator::global()->generate());
}

QByteArray portalPathBytes(const QString &path) {
    QByteArray bytes = path.toUtf8();
    bytes.append('\0');
    return bytes;
}
}

PortalFilePicker::PortalFilePicker(QObject *parent) : FilePicker(parent) {
    registerPortalFilterTypes();
}

void PortalFilePicker::openImage() {
    QVariantMap options;
    options.insert(QStringLiteral("accept_label"), QStringLiteral("Open"));
    options.insert(QStringLiteral("modal"), true);
    options.insert(QStringLiteral("multiple"), false);
    options.insert(QStringLiteral("current_folder"), portalPathBytes(QDir::homePath()));
    options.insert(QStringLiteral("filters"), QVariant::fromValue(imageFilters()));
    options.insert(QStringLiteral("current_filter"), QVariant::fromValue(imageFilter()));

    requestFile(QStringLiteral("OpenFile"), QStringLiteral("Open Image File"), options, Action::Open);
}

void PortalFilePicker::saveImage(const QString &suggestedName) {
    QVariantMap options;
    options.insert(QStringLiteral("accept_label"), QStringLiteral("Save"));
    options.insert(QStringLiteral("modal"), true);
    options.insert(QStringLiteral("current_folder"), portalPathBytes(QDir::homePath()));
    options.insert(QStringLiteral("current_name"),
                   suggestedName.isEmpty() ? QStringLiteral("omapic.png") : suggestedName);
    options.insert(QStringLiteral("filters"), QVariant::fromValue(imageFilters()));
    options.insert(QStringLiteral("current_filter"), QVariant::fromValue(imageFilter()));

    requestFile(QStringLiteral("SaveFile"), QStringLiteral("Save Image As"), options, Action::Save);
}

bool PortalFilePicker::connectToRequestPath(const QString &path) {
    m_pendingPath = path;
    return QDBusConnection::sessionBus().connect(
        QStringLiteral("org.freedesktop.portal.Desktop"), m_pendingPath,
        QStringLiteral("org.freedesktop.portal.Request"), QStringLiteral("Response"),
        this, SLOT(handleResponse(uint,QVariantMap)));
}

bool PortalFilePicker::requestFile(const QString &method, const QString &title,
                                   QVariantMap options, Action action) {
    if (m_pendingAction != Action::None)
        return false;

    QDBusConnection bus = QDBusConnection::sessionBus();
    QDBusInterface portal(QStringLiteral("org.freedesktop.portal.Desktop"),
                          QStringLiteral("/org/freedesktop/portal/desktop"),
                          QStringLiteral("org.freedesktop.portal.FileChooser"),
                          bus);
    if (!portal.isValid()) {
        emit failed(QStringLiteral("The XDG desktop portal file chooser is not available."));
        return false;
    }

    // Subscribe to the Response signal at the request path the portal will
    // derive from our handle_token *before* making the call, so a response
    // can't slip past while our match rule is still being installed.
    const QString token = portalToken();
    options.insert(QStringLiteral("handle_token"), token);
    QString sender = bus.baseService().mid(1);
    sender.replace(QLatin1Char('.'), QLatin1Char('_'));
    const QString predictedPath =
        QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, token);

    m_pendingAction = action;
    if (!connectToRequestPath(predictedPath)) {
        clearPending();
        emit failed(QStringLiteral("Could not listen for the portal file picker response."));
        return false;
    }

    auto *watcher = new QDBusPendingCallWatcher(
        portal.asyncCall(method, QString(), title, options), this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        watcher->deleteLater();

        // A response may already have been handled while the reply was in flight.
        if (m_pendingAction == Action::None)
            return;

        if (reply.isError()) {
            clearPending();
            emit failed(QStringLiteral("The portal file picker failed: %1")
                            .arg(reply.error().message()));
            return;
        }

        // Old portal versions can hand back a different request path than the
        // handle_token predicts; move our subscription over if so.
        const QString actualPath = reply.value().path();
        if (actualPath != m_pendingPath) {
            QDBusConnection::sessionBus().disconnect(
                QStringLiteral("org.freedesktop.portal.Desktop"), m_pendingPath,
                QStringLiteral("org.freedesktop.portal.Request"), QStringLiteral("Response"),
                this, SLOT(handleResponse(uint,QVariantMap)));
            if (!connectToRequestPath(actualPath)) {
                clearPending();
                emit failed(QStringLiteral("Could not listen for the portal file picker response."));
            }
        }
    });
    return true;
}

void PortalFilePicker::handleResponse(uint response, const QVariantMap &results) {
    const Action action = m_pendingAction;
    clearPending();

    if (response != 0)
        return;

    const QStringList uris = results.value(QStringLiteral("uris")).toStringList();
    if (uris.isEmpty())
        return;

    const QUrl url(uris.first());
    if (action == Action::Open)
        emit openSelected(url);
    else if (action == Action::Save)
        emit saveSelected(url);
}

void PortalFilePicker::clearPending() {
    if (!m_pendingPath.isEmpty()) {
        QDBusConnection::sessionBus().disconnect(
            QStringLiteral("org.freedesktop.portal.Desktop"), m_pendingPath,
            QStringLiteral("org.freedesktop.portal.Request"), QStringLiteral("Response"),
            this, SLOT(handleResponse(uint,QVariantMap)));
    }

    m_pendingPath.clear();
    m_pendingAction = Action::None;
}
