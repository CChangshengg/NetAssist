#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTranslator>
#include "define.h"
#include <QMessageBox>
#include <stdlib.h>
#include <QFileDialog>
#include <QFile>
#include <QString>
#include <QTimer>
#include <QInputDialog>
#include <QDateTime>
#include<wpcapi.h>
#include<QDebug>
#include <QByteArray> //QByteArray类有一个非常方便的toHex方法
#include<QScrollBar>
#include<QPalette>

#include <QString>
#include <QRegularExpression>
#include <QFile>
#include <QTextStream>
#include <QtNetwork/QHostAddress>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("网络调试助手");
    setWindowIcon(QIcon(":/qrc_/NATIcon.png"));
    this->resize(800,300);
    ui->widget->setVisible(false); //默认情况下，udp的目的地址和端口的组件不显示出来，连接成功才显示出来

    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();//找到一个合适的源ip
    for (const QHostAddress& address : ipAddressesList) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol) {
            ui->cb_IpAddr->addItem(address.toString());
            ip_arr.insert(address.toString());
        }
    //           qDebug() << "本机IP地址: " << address.toString();
    }
    create_file();
    //给输入框设置规则 Qtextedit没有setValidator,只有Qlineedit有，因此
    //使用信号与槽函数

    send_state=1;

    Code_mask_type=QTextCodec::codecForName("GBK");

    connect(ui->cBoxNetType, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboxSelect(int)));//选择类型不同，显示不同的样式


    connect(ui->tEditSendText,&QTextEdit::textChanged,this,&MainWindow::tEditSendText_send_state);//输入文本框变动，检测输入格式
    connect(ui->RB_Ascii_Send, &QRadioButton::clicked, this, &MainWindow::str_to_ASCII); //发送设置的 ascii按钮 ，点击了就会触发函数
    connect(ui->RB_Hex_Send, &QRadioButton::clicked, this, &MainWindow::str_to_HEX);    //发送设置的 hex 按钮 ，点击了就会触发函数
    connect(ui->actionANSI_GBK,&QAction::triggered,this,[this](){Code_mask_type=QTextCodec::codecForName("GBK");});
    connect(ui->actionUTF_8,&QAction::triggered,this,[this](){Code_mask_type=QTextCodec::codecForName("UTF-8");});

    group=new QActionGroup(this);//给字符集做互斥
    group->addAction(ui->actionANSI_GBK);
    group->addAction(ui->actionUTF_8);
    group->setExclusive(true);

}

void MainWindow::select_cb_Ip_port_addr( ){
    QString curr_str=ui->cb_IpAddr->currentText();
//     curr_str.split(":").at(0);
    ui->cb_IpAddr->setCurrentText(curr_str.split(":").at(0));
    ui->lEditIpPort->setText(curr_str.split(":").at(1));
//    qDebug()<<curr_str<<index ;
}
void MainWindow::insertcombox1(QSet<QString> arr){
        ui->cb_IpAddr->clear();
        for (const QString& str : arr) {
            ui->cb_IpAddr->addItem(str);
        }
        if(ui->cBoxNetType->currentIndex()==TCP_CLIENT_MODE){//第一次的时候，需要手动设置，
//            ui->cb_IpAddr->set
            QString curr_str=ui->cb_IpAddr->currentText(); //取当前的控件ip和port

            ui->cb_IpAddr->setCurrentText(curr_str.split(":").at(0));  //ip控件赋值
            ui->lEditIpPort->setText(curr_str.split(":").at(1));    //port控件赋值
        }
}



