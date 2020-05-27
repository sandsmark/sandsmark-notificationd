#ifndef WIDGET_H
#define WIDGET_H

#include <QDialog>
#include <QTextBrowser>
#include <QLabel>

class QLabel;
class QTextBrowser;
class QTimer;

class BodyWidget : public QTextBrowser
{
    Q_OBJECT

protected:
    QVariant loadResource(int type, const QUrl &name) override;
};

class ClickableIcon : public QLabel
{
    Q_OBJECT

public:
    void setClickAction(const QString &action) { m_action = action; }
    void setNotificationId(const int id) { m_id = id; }

signals:
    void clicked(const int id, const QString &action);

protected:
    void mousePressEvent(QMouseEvent *event) override;
private:
    QString m_action;
    int m_id;
};

class Widget : public QDialog
{
    Q_OBJECT

public:
    Widget();
    ~Widget();

    void setTitle(const QString &title);
    void setSummary(const QString &summary);
    void setBody(const QString &body);

    void setNotificationId(const int id) { m_appIcon->setNotificationId(id); m_id = id; }
    void setDefaultAction(const QString &action);

    void setAppName(const QString &name);
    void setAppIcon(const QString &iconPath);
    void setAppIcon(const QImage &icon);

    void setTimeout(int timeout);

public slots:
    void onCloseRequested(const int id);

signals:
    void actionClicked(const int id, const QString &action);
    void notificationClosed(const int id, const int reason);
    void muteRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onUrlClicked(const QUrl &url);

private:
    QLabel *m_summary;
    ClickableIcon *m_appIcon;
    QLabel *m_appName;
    QTimer *m_dismissTimer;
    BodyWidget *m_body;
    int m_id;

    static int s_visibleNotifications;
};
#endif // WIDGET_H
