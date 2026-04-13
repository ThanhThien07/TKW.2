#include "mainwindow.h"
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), server(nullptr), socket(nullptr), isServerMode(false)
{
    setupUI();
    setWindowTitle("Chat 2 người - Messenger Style");
    resize(500, 600);
    updateStatus("🔌 Chưa kết nối");
    setMessageInputEnabled(false);
}

MainWindow::~MainWindow()
{
    disconnectCurrent();
    if (server) server->close();
}

void MainWindow::setupUI()
{
    centralWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centralWidget);

    // Hàng nút chế độ
    topLayout = new QHBoxLayout();
    btnServer = new QPushButton("Tạo phòng (Server)");
    btnClient = new QPushButton("Kết nối (Client)");
    topLayout->addWidget(btnServer);
    topLayout->addWidget(btnClient);
    mainLayout->addLayout(topLayout);

    // Khung kết nối (ẩn ban đầu)
    connectWidget = new QWidget();
    connectLayout = new QHBoxLayout(connectWidget);
    ipEdit = new QLineEdit();
    ipEdit->setPlaceholderText("Nhập IP của server...");
    btnConnect = new QPushButton("Kết nối");
    connectLayout->addWidget(ipEdit);
    connectLayout->addWidget(btnConnect);
    connectWidget->setVisible(false);
    mainLayout->addWidget(connectWidget);

    // Trạng thái
    statusLabel = new QLabel();
    mainLayout->addWidget(statusLabel);

    // Khung chat
    chatDisplay = new QTextEdit();
    chatDisplay->setReadOnly(true);
    mainLayout->addWidget(chatDisplay);

    // Khung nhập tin
    QHBoxLayout *inputLayout = new QHBoxLayout();
    msgEdit = new QLineEdit();
    msgEdit->setPlaceholderText("Nhập tin nhắn...");
    btnSend = new QPushButton("Gửi");
    inputLayout->addWidget(msgEdit);
    inputLayout->addWidget(btnSend);
    mainLayout->addLayout(inputLayout);

    setCentralWidget(centralWidget);

    // Kết nối tín hiệu
    connect(btnServer, &QPushButton::clicked, this, &MainWindow::onStartServer);
    connect(btnClient, &QPushButton::clicked, this, &MainWindow::onConnectClient);
    connect(btnSend, &QPushButton::clicked, this, &MainWindow::onSendMessage);
    connect(msgEdit, &QLineEdit::returnPressed, this, &MainWindow::onSendMessage);
    connect(btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClient);
}

void MainWindow::onStartServer()
{
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        disconnectCurrent();
    }
    if (server) {
        server->close();
        delete server;
        server = nullptr;
    }

    server = new QTcpServer(this);
    if (!server->listen(QHostAddress::Any, 12345)) {
        updateStatus("❌ Không thể mở cổng 12345. Hãy thử lại.", true);
        delete server;
        server = nullptr;
        return;
    }

    isServerMode = true;
    connect(server, &QTcpServer::newConnection, this, &MainWindow::newConnection);
    updateStatus("📡 Server đang chạy, chờ kết nối...");
    btnServer->setEnabled(false);
    btnClient->setEnabled(false);
    connectWidget->setVisible(false);
    setMessageInputEnabled(false);
}