void MainWindow::insertcombox2(QSet<QString> arr){
    ui->cBox_ip_port2->clear();
    for (const QString& str : arr) {
        ui->cBox_ip_port2->addItem(str);
    }
}
void MainWindow::insert_file(QSet<QString> arr){
     file.open(QIODevice::WriteOnly | QIODevice::Text|QIODevice::Truncate);
         for ( const QString& str : arr) {
            file.write(str.toUtf8());
            file.write("\n");
         }
}
void MainWindow::onComboxSelect(int index)
{
    //获取combobox当前内容
    switch(index){
        case 0://udp
            disconnect(ui->cb_IpAddr, SIGNAL(currentIndexChanged(int)), this, SLOT(select_cb_Ip_port_addr()));
            ui->cb_IpAddr->setEditable(false);
            insertcombox1(ip_arr);
            insertcombox2(ip_port_arr);
//            insert_file(ip_port_arr);//arr->file
            ui->label_IP->setText("（2）本地IP地址");
            ui->label_Port->setText("（3）本地端口");
            break;
        case 1://tcp_server
            disconnect(ui->cb_IpAddr, SIGNAL(currentIndexChanged(int)), this, SLOT(select_cb_Ip_port_addr()));
            ui->cb_IpAddr->setEditable(false);
            insertcombox1(ip_arr);
//            file->arr
//            file-控件
           //进来的ip_port ->arr
            //ip_port-> 控件
//            insertcombox2(ip_port_arr);// arr->控件
            ui->label_IP->setText("（2）本地IP地址");
            ui->label_Port->setText("（3）本地端口");
            break;
        case 2://tcp_client
//        qDebug()<<"2";
            ui->cb_IpAddr->setEditable(true);//  ip地址控件可以选择编辑内容
            if(!ip_port_arr.isEmpty()){
                insertcombox1(ip_port_arr);         //将地址插入控件内
                connect(ui->cb_IpAddr, SIGNAL(currentIndexChanged(int)), this, SLOT(select_cb_Ip_port_addr()));
            }
            ui->label_IP->setText("（2）远程IP地址");
            ui->label_Port->setText("（3）远程端口");
            break;
           }

}

//点击 on_pBtnNetCnnt 按钮，开启服务，入口函数
void MainWindow::on_pBtnNetCnnt_clicked(bool checked)
{
//    按钮按下去，不松开，一直是ture,checked是true
    sndDataCnt=0;//收集发送的字节数
    rcvDataCnt=0;//收集接收的字节数
    if(checked){
        src_ip.setAddress(ui->cb_IpAddr->currentText());
        src_port=ui->lEditIpPort->text().toInt(); //从 端口文本框中获取端口，设置到源端口上
//        qDebug()<<ui->cBoxNetType->currentText();
//      qDebug()<<checked<<endl; //输出true
        if(ui->cBoxNetType->currentIndex() == UDP_MODE){
        //创建UDP套接字
            udpSocket = new QUdpSocket(this);
            //接收数据，需要bind一个端口，这个端口得是有效端口，就是没有被占用的
//            bool result = udpSocket->bind(src_port);

            bool result = udpSocket->bind(src_ip,src_port);
            if(!result){
                ui->pBtnNetCnnt->setChecked(0);
                QMessageBox::information(this, tr("错误"), tr("UDP绑定端口失败!"));
                return;
            }else {//绑定成功
                ui->cBoxNetType->setEnabled(false);
                ui->cb_IpAddr->setEnabled(false);
                ui->lEditIpPort->setEnabled(false);
                connect(udpSocket, SIGNAL(readyRead()), this, SLOT(udpDataReceived()));//这个是udp接收数据的，一直阻塞，来数据触发udpDataReceived函数
                ui->CurState->setText(tr("建立UDP连接成功"));
                //成功就把远程的框展示出来
                ui->widget->setVisible(true);//dst_ip和port控件展示
            }
        }
        else if(ui->cBoxNetType->currentIndex() == TCP_SERVER_MODE){
            ui->widget->setVisible(true);
            ui->labelUdp->setText("客户端");

            src_ip.setAddress(ui->cb_IpAddr->currentText());
            src_port=ui->lEditIpPort->text().toInt();

            tcpServer_socket=new QTcpServer(this);
            if(!tcpServer_socket->listen(src_ip,src_port))
            {
                ui->pBtnNetCnnt->setChecked(0);
                QMessageBox::critical(this, "失败", "服务器启动失败");
                return;
            }else{
                ui->cBoxNetType->setEnabled(false);
                ui->cb_IpAddr->setEnabled(false);
                ui->lEditIpPort->setEnabled(false);
                ui->CurState->setText(tr("建立TCP服务器成功"));
            }
            //执行到这表明服务器启动成功，并对客户端连接进行监听，如果有客户端向服务器发来连接请求，那么该服务器就会自动发射一个newConnection信号
            //我们可以将信号连接到对应的槽函数中处理相关逻辑
            ui->cBox_ip_port2->clear();
            ui->cBox_ip_port2->addItem("All Connections (0)");
            ui->cBox_ip_port2->setCurrentIndex(0);

            connect(tcpServer_socket, &QTcpServer::newConnection, this, &MainWindow::onNewConnection);

        }else if(ui->cBoxNetType->currentIndex()==TCP_CLIENT_MODE){

            ui->cb_IpAddr->setEditable(true);//设置可编辑状态

            dst_ip.setAddress(ui->cb_IpAddr->currentText());
//            dst_ip.setAddress(ui->lEditIpAddr->text());
            dst_port=ui->lEditIpPort->text().toInt();

            tcpSocket = new QTcpSocket(this);
            connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(tcpClientDataReceived()));
            tcpSocket->connectToHost(dst_ip,dst_port);


            if(!tcpSocket->waitForConnected(2000)){//尝试连接失败
                ui->pBtnNetCnnt->setChecked(0);
                ui->CurState->setText(tr("建立TCP客户端失败"));
                return;
            }


//            qDebug()<<tcpSocket->localPort()<<tcpSocket->localAddress();
            src_ip=tcpSocket->localAddress();
            src_port=tcpSocket->localPort();
            data_head_time();
            data_head.append("The server is connected from local ");
            data_head.append(src_ip.toString());
            data_head.append(":");
            data_head.append(QString::number(src_port,10));
            data_head.append("\n\n");

            format.setForeground(QColor(109,109,109));
            ui->ReceiveTextEdit->setCurrentCharFormat(format);
            ui->ReceiveTextEdit->insertPlainText(data_head);

            ui->cBoxNetType->setEnabled(false);
            ui->cb_IpAddr->setEnabled(false);
            ui->lEditIpPort->setEnabled(false);

            ui->CurState->setText(tr("建立TCP客户端成功"));

        }
        ui->pBtnNetCnnt->setText(tr("断开网络"));   //设置 连接网络按钮 的字体
        ui->pBtnSendData->setEnabled(true);         //给 发送按钮 设置可以使用
    }else{//连接 主动 断开
//         qDebug()<<"false"<<endl;
        ui->cBoxNetType->setEnabled(true);
        ui->cb_IpAddr->setEnabled(true);
        ui->lEditIpPort->setEnabled(true);

        if(ui->cBoxNetType->currentIndex() == UDP_MODE) {//断开UDP链接
            udpSocket->close();
            delete udpSocket;
        } else if(ui->cBoxNetType->currentIndex() == TCP_SERVER_MODE) {//断开TCP服务器链接
            tcpServer_socket->close();
            delete tcpServer_socket;
        } else if(ui->cBoxNetType->currentIndex() == TCP_CLIENT_MODE) {//断开TCP客户端链接
            tcpSocket->close();
            delete tcpSocket;
        }

        insert_file(ip_port_arr);//arr->file
        ui->widget->setVisible(false);                  //断开连接，将 目的地址端口框 收回
        ui->pBtnNetCnnt->setText(tr("连接网络"));        //设置 连接网络按钮 的字体
        ui->pBtnSendData->setEnabled(false);            //给 发送按钮设置 不可以使用
        ui->CurState->setText(tr(""));                  //设置状态位空
    }

}


