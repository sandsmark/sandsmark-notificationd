#include "Manager.h"
#include "Widget.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>

Manager::Manager(QObject *parent) : QObject(parent)
{
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

    Widget *widget = new Widget;
    connect(this, &QObject::destroyed, widget, &QWidget::deleteLater);
    widget->setAppIcon(appIconName);
    widget->setAppName(name);
    widget->setSummary(summary);
    widget->setBody(body);
    widget->show();

    for (int i=0; i<actions.length() - 1; i++) {
        if (actions[i] != "default") {
            continue;
        }

        widget->setNotificationId(m_lastId);
        widget->setDefaultAction(actions[i + 1]);
        connect(widget, &Widget::actionClicked, this, &Manager::ActionInvoked);
        break;
    }

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
