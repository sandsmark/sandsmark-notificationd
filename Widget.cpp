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
#include <QIcon>

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
                  "    background-color: rgba(0, 0, 0, 200);\n"
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

    m_appIcon = new ClickableLabel;
    m_appIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    appLayout->addWidget(m_appIcon);

    m_summary = new ClickableLabel;
    appLayout->addWidget(m_summary);

    QWidget *appStretch = new QWidget;
    appStretch->setMinimumSize(0, 0);
    appStretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    appLayout->addWidget(appStretch);

    m_appName = new ClickableLabel;
    QFont appFont = m_appName->font();
    appFont.setBold(true);
    m_appName->setFont(appFont);
    appLayout->addWidget(m_appName);

    QVBoxLayout *contentLayout = new QVBoxLayout;
    contentLayout->setMargin(0);
    contentLayout->setSpacing(0);
    mainLayout->addLayout(contentLayout);

    m_body = new BodyWidget;
    m_body->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_body->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_body->setFixedSize(600, 50);
    m_body->setFocusPolicy(Qt::NoFocus);
    m_body->setOpenLinks(false);
    m_body->document()->setDefaultStyleSheet(
            "a {\n"
            "  color: #aaf; \n"
            "}\n"
            );
    contentLayout->addWidget(m_body);


    QHBoxLayout *muteLayout = new QHBoxLayout;
    QPushButton *mute5Button = new QPushButton(QIcon::fromTheme("media-playback-pause-symbolic"), "Mute 5 minutes");
    QPushButton *mute30Button = new QPushButton(QIcon::fromTheme("media-playback-pause-symbolic"), "Mute 30 minutes");
    muteLayout->addWidget(mute5Button);
    muteLayout->addWidget(mute30Button);
    contentLayout->addLayout(muteLayout);

    QWidget *contentStretch = new QWidget;
    contentStretch->setMinimumSize(1, 1);
    contentStretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    contentStretch->setFocusPolicy(Qt::NoFocus);
    contentLayout->addWidget(contentStretch);

    connect(mute5Button, &QPushButton::clicked, this, [this]() {
        emit muteRequested(5);
    });
    connect(mute30Button, &QPushButton::clicked, this, [this]() {
        emit muteRequested(30);
    });

    connect(this, &Widget::muteRequested, this, &Widget::close);

    connect(m_body, &QTextBrowser::anchorClicked, this, [](const QUrl &url) { QDesktopServices::openUrl(url); });
    connect(m_appIcon, &ClickableLabel::clicked, this, &Widget::actionClicked);
    connect(m_summary, &ClickableLabel::clicked, this, &Widget::actionClicked);
    connect(m_appName, &ClickableLabel::clicked, this, &Widget::actionClicked);

    m_dismissTimer = new QTimer(this);
    m_dismissTimer->setInterval(10000);
    m_dismissTimer->setSingleShot(true);
    connect(m_dismissTimer, &QTimer::timeout, this, &QWidget::close);
    m_dismissTimer->start();

    resize(600, 50);

    setVisible(true);
    adjustSize();
    clearFocus();
}

Widget::~Widget()
{
    s_visibleNotifications--;

    emit notificationClosed(m_id, 2); // always fake that the user clicked it away
}

void Widget::setSummary(const QString &summary)
{
    m_summary->setText(m_summary->fontMetrics().elidedText(summary, Qt::ElideRight, 500));
}

void Widget::setBody(QString body)
{
    m_body->setHtml(body.replace("\n", "<br/>"));
}

void Widget::setDefaultAction(const QString &action)
{
    m_appIcon->setClickAction(action);
    m_appIcon->setCursor(Qt::PointingHandCursor);

    m_summary->setClickAction(action);
    m_summary->setCursor(Qt::PointingHandCursor);

    m_appName->setClickAction(action);
    m_appName->setCursor(Qt::PointingHandCursor);
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

        if (icon.isNull()) {
            icon = QIcon::fromTheme(iconPath).pixmap(64, 64).toImage();
        }
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
            icon.scaled(32, 32, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)
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
    setWindowOpacity(0.4); // slightly less visible on purpose, yes thank you
    m_dismissTimer->start(qMax(m_timeLeft, 0));
}

void Widget::onUrlClicked(const QUrl &url)
{
    qDebug() << "Clicked" << url;
}

void Widget::onCloseRequested(const int id)
{
    if (id == m_id) {
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

void ClickableLabel::mousePressEvent(QMouseEvent *)
{
    if (m_action.isEmpty()) {
        return;
    }

    emit clicked(m_id, m_action);
}