//当文本框change的时候，检测输入的内容是不是符合编码格式的
void MainWindow::tEditSendText_send_state(){
    QChar temp_char;
//        QString temp_str=ui->tEditSendText->toPlainText();

    if(ui->tEditSendText->toPlainText().length()>0){//判断长度大于0再取，因为输入中文的时候，会异常
        temp_char=ui->tEditSendText->toPlainText().right(1).at(0);//取最后一个字符判断
        }
    if(send_state==1){                  //判断输入的是不是ascii格式
//        qDebug()<<send_state<<"--"<<temp_char<<"--"<<temp_char.unicode();
        if ((48 <= temp_char.unicode() && temp_char.unicode() <= 57) ||
            (65 <= temp_char.unicode() && temp_char.unicode() <= 90) ||
            (97 <= temp_char.unicode() && temp_char.unicode() <= 122)||
            (0X4E00<=temp_char.unicode()&& temp_char.unicode()<=0X9FFF)||
             (  temp_char.unicode()==32||temp_char.unicode()==0)  ){

        }else if(temp_char.unicode()==10){//10是回车，也就是发送
            ui->tEditSendText->undo();
            if(ui->pBtnSendData->isEnabled()){
                on_pBtnSendData_clicked();
            }
//            qDebug()<<"发送ascii";

        }else { //非法输入
            ui->tEditSendText->undo();
            QMessageBox::warning(this, "警告", "输入内容需符合ASCII");
        }
    }
    if(send_state==2){            //判断输入的是不是hex格式
//        qDebug()<<send_state<<"--"<<temp_char<<"--"<<temp_char.unicode();
        if ((48 <= temp_char.unicode() && temp_char.unicode() <= 57) ||
            (65 <= temp_char.unicode() && temp_char.unicode() <= 69) ||
            (97 <= temp_char.unicode() && temp_char.unicode() <= 101)||
                temp_char.unicode()==32||temp_char.unicode()==0 ) {
            qDebug()<<temp_char.unicode();
            //48-57 <数字0-9>
//                65-69 < 大写字母 A-E >
//                97-101 < 小写字母 -e>
//                32 空格32 <space>
            //0 是null
            // 符合条件后的代码逻辑
        }else if(temp_char.unicode()==10){//10是回车，也就是发送
            ui->tEditSendText->undo();
            if(ui->pBtnSendData->isEnabled()){
                on_pBtnSendData_clicked();
            }
//            qDebug()<<"发送hex";
        }else{
            ui->tEditSendText->undo();
            QMessageBox::warning(this, "警告", "输入内容需符合十六进制格式");
        }
    }
}
MainWindow::~MainWindow()
{
    delete ui;
}
//----tcp data recvied
void MainWindow::tcpClientDataReceived(){
     while(tcpSocket->bytesAvailable() > 0){ // 读取缓冲区中的所有数据
            datagram="";
            datagram.resize(tcpSocket->bytesAvailable());
            tcpSocket->read(datagram.data(),datagram.size());
            dst_ip_port=ui->cb_IpAddr->currentText()+":"+ui->lEditIpPort->text();
            ReceiveTextEdit_recvdata(datagram);
     }
}