void MainWindow::onConnectClient()
{
    // Nếu đang ở chế độ server, hủy server
    if (server) {
        server->close();
        delete server;
        server = nullptr;
    }
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        disconnectCurrent();
    }

    // Hiển thị khung nhập IP nếu đang ẩn
    if (!connectWidget->isVisible()) {
        connectWidget->setVisible(true);
        btnServer->setEnabled(false);
        btnClient->setEnabled(false);
        return;
    }

    // Thực hiện kết nối
    QString ip = ipEdit->text().trimmed();
    if (ip.isEmpty()) {
        QMessageBox::warning(this, "Thiếu IP", "Vui lòng nhập địa chỉ IP của server.");
        return;
    }

    updateStatus("⏳ Đang kết nối...");
    QTcpSocket *newSocket = new QTcpSocket(this);
    connect(newSocket, &QTcpSocket::connected, this, [=]() {
        socket = newSocket;
        isServerMode = false;
        updateStatus("✅ Đã kết nối đến " + ip);
        setMessageInputEnabled(true);
        appendMessage("other", "Đã kết nối! Bắt đầu chat.");
        connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readMessage);
        connect(socket, &QTcpSocket::disconnected, this, &MainWindow::socketDisconnected);
        connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::socketError);
        // Ẩn khung nhập IP sau khi kết nối thành công
        connectWidget->setVisible(false);
        btnServer->setEnabled(true);
        btnClient->setEnabled(true);
    });
    connect(newSocket, &QTcpSocket::errorOccurred, this, [=](QAbstractSocket::SocketError err) {
        updateStatus("❌ Lỗi kết nối: " + newSocket->errorString(), true);
        newSocket->deleteLater();
        btnServer->setEnabled(true);
        btnClient->setEnabled(true);
        connectWidget->setVisible(false); // cho phép thử lại bằng cách bấm Client lần nữa
    });
    newSocket->connectToHost(ip, 12345);
}

void MainWindow::newConnection()
{
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        socket->close();
        delete socket;
    }
    socket = server->nextPendingConnection();
    if (!socket) return;

    updateStatus("✅ Đã có người kết nối!");
    setMessageInputEnabled(true);
    appendMessage("other", "Đã kết nối! Bắt đầu chat.");
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readMessage);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::socketDisconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::socketError);
    // Ẩn nút chọn chế độ, không cho phép tạo server mới khi đang chat
    btnServer->setEnabled(false);
    btnClient->setEnabled(false);
    connectWidget->setVisible(false);
}

void MainWindow::readMessage()
{
    QTcpSocket *sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock) return;
    QByteArray data = sock->readAll();
    if (!data.isEmpty()) {
        appendMessage("other", QString::fromUtf8(data));
    }
}

void MainWindow::onSendMessage()
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        updateStatus("⚠️ Chưa kết nối với ai cả!", true);
        return;
    }
    QString msg = msgEdit->text().trimmed();
    if (msg.isEmpty()) return;

    socket->write(msg.toUtf8());
    socket->flush();
    appendMessage("me", msg);
    msgEdit->clear();
}

void MainWindow::appendMessage(const QString &sender, const QString &msg)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm");
    QString displayText;
    if (sender == "me")
        displayText = QString("Bạn (%1):\n  %2\n\n").arg(timestamp, msg);
    else
        displayText = QString("Đối phương (%1):\n  %2\n\n").arg(timestamp, msg);

    chatDisplay->moveCursor(QTextCursor::End);
    chatDisplay->insertPlainText(displayText);
    // Cuộn xuống cuối
    QScrollBar *bar = chatDisplay->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void MainWindow::updateStatus(const QString &msg, bool isError)
{
    statusLabel->setText(msg);
    if (isError) {
        statusLabel->setStyleSheet("color: red;");
    } else {
        statusLabel->setStyleSheet("");
    }
}

void MainWindow::setMessageInputEnabled(bool enabled)
{
    msgEdit->setEnabled(enabled);
    btnSend->setEnabled(enabled);
    if (enabled) msgEdit->setFocus();
}

void MainWindow::socketDisconnected()
{
    updateStatus("🔌 Đã ngắt kết nối");
    setMessageInputEnabled(false);
    disconnectCurrent();
    // Cho phép chọn lại chế độ
    btnServer->setEnabled(true);
    btnClient->setEnabled(true);
}

void MainWindow::socketError(QAbstractSocket::SocketError)
{
    QTcpSocket *sock = qobject_cast<QTcpSocket*>(sender());
    if (sock) {
        updateStatus("❌ Lỗi socket: " + sock->errorString(), true);
    }
    socketDisconnected();
}

void MainWindow::disconnectCurrent()
{
    if (socket) {
        socket->disconnect(); // ngắt tín hiệu
        if (socket->state() == QAbstractSocket::ConnectedState)
            socket->disconnectFromHost();
        socket->deleteLater();
        socket = nullptr;
    }
    if (server) {
        server->close();
        delete server;
        server = nullptr;
    }
}