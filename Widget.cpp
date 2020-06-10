#include "Widget.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QTextBrowser>
#include <QPushButton>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QScreen>
#include <QDesktopServices>

int Widget::s_visibleNotifications = 0;

Widget::Widget()
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint | Qt::WindowDoesNotAcceptFocus | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    if (s_visibleNotifications > 10) {
        qWarning() << "Too many visible notifications already";
        close();
    }
    m_index = ++s_visibleNotifications;
    setStyleSheet("QWidget {\n"
                  "    background-color: rgba(0, 0, 0, 192);\n"
                  "    color: white;\n"
                  "}\n");


    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    setWindowOpacity(0.5);

    QHBoxLayout *appLayout = new QHBoxLayout;
    appLayout->setContentsMargins(0, 0, 0, 0);
    appLayout->setSpacing(0);
    mainLayout->addLayout(appLayout);

    m_appIcon = new ClickableIcon;
    m_appIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    appLayout->addWidget(m_appIcon);

    m_summary = new QLabel;
    appLayout->addWidget(m_summary);

    QWidget *appStretch = new QWidget;
    appStretch->setMinimumSize(0, 0);
    appStretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //appStretch->setFocusPolicy(Qt::NoFocus);
    appLayout->addWidget(appStretch);

    m_appName = new QLabel;
    QFont appFont = m_appName->font();
    appFont.setBold(true);
    m_appName->setFont(appFont);
    appLayout->addWidget(m_appName);

    QVBoxLayout *contentLayout = new QVBoxLayout;
    contentLayout->setMargin(0);
    contentLayout->setSpacing(0);
    mainLayout->addLayout(contentLayout);

    QPushButton *showBodyButton = new QPushButton("Details..");
    contentLayout->addWidget(showBodyButton);

    QPushButton *muteButton = new QPushButton("Mute 5 minutes");
    contentLayout->addWidget(muteButton);

    m_body = new BodyWidget;
    m_body->setMinimumSize(600, 1);
    m_body->setFocusPolicy(Qt::NoFocus);
    m_body->setVisible(false); // We have to set this as visible first, and then hide, to force it to load the resources. They are deleted after the dbus call returns
    m_body->setOpenLinks(false);
    contentLayout->addWidget(m_body);

    QWidget *contentStretch = new QWidget;
    contentStretch->setMinimumSize(1, 1);
    contentStretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    contentStretch->setFocusPolicy(Qt::NoFocus);
    contentLayout->addWidget(contentStretch);

    connect(showBodyButton, &QPushButton::clicked, this, [=]() {
        contentStretch->hide();
        this->m_body->show();
        adjustSize();
        resize(800, height());
    });
    connect(showBodyButton, &QPushButton::clicked, showBodyButton, &QPushButton::hide);
    connect(muteButton, &QPushButton::clicked, this, &Widget::muteRequested);
    connect(muteButton, &QPushButton::clicked, this, &Widget::close);
    connect(m_body, &QTextBrowser::anchorClicked, this, [](const QUrl &url) { QDesktopServices::openUrl(url); });
    connect(m_appIcon, &ClickableIcon::clicked, this, &Widget::actionClicked);

    m_dismissTimer = new QTimer(this);
    m_dismissTimer->setInterval(10000);
    m_dismissTimer->setSingleShot(true);
    connect(m_dismissTimer, &QTimer::timeout, this, &QWidget::close);
    connect(showBodyButton, &QPushButton::clicked, m_dismissTimer, &QTimer::stop);
    m_dismissTimer->start();

    resize(600, 150);

    setVisible(true);
    adjustSize();
    clearFocus();
}

Widget::~Widget()
{
    s_visibleNotifications--;

    qDebug() << "Widget destroyed";
    emit notificationClosed(m_id, 2); // always fake that the user clicked it away
}

void Widget::setSummary(const QString &summary)
{
    m_summary->setText(m_summary->fontMetrics().elidedText(summary, Qt::ElideRight, 500));
}

void Widget::setBody(QString body)
{
    m_body->setVisible(true); // We have to set this as visible first, and then hide, to force it to load the resources. They are deleted after the dbus call returns
    m_body->setHtml(body.replace("\n", "<br/>"));
    m_body->setVisible(false); // We have to set this as visible first, and then hide, to force it to load the resources. They are deleted after the dbus call returns
}

void Widget::setDefaultAction(const QString &action)
{
    m_appIcon->setClickAction(action);
    m_appIcon->setCursor(Qt::PointingHandCursor);
}

void Widget::setAppName(const QString &name)
{
    m_appName->setText(name);
}

void Widget::setAppIcon(const QString &iconPath)
{
    QImage icon;

    if (!iconPath.isEmpty()) {
        QUrl iconUrl(iconPath);
        if (!iconUrl.isLocalFile()) {
            iconUrl = QUrl::fromLocalFile(iconPath);
        }
        icon = QImage(iconUrl.toLocalFile());
    } else {
        qDebug() << "Tried to set empty icon";
    }
    setAppIcon(icon);
}

void Widget::setAppIcon(const QImage &icon)
{
    if (icon.isNull()) {
        m_appIcon->setPixmap(QPixmap(":/annoying.png"));
        qWarning() << "Invalid icon";
        return;
    }

    m_appIcon->setPixmap(QPixmap::fromImage(
            icon.scaled(64, 64, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)
        ));
}

void Widget::setTimeout(int timeout)
{
    if (timeout <= 0) {
        timeout = 10;
    }

    m_dismissTimer->start(timeout * 1000);
}

void Widget::mousePressEvent(QMouseEvent *)
{
    close();
}

void Widget::resizeEvent(QResizeEvent *)
{
    const QRect screenGeometry = screen()->geometry();

    move(screenGeometry.width() - width() - 10, screenGeometry.height() - height() * m_index - 30);
}

void Widget::enterEvent(QEvent *)
{
    setWindowOpacity(1);
    m_timeLeft = m_dismissTimer->remainingTime();
    m_dismissTimer->stop();
}

void Widget::leaveEvent(QEvent *)
{
    if (m_body->isVisible()) {
        return;
    }
    setWindowOpacity(0.4); // slightly less visible on purpose, yes thank you
    if (m_timeLeft > 0) {
        m_dismissTimer->start(m_timeLeft);
    }
}

void Widget::onUrlClicked(const QUrl &url)
{
    qDebug() << "Clicked" << url;
}

void Widget::onCloseRequested(const int id)
{
    if (id == m_id) {
        qDebug() << "Requested that we close";
        close();
    }
}


QVariant BodyWidget::loadResource(int type, const QUrl &name)
{
    if (!name.isLocalFile()) {
        qWarning() << "Refusing non-local resource" << name;
        return QVariant();
    }

    if (type == QTextDocument::ImageResource) {
        return QImage(name.toLocalFile());
    }

    qDebug() << "Requested" << name << "of type" << type;
    if (type != QTextDocument::ImageResource) {
        qWarning() << "Refusing type" << type << "for now";
        return QVariant();
    }

    QFile file(name.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "FAiled to open" << file.fileName() << file.errorString();
        return QVariant();
    }
    return file.readAll();
}

void ClickableIcon::mousePressEvent(QMouseEvent *)
{
    if (m_action.isEmpty()) {
        return;
    }
    qDebug() << "Clicked" << m_id << m_action;

    emit clicked(m_id, m_action);
}