//----udp data recvied
void MainWindow::udpDataReceived()
{
    while(udpSocket->hasPendingDatagrams())
    {
        datagram="";
        datagram.resize(udpSocket->pendingDatagramSize());

        //接数据,目的ip 和 目的port
//        QHostAddress temp;
        udpSocket->readDatagram(datagram.data(), datagram.size(), &dst_ip, &dst_port);
//        dst_ip 的数据是"::ffff:169.254.109.105%23"
        //处理数据
//        qDebug()<<dst_ip;
        Ipv6_to_Ipv4();//将目的地址从ipv6格式转成ipv4格式

//      qDebug()<<src_ip<<"--"<<dst_ip;
//      qDebug()<<"123";
//      qDebug()<<src_port<<"--"<<dst_port;
//      qDebug()<<"456";

        dst_ip_port=dst_ip.toString()+":"+QString::number(dst_port,10);//拿到了对于的ip和port，并组合
//        qDebug()<<dst_ip<<dst_port;

        ip_port_arr.insert(dst_ip_port);//插入到组中  接收的dst_ip_port  =>   ip_port_arr

        //插入到控件内 ip_port   ->   控件
        bool inserted=false;    //有没有插入到控件中
        int i;                  //端口地址的索引
        for(i=0;i< ui->cBox_ip_port2->count();i++){
            if(ui->cBox_ip_port2->itemText(i)==dst_ip_port){//说明找到了
                inserted=true;
                ui->cBox_ip_port2->setCurrentIndex(i);
            }
        }
        if(!inserted){//说明没找到
             ui->cBox_ip_port2->addItem(dst_ip_port);
             ui->cBox_ip_port2->setCurrentIndex(i);
        }

        ReceiveTextEdit_recvdata(datagram);//处理数据
    }
}
void MainWindow::Ipv6_to_Ipv4(){
   if (dst_ip.protocol() == QAbstractSocket::IPv6Protocol){
       QString temp_ip=dst_ip.toString().mid(7,dst_ip.toString().length()-7-3);
       QHostAddress temp(temp_ip);
       dst_ip=temp;
   }
}
void MainWindow::ReceiveTextEdit_recvdata(QByteArray datagram){
        //检测字符集的设置
//        bool charset1=ui->RB_Ascii_Recv->isChecked();
//        qDebug()<<charset1<<":::"<<endl;



    ui->ReceiveTextEdit->moveCursor(QTextCursor::End);//移动光标到最后
    receivedText="";
    if(ui->cBox_hide_recv->isChecked()){
    //如果选隐藏，则不处理
    }else{//接头部和内容
        //加头部
        data_head_time();//组data_head的时间的部分
        if(ui->RB_Ascii_Recv->isChecked()){//检测,设置为字符串格式，也就是ascii码格式
            //组头部 收到的格式和信息
            data_head.append("RECV ASCII FROM ");
            data_head.append(dst_ip_port);
            data_head.append(" >\n");

            receivedText = QString::fromUtf8(datagram);//将拿到的数据转成string，组给data_head


        }else if(ui->RB_Hex_Recv->isChecked()){//检测,设置为hex格式，也就是ascii码格式
            //组头部 收到的格式和信息
            data_head.append("RECV HEX FROM ");
            data_head.append(dst_ip_port);
            data_head.append(" >\n");

            //处理接收到的数据,16进制格式
            QString compactHex =datagram.toHex().toUpper();//313233343536

            //加空格
            for (int i = 0; i < compactHex.length(); i += 2) {
                QString bytePair = compactHex.mid(i, 2);
                receivedText.append(bytePair);
                if (i < compactHex.length()-1) {
                    receivedText.append(" ");
                }
            }
        }

        rcvDataCnt += receivedText.remove(" ").size();
//        qDebug()<<"456"<<rcvDataCnt;
        ui->lEdit_RcvCnt->setText(QString::number(rcvDataCnt, 10));

        receivedText.append("\n");

        if(ui->cBox_huanhangMode->isChecked()){//检测，要多加一个换行
            receivedText.append("\n");
        }

        //将数据输出到 控件 ReceiveTextEdit 上

        //设置头部份的颜色  浅灰色
        format.setForeground(QColor(109,109,109));
        ui->ReceiveTextEdit->setCurrentCharFormat(format);
        ui->ReceiveTextEdit->insertPlainText(data_head);
        //设置主要内容的颜色 墨绿色
        format.setForeground(QColor(0, 128, 0));
//        format.setForeground(QColor(0, 0, 128));//蓝色
        ui->ReceiveTextEdit->setCurrentCharFormat(format);
        ui->ReceiveTextEdit->insertPlainText(receivedText);

        //将滚动条设置到最底部
        ui->ReceiveTextEdit->verticalScrollBar()->setSliderPosition(ui->ReceiveTextEdit->verticalScrollBar()->maximum());
    }
        data_head="";
        receivedText="";
}
void MainWindow::data_head_time(){
    //处理头部份
    data_head="";
    int year, month, day;
    QDateTime::currentDateTime().date().getDate(&year, &month, &day);//获取年月日

    QString date = QString::number(year, 10) % "-" % QString::number(month, 10) % "-" % QString::number(day, 10);
    //10是将 数字 转换成10进制
    data_head.append("[");
    data_head.append(date);
    data_head.append(" ");
    data_head.append(QDateTime::currentDateTime().time().toString());
    data_head.append("] # ");
//    ui->ReceiveTextEdit->insertPlainText(data_head);
//    data_head="";
//               qDebug() << send_data;

}

