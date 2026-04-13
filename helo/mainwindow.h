#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartServer();
    void onConnectClient();
    void onSendMessage();
    void newConnection();
    void readMessage();
    void socketDisconnected();
    void socketError(QAbstractSocket::SocketError);
    void updateStatus(const QString &msg, bool isError = false);

private:
    void setupUI();
    void appendMessage(const QString &sender, const QString &msg);
    void setMessageInputEnabled(bool enabled);
    void disconnectCurrent();

    QWidget *centralWidget;
    QVBoxLayout *mainLayout;

    // Top buttons
    QHBoxLayout *topLayout;
    QPushButton *btnServer;
    QPushButton *btnClient;

    // Connect frame (client only)
    QWidget *connectWidget;
    QHBoxLayout *connectLayout;
    QLineEdit *ipEdit;
    QPushButton *btnConnect;

    QLabel *statusLabel;

    QTextEdit *chatDisplay;
    QLineEdit *msgEdit;
    QPushButton *btnSend;

    QTcpServer *server;
    QTcpSocket *socket;
    bool isServerMode;
};

#endif // MAINWINDOW_H