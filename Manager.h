#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>

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
//                                 const QMap<QString, QString> &hints,
                                 const int timeout
                                 );

    Q_SCRIPTABLE QStringList GetCapabilities();

signals:
    Q_SCRIPTABLE void ActionInvoked(const quint32 id, const QString &action_key);

private:
    int m_lastId = 0;
};

#endif // MANAGER_H