void MainWindow::on_pBtnSendData_clicked()//发送
{
    ui->ReceiveTextEdit->moveCursor(QTextCursor::End);//移动光标到最后
    sendText="";
    //检测输入框是不是空的
    if(ui->cBoxNetType->currentIndex()==UDP_MODE){//赋值即可，检测交给后面
        dst_ip_port=ui->cBox_ip_port2->currentText();
//        qDebug()<<dst_ip_port;
    }else if(ui->cBoxNetType->currentIndex()==TCP_CLIENT_MODE){
        dst_ip_port=ui->cb_IpAddr->currentText()+":"+ui->label_IP->text();
//        qDebug()<<dst_ip_port;
    }else if(ui->cBoxNetType->currentIndex()==TCP_SERVER_MODE){
        dst_ip_port=ui->cBox_ip_port2->currentText();
//        qDebug()<<dst_ip_port;
    }
    int bool_setaddress =dst_ip.setAddress(dst_ip_port.split(":").at(0));//解析地址是否正确

    if(ui->tEditSendText->toPlainText().size() == 0) {
            QMessageBox::information(this, tr("提示"), tr("发送区为空，请输入内容。"));
    }else if(ui->cBoxNetType->currentIndex()==UDP_MODE && !bool_setaddress){
        QMessageBox::information(this, tr("提示"), tr("无效的远程主机地址\n\n远程主机地址的格式 IP:PORT \n\n 例如 192.168.1.8:8080"));
    }else if(ui->cBoxNetType->currentIndex()==TCP_CLIENT_MODE && !bool_setaddress){
        QMessageBox::information(this,tr("提示"),tr("无效的远程IP地址\n\n格式 192.168.1.8"));
    }else if(ui->cBoxNetType->currentIndex()==TCP_SERVER_MODE && ui->cBox_ip_port2->currentIndex()!=0 &&!bool_setaddress){
        //1.是TCP_SERVER_MODE
//        2.是索引为0的情况下，过
//        3.索引不为0的情况下，检测地址,正确则过
        QMessageBox::information(this, tr("提示"), tr("无效的远程主机地址\n\n远程主机地址的格式 IP:PORT \n\n 例如 192.168.1.8:8080"));
    }else {//符合条件
        //组装头
        data_head_time();//组装时间
        if(ui->RB_Ascii_Send->isChecked()){//检测,设置为字符串格式，也就是ascii码格式
            //组头部 收到的格式和信息
            data_head.append("SEND ASCII TO ");
            data_head.append(dst_ip_port);
            data_head.append(" >\n");
        }
        if(ui->RB_Hex_Send->isChecked()){//检测,设置为字符串格式，也就是ascii码格式
            //组头部 收到的格式和信息
            data_head.append("SEND HEX TO ");
            data_head.append(dst_ip_port);
            data_head.append(" >\n");
        }
        //将发送的内容插入到 接受框内
         format.setForeground(QColor(109,109,109));//灰色
         ui->ReceiveTextEdit->setCurrentCharFormat(format);
         ui->ReceiveTextEdit->insertPlainText(data_head);//将头部份发到 展示框中

         //组装内容
         sendText.append(ui->tEditSendText->toPlainText());
         sendText.append("\n");
         format.setForeground(QColor(0, 0, 128));//蓝色
         ui->ReceiveTextEdit->setCurrentCharFormat(format);//设置颜色
         ui->ReceiveTextEdit->insertPlainText(sendText);//将内容发到 展示框中
         //将接受框下拉到最底下
         ui->ReceiveTextEdit->verticalScrollBar()->setSliderPosition(ui->ReceiveTextEdit->verticalScrollBar()->maximum());


         //真实的发送
         datagram= ui->tEditSendText->toPlainText().toUtf8();//从文本框内拿数据

         sndDataCnt += ui->tEditSendText->toPlainText().size();
//         qDebug()<<"123"<<sndDataCnt;
         ui->lEdit_SndCnt->setText(QString::number(sndDataCnt, 10));

         if(ui->cBoxNetType->currentIndex() == UDP_MODE) {//真实的发送 udp
             //发送
           dst_ip.setAddress(dst_ip_port.split(":").at(0));//将地址赋给 目的地址
           dst_port=dst_ip_port.split(":").at(1).toInt();//将端口赋值给 目的端口

           udpSocket->writeDatagram(datagram.data(), datagram.size(), dst_ip, dst_port);

         }
         else if(ui->cBoxNetType->currentIndex()==TCP_SERVER_MODE){
             //将接收到的该消息，发送给所有客户端

                for(int i=0; i<clientList.count(); i++)
                {
                    if(ui->cBox_ip_port2->currentIndex()==0){
                        clientList[i]->write(datagram.data(),datagram.size());
                    }else{
                        if(dst_ip_port==(clientList[i]->peerAddress().toString()+":"+QString::number(clientList[i]->peerPort(),10))){
                            clientList[i]->write(datagram.data(),datagram.size());
                        }
                    }
              }
         }
         else if(ui->cBoxNetType->currentIndex()==TCP_CLIENT_MODE){//真实的发送
              tcpSocket->write(datagram.data(),datagram.size());
          }
    }
}
void MainWindow::str_to_ASCII(){//点击要转换成ascii模式
//    qDebug()<<"--ascii";

    sendText = ui->tEditSendText->toPlainText();//用于接收文本的内容

    asciiStr="";    //出口初始化
    hexStr="";      //入口初始化

    if(send_state==2){        //hex->ascii 2->1 ，那么就要转义了
        send_state=1;

       hexStr=sendText.replace(" ","");// 00 2E 3F 11  ->002E3F11

//       十六进制字符串本身并不带有明确编码属性，用toUtf8是较为通用的做法

//        if(ui->actionANSI_GBK->isChecked()){
//            Code_mask_type = QTextCodec::codecForName("GBK");//获取 GBK 编码的解码器
//        }

//        if(ui->actionUTF_8->isChecked()){
//            Code_mask_type=QTextCodec::codecForName("UTF-8");//获取 UTF-8 编码的解码器
//        }

        byteArray = QByteArray::fromHex(hexStr.toUtf8());

        asciiStr = Code_mask_type->toUnicode(byteArray);
//        qDebug()<<asciiStr<<"---asc";
    }else{//这里没有转格式
        asciiStr=sendText;
    }
    //清空文本框
    ui->tEditSendText->clear();
    ui->tEditSendText->insertPlainText(asciiStr);

}

