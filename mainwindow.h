#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"
#include <QTranslator>

#include <QtNetwork>
#include <QDataStream>
#include <QTextStream>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    ~MainWindow();


    void insertcombox1(QSet<QString> arr);
    void insertcombox2(QSet<QString> arr);

    void create_file();
    void insert_file(QSet<QString> arr);

    void ReceiveTextEdit_recvdata(QByteArray datagram);
    void data_head_time();//处理头信息 时间的部分
    void Ipv6_to_Ipv4(); //将接收到的ipv6格式的ipv4地址 转成 真正的ipv4格式 QHostAddress("::ffff:169.254.109.105%23")=>QHostAddress("169.254.109.105")

private slots:
    void on_pBtnNetCnnt_clicked(bool checked);//开启按钮的槽函数
    void onComboxSelect(int index);//改变文本的名字
    void tEditSendText_send_state();    //判断输出框的状态
    void str_to_ASCII();
    void str_to_HEX();
    void select_cb_Ip_port_addr();
    void udpDataReceived();
    void onNewConnection();
    void tcpClientDataReceived();
    void readyRead_slot();
    void clientDisconnected();
    void on_pBtnSendData_clicked();     //当 发送按钮  按下，执行函数

    void on_pBtnResetCnt_clicked();

private:
    Ui::MainWindow *ui;
    unsigned int sndDataCnt;
    unsigned int rcvDataCnt;
    QHostAddress src_ip;    //源ip地址
    quint16 src_port;       //源端口

    QHostAddress dst_ip;     //目的ip
    quint16 dst_port;       //目的端口

    QString dst_ip_port;    //用于拼接接收到的目的地址和ip

    QString data_head;      //接收或发送数据的 头部分  的组装

    QString hexStr;          //转换时，用于存储hexstr
    QString asciiStr;          //转换时，用于存储 asciiStr

    QTextCharFormat format; //设置颜色

    QByteArray datagram;    //  接收/发送 要组装/解开 包的模板



    QString receivedText;   // 接收到的数据
    QString sendText;       // 发送出去的数据

    int send_state;//默认是ascii
    QRegExp *regex_ASCII;//定义ascii的规则
    QRegExp *regex_HEX;//定义hex的规则
    QActionGroup *group;//对字符集做一个组，方便互斥

    QTextCodec* Code_mask_type; //utf-8 / GBK
    QByteArray byteArray;// 接收转成byte array 的数据.将包数据，分割拆开，便于转码


    QSet<QString> ip_arr;//本地初始化时，将收集到的可用的ip地址存进去


    QSet<QString> ip_port_arr;//将连接成功的目的地址和目的端口的组合放进去，并且存在了文本中
    QString appDir;//app的路径
    QString filePath;//app的路径+自己的路径
    QFile file; //用上面的文件路径，创建文件，存储ip_port_arr的内容
    //-----UDP-----
    QUdpSocket *udpSocket;          //udp的套接字
    QTcpSocket *tcpSocket;          //tcp客户端的套接字
    QTcpServer *tcpServer_socket;   //tcp服务端的套接字

    //定义客户端指针链表容器
    QList<QTcpSocket *> clientList;//tcp server需要存客户端的socket
};
#endif // MAINWINDOW_H
