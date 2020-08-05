#include "Manager.h"
#include "Widget.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QDBusArgument>

Manager::Manager(QObject *parent) : QObject(parent)
{
    m_unmuteIcon = new QSystemTrayIcon(this);
    connect(m_unmuteIcon, &QSystemTrayIcon::activated, this, &Manager::onUnmute);

    m_unmuteTimer = new QTimer(this);
    m_unmuteTimer->setSingleShot(true);
    connect(m_unmuteTimer, &QTimer::timeout, this, &Manager::onUnmute);
}

static const char *serviceName = "org.freedesktop.Notifications";
static const char *objectPath = "/org/freedesktop/Notifications";

bool Manager::init()
{
    if (!QDBusConnection::sessionBus().isConnected()) {
        qWarning() << "Failed to connect to session bus";
        return false;
    }

    if (!QDBusConnection::sessionBus().registerService(serviceName)) {
        qWarning() << "Failed to register service";
        return false;
    }

    if (!QDBusConnection::sessionBus().registerObject(objectPath, this, QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals)) {
        qWarning() << "Failed to register object";
        return false;
    }

    return true;
}

QImage decodeIcon(const QDBusArgument &crap)
{
    int width, height, bytesPerLine, hasAlpha, bitsPerPixel, channels;
    QByteArray data;

    crap.beginStructure();
    crap >> width >> height >> bytesPerLine >> hasAlpha >> bitsPerPixel >> channels >> data;
    crap.endStructure();

    if (width < 0 || width > 1000 || height < 0 || height > 1000) {
        qWarning() << "Invalid size" << width << height;
        return QImage();
    }

    if (bitsPerPixel != 8) {
        qWarning() << "Unsupported depth" << bitsPerPixel;
        return QImage();
    }

    if (channels == 4) {
        if (!hasAlpha) {
            qWarning() << "Invalid number of channels without alpha" << channels << hasAlpha;
            return QImage();
        }
    } else if (channels == 3) {
        if (hasAlpha) {
            qWarning() << "Invalid number of channels with alpha" << channels << hasAlpha;
            return QImage();
        }
    } else {
        qWarning() << "Invalid number of channels" << channels;
        return QImage();
    }

    const int expectedBytes = bytesPerLine * height;
    if (expectedBytes != data.size()) {
        qWarning() << "Invalid amount of pixels" << expectedBytes << data.size();
        return QImage();
    }


    return QImage(
            reinterpret_cast<uchar*>(data.data()),
            width,
            height,
            bytesPerLine,
            hasAlpha ? QImage::Format_ARGB32 : QImage::Format_RGB32
            )
        .copy();
}

quint32 Manager::Notify(const QString &name, const quint32 replacesId, const QString &appIconName, const QString &summary, const QString &body, const QStringList &actions, const QVariantMap &hints, const int timeout)
{
    qDebug() << "name" << name;
    qDebug() << "replaces" << replacesId;
    qDebug() << "appicon:" << appIconName;
    qDebug() << "summary" << summary;
    qDebug() << "Body" << body;
    qDebug() << "Actions" << actions;
    qDebug() << "hints"<< hints;
    qDebug() << "timeout" << timeout;

    if (m_unmuteTimer->isActive()) {
        qDebug() << "Notifications muted";
        m_unmuteIcon->setIcon(QPixmap(":/muted-active.png"));
        return m_lastId++;
    }

    QString icon;
    if (hints.contains("image_path")) {
        icon = hints["image_path"].toString();
    }
    if (icon.isEmpty() && hints.contains("image-path")) {
        icon = hints["image_path"].toString();
    }
    if (icon.isEmpty()) {
        icon = appIconName;
    }


    Widget *widget = new Widget;
    connect(this, &QObject::destroyed, widget, &QWidget::deleteLater);
    widget->setAppName(name);
    widget->setSummary(summary);
    widget->setBody(body);
    if (!icon.isEmpty()) {
        widget->setAppIcon(icon);
    } else {
        qWarning() << "Got deprecated image data";

        widget->setAppIcon(decodeIcon(qvariant_cast<QDBusArgument>(hints["icon_data"])));
    }
    widget->show();

    for (int i=0; i<actions.length() - 1; i++) {
        if (actions[i] != "default") {
            continue;
        }

        widget->setNotificationId(m_lastId);
        widget->setDefaultAction(actions[i ]);
        connect(widget, &Widget::actionClicked, this, &Manager::ActionInvoked);
        break;
    }

    connect(widget, &Widget::muteRequested, this, &Manager::onMuted, Qt::QueuedConnection);
    connect(widget, &Widget::notificationClosed, this, &Manager::NotificationClose, Qt::QueuedConnection);
    connect(this, &Manager::NotificationClose, widget, &Widget::onCloseRequested, Qt::QueuedConnection);

    return m_lastId++;
}

QStringList Manager::GetCapabilities()
{
    qDebug() << "Requested capabilities";
    return {
        "action-icons",
        "actions",
        "body",
        "body-hyperlinks",
        "body-images",
        "body-markup",
        "icon-multi",
        "persistence",
        "sound"
    };
}

void Manager::CloseNotification(quint32 id)
{
    qDebug() << "Asked to close" << id;
}

void Manager::onMuted(const int minutes)
{
    m_unmuteIcon->setIcon(QPixmap(":/muted.png"));
    m_unmuteIcon->show();

    m_unmuteTimer->start(minutes * 60 * 1000);
}

void Manager::onUnmute()
{
    m_unmuteIcon->hide();
    m_unmuteTimer->stop();
}