void MainWindow::str_to_HEX(){
//    qDebug()<<"-----hex";

    sendText = ui->tEditSendText->toPlainText();//用于接收文本的内容

    asciiStr="";//入口
    hexStr="";  //出口

    asciiStr=sendText;

    if(send_state==1){
        send_state=2;

//        if(ui->actionANSI_GBK->isChecked()){            //GBK使用这个
//            Code_mask_type=QTextCodec::codecForName("GBK");//获取 GBK 编码的解码器
//        }
//        if(ui->actionUTF_8->isChecked()){
//            Code_mask_type=QTextCodec::codecForName("UTF-8");//获取 UTF-8 编码的解码器
//        }

         byteArray=Code_mask_type->fromUnicode(asciiStr);//将str按utf8的格式转换
        for (int i = 0; i < byteArray.length(); ++i) {
            hexStr.append(QString("%1").arg(static_cast<unsigned char>(byteArray[i]), 2, 16, QChar('0')).toUpper());
            if (i < byteArray.length() - 1) {
                hexStr.append(" ");
            }
        }
    }else{  //相当于本来就是2
        hexStr=sendText;
    }
    ui->tEditSendText->clear();
    ui->tEditSendText->insertPlainText(hexStr);
}


void MainWindow::onNewConnection()
{
    //获取最新连接的客户端套接字
    tcpSocket = tcpServer_socket->nextPendingConnection();

    //将获取的套接字存放到客户端容器中
//    qDebug()<<tcpSocket->peerAddress()<<tcpSocket->peerPort();
    QString temp_item=tcpSocket->peerAddress().toString()+":"+QString::number(tcpSocket->peerPort(),10);
    clientList.push_back(tcpSocket);
//    qDebug()<<clientList.count();
    QString temp_head="All Connections ("+QString::number(clientList.count(),10)+")";
//    ui->cBox_ip_port2->currentIndex();

    ui->cBox_ip_port2->setCurrentIndex(0);
    ui->cBox_ip_port2->setItemText(0,temp_head);
    ui->cBox_ip_port2->addItem(temp_item);
//    [2025-01-02 02:40:11.727]# Client 192.168.20.1:59160 gets online

    data_head_time();
    data_head.append(" Client ");
    data_head.append(temp_item);
    data_head.append(" gets online\n\n");
    format.setForeground(QColor(109,109,109));
    ui->ReceiveTextEdit->setCurrentCharFormat(format);
    ui->ReceiveTextEdit->insertPlainText(data_head);

    //此时，客户端就和服务器建立起来联系了
    //如果客户端有数据向服务器发送过来，那么该套接字就会自动发送一个readyread信号
    //我们可以将该信号连接到自定义的槽函数中处理相关逻辑

    connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::readyRead_slot);
    connect(tcpSocket,&QTcpSocket::disconnected,this,&MainWindow::clientDisconnected);
