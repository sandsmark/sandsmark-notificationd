#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>

class QTimer;

class QSystemTrayIcon;

class Manager : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")

public:
    explicit Manager(QObject *parent = nullptr);

    bool init();

public slots:
    Q_SCRIPTABLE quint32 Notify(const QString &name,
                                 const quint32 replacesId,
                                 const QString &appIconName,
                                 const QString &summary,
                                 const QString &body,
                                 const QStringList &actions,
                                 const QVariantMap &hints,
                                 const int timeout
                                 );

    Q_SCRIPTABLE QStringList GetCapabilities();
    Q_SCRIPTABLE void CloseNotification(quint32 id);
    Q_SCRIPTABLE QString GetServerInformation(QString &vendor, QString &version, QString &specVersion) {
        specVersion = "garbage";
        version = "utter";
        vendor = "is";
        return "libnotify";
    }

signals:
    Q_SCRIPTABLE void ActionInvoked(const quint32 id, const QString &action_key);
    Q_SCRIPTABLE void NotificationClose(const quint32 id, const quint32 reason);

    void closeRequested(int id);

private slots:
    void onMuted();
    void onUnmute();

private:
    int m_lastId = 0;
    QSystemTrayIcon *m_unmuteIcon = nullptr;
    QTimer *m_unmuteTimer = nullptr;
};

#endif // MANAGER_H
