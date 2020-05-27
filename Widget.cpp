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
    s_visibleNotifications++;
//    static int num = 0;
//    setObjectName("Popup" + QString::number(num++));
    setStyleSheet("QWidget {\n"
                "    background-color: rgba(0, 0, 0, 192);"//, 192);\n"
                "    color: white;\n"
//                "    selection-background-color: white;\n"
                "}\n");
//    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint | Qt::WindowDoesNotAcceptFocus | Qt::Tool);

//    setWindowFlag(Qt::WindowStaysOnBottomHint);
//    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    setWindowOpacity(1);

    QHBoxLayout *appLayout = new QHBoxLayout;
    appLayout->setMargin(0);
    appLayout->setSpacing(0);
    mainLayout->addLayout(appLayout);

    m_appIcon = new ClickableIcon;
    m_appIcon->setMaximumHeight(60);
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

void Widget::setBody(const QString &body)
{
    m_body->setVisible(true); // We have to set this as visible first, and then hide, to force it to load the resources. They are deleted after the dbus call returns
    m_body->setHtml(body);
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
    QUrl iconUrl(iconPath);
    if (!iconUrl.isLocalFile()) {
        iconUrl = QUrl::fromLocalFile(iconPath);
    }
    QImage icon(iconUrl.toLocalFile());
    if (icon.isNull()) {
        qWarning() << "Invalid icon path" << iconPath;
        return;
    }
    icon = icon.scaled(60, 60, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    m_appIcon->setPixmap(QPixmap::fromImage(icon));
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

    move(screenGeometry.width() - width() - 10, screenGeometry.height() - height() * s_visibleNotifications - 30);
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
    if (type != QTextDocument::ImageResource) {
        qWarning() << "Skipping type" << type;
        return QVariant();
    }

    if (!name.isLocalFile()) {
        qWarning() << "Refusing non-local resource" << name;
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