//    qDebug() << "有新客户端连接";
    // 可以在这里添加更多与客户端通信的代码，如发送和接收数据
}
void MainWindow::clientDisconnected(){
//    qDebug()<<"456";
     //删除客户端链表中的无效客户端套接字
    for(int i=0; i<clientList.count(); i++)
    {
//        qDebug()<<"456";
//        qDebug()<<clientList.count();
//        qDebug()<<clientList[i]->state();
        //判断套接字的状态
        //函数原型     SocketState state() const;
        //功能：返回客户端状态
        //返回值：客户端状态，如果是0，表示无连接
        if(clientList[i]->state() == 0)
        {
            QString remv_dst_ip=clientList[i]->peerAddress().toString()+":"+QString::number(clientList[i]->peerPort(),10);

            data_head_time();
            data_head.append(" Client ");
            data_head.append(remv_dst_ip);
            data_head.append(" offline\n\n");

            format.setForeground(QColor(109,109,109));
            ui->ReceiveTextEdit->setCurrentCharFormat(format);
            ui->ReceiveTextEdit->insertPlainText(data_head);

            clientList.removeAt(i);     //将下标为i的客户端移除
            QString temp_head="All Connections ("+QString::number(clientList.count(),10)+")";
            ui->cBox_ip_port2->setCurrentIndex(0);
            ui->cBox_ip_port2->setItemText(0,temp_head);
            ui->cBox_ip_port2->removeItem(ui->cBox_ip_port2->findText(remv_dst_ip));
        }
    }
}
void MainWindow::readyRead_slot()
{
    //遍历所有客户端，查看是哪个客户端发来数据
    for(int i=0; i<clientList.count(); i++)
    {
        //函数原型：qint64 bytesAvailable() const override;
        //功能：返回当前客户端套接字中的可读数据字节个数
        //返回值：当前客户端待读的字节数，如果该数据0，表示无待读数据
        if(clientList[i]->bytesAvailable() != 0)
        {

            datagram = clientList[i]->readAll();//读取当前套接字中的所有数据，并返回一个字节数组
            dst_ip=clientList[i]->peerAddress();//拿到目的地址
            dst_port=clientList[i]->peerPort();//拿到目的端口
            dst_ip_port=dst_ip.toString()+":"+QString::number(dst_port,10);//组合
            ReceiveTextEdit_recvdata(datagram);//展示 到展示框中
            }
        }
    }
void MainWindow::create_file(){
    appDir = QCoreApplication::applicationDirPath();// 获取应用程序所在目录的绝对路径
    filePath = appDir + "/dst_ip_port.txt";
//    qDebug()<<filePath;
    file.setFileName(filePath);

    if (!file.exists()) {//如果不存在，则创建
            if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
                qDebug() << "文件创建成功";
                file.close();
            } else {
                qDebug() << "文件创建失败: " << file.errorString();
            }
        } else {//文件存在
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {

            QTextStream in(&file);//一行一行的读取数据
            while (!in.atEnd())
            {
                QString line = in.readLine();      //整行读取
                if(!line.isEmpty()){
                    ui->cBox_ip_port2->addItem(line);   //file->cBox_ip_port2 为了展示
                    ip_port_arr.insert(line);           // file -> ip_port_arr 为了不重复
                }

//                qDebug() << line;
            }
            file.close();

        }
    }
}

void MainWindow::on_pBtnResetCnt_clicked()
{
    sndDataCnt=0;
    ui->lEdit_SndCnt->setText("0");
    rcvDataCnt=0;
    ui->lEdit_RcvCnt->setText("0");

}
