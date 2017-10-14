#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include <QtNetwork/QHostInfo>
#include <QtNetwork>
#include <QByteArray>
#include <stdint.h>
#include <QDebug>
#include <QProcess>
#include <QTableView>
#include <QTableWidget>
#include <QColorDialog>
#include <QAbstractTableModel>
#include <QVBoxLayout>

class QBulbModel : public QAbstractTableModel
{
  //Q_OBJECT
private:
  QList<::bulb> bulbs;

public:
  bool have_bulb(const ::bulb& bulb)
  {
    return std::find(bulbs.begin(), bulbs.end(), bulb) != bulbs.end();
  }

  void add_bulb(const ::bulb& bulb)
  {
    int n = bulbs.size();
    beginInsertRows(QModelIndex(), n, n);
    bulbs.push_back(bulb);
    endInsertRows();
  }

  int size()
  {
    return bulbs.size();
  }

  ::bulb& operator [](int i)
  {
    return bulbs[i];
  }

  QBulbModel()
  {

  }
  // Inherited via QAbstractTableModel
  virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override
  {
    return bulbs.size();
  }

  virtual int columnCount(const QModelIndex & parent = QModelIndex()) const override
  {
    return 3;
  }

  virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override
  {
    int row = index.row();
    int column = index.column();
    auto& b = bulbs[row];
    switch (role) {
    case Qt::DisplayRole: {
      switch (column) {
      case 0: return b.get_id_str();
      case 1: return b.get_ip_str();
      case 2: return b.get_port();
      }
    }
    case Qt::CheckStateRole: {
      return QVariant();// Qt::Checked;
    }
    default: return QVariant();
    }
    if (role == Qt::CheckStateRole) {
      //
    }
  }

  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
  {
    if (role != Qt::DisplayRole) {
      return QVariant();
    }
    if (orientation != Qt::Horizontal) {
      return QVariant();
    }
    switch (section) {
    case 0: return "id";
    case 1: return "ip";
    case 2: return "port";
    }
  }

  virtual Qt::ItemFlags flags(const QModelIndex &index) const override
  {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }

};

static int message_id = 0;
static QBulbModel* model;

static QString get_id()
{
  return QString::number(message_id++);
}

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  model = new QBulbModel();
  connect(ui->qpb_initialize_db, &QPushButton::clicked,
    this, &MainWindow::on_qpb_initialize_db_clicked);

  MainWindow::setWindowTitle("Lord of Yeelight");

  {
    auto qtwBulbs = new QTableView(this->centralWidget());
    qtwBulbs->setSelectionBehavior(QAbstractItemView::SelectRows);
    qtwBulbs->verticalHeader()->hide();
    //qtwBulbs->horizontalHeader()->setStretchLastSection(true);
    qtwBulbs->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->centralWidget->layout()->addWidget(qtwBulbs);
    qtwBulbs->setModel(model);
  }

  mcast_addr = "239.255.255.250";
  udp_port = 1982;

  {
    QString localHostName = QHostInfo::localHostName();
    QHostInfo info = QHostInfo::fromName(localHostName);
    foreach (QHostAddress address, info.addresses()) {
      if(address.protocol() == QAbstractSocket::IPv4Protocol) {
        local_ip = address.toString();
        qDebug()<<"IP:"<<address.toString();
      }
    }
  }

  {
    udp_send_socket = new QUdpSocket(this);
    udp_receive_socket = new QUdpSocket(this);

    udp_send_socket->close();
    if (!udp_send_socket->bind(QHostAddress(local_ip), 0, QUdpSocket::ReuseAddressHint)) {
      qDebug() << "udp_send_socket udp bind failed";
    } else {
      qDebug() << "udp_send_socket udp bind success";
    }
    if (!udp_send_socket->joinMulticastGroup(mcast_addr)) {
      qDebug() << "udp_send_socket failed joining multicast group.";
    } else {
      qDebug() << "udp_send_socket joined multicast group";
    }
    connect(udp_send_socket, &QUdpSocket::readyRead,
            this, &MainWindow::processPendingDatagrams);
    udp_receive_socket->close();
    if (!udp_receive_socket->bind(QHostAddress::AnyIPv4, udp_port, QUdpSocket::ReuseAddressHint)) {
      qDebug() << "udp_receive_socket udp bind failed";
    } else {
      qDebug() << "udp_receive_socket udp bind success";
    }
    if (!udp_receive_socket->joinMulticastGroup(mcast_addr)) {
      qDebug() << "udp_receive_socket failed joining multicast group.";
    } else {
      qDebug() << "udp_receive_socket joined multicast group";
    }
    connect(udp_receive_socket, &QUdpSocket::readyRead,
            this, &MainWindow::processPendingDatagrams);
  }

  discover();
}

MainWindow::~MainWindow()
{
  for (auto socket : tcp_sockets) {
    socket->close();
  }
  udp_send_socket->close();
  udp_receive_socket->close();
  delete ui;
}

void MainWindow::processPendingDatagrams()
{
  auto udp_socket = static_cast<QUdpSocket*>(QObject::sender());
  while (udp_socket->hasPendingDatagrams()) {
    qDebug() << "udp receive data";
    udp_datagram_recv.resize(udp_socket->pendingDatagramSize());
    udp_socket->readDatagram(udp_datagram_recv.data(), udp_datagram_recv.size());
    qDebug() << udp_datagram_recv.data();

    bulb_ip = sub_string("Location: yeelight://", ":");
    bulb_id_str = sub_string("id: ", "\r\n");
    ::bulb bulb_tmp(bulb_ip, bulb_id_str);
    if (!model->have_bulb(bulb_tmp)) {
      model->add_bulb(bulb_tmp);
    }
  }
}

QByteArray MainWindow::sub_string(const char* start_str, const char* end_str)
{
  QByteArray result;
  int pos1 = -1;
  int pos2 = -1;
  result.clear();
  auto prefix = QByteArray(start_str);
  auto suffix = QByteArray(end_str);
  pos1 = udp_datagram_recv.indexOf(prefix, 0);
  if (pos1 != -1) {
    result = udp_datagram_recv.mid(pos1);
    pos1 = prefix.length();
    result = result.mid(pos1);
    pos2 = result.indexOf(suffix);
    result = result.mid(0, pos2);
  }
  return result;
}

void MainWindow::on_qpb_discover_clicked()
{
  discover();
}

void MainWindow::discover()
{
  QByteArray datagram = "M-SEARCH * HTTP/1.1\r\n"
    "HOST: 239.255.255.250:1982\r\n"
    "MAN: \"ssdp:discover\"\r\n"
    "ST: wifi_bulb";
  int ret = udp_send_socket->writeDatagram(datagram.data(), datagram.size(), mcast_addr, udp_port);
  qDebug() << "udp write " << ret << " bytes";
}

void MainWindow::on_qpb_connect_clicked()
{
  connect_to_all_bulbs();
}

void MainWindow::connect_to_all_bulbs()
{
  if (!tcp_server.isListening()) {
    tcp_server.listen(QHostAddress(local_ip), 0);
    connect(&tcp_server, &QTcpServer::newConnection,
      this, &MainWindow::on_new_tcp_server_connection);
  }
  while (tcp_sockets.size() < model->size()) {
    tcp_sockets.append(new QTcpSocket(this));
  }
  for (int i = 0; i < model->size(); i++) {
    auto& b = (*model)[i];
    auto& s = *tcp_sockets[i];
    if (!s.isOpen()) {
      s.connectToHost(QHostAddress(b.get_ip_str()), b.get_port());
      connect(&s, &QTcpSocket::readyRead,
        this, &MainWindow::on_ready_read_tcp_socket);
    }
  }
}

void MainWindow::on_qpb_toggle_clicked()
{
  connect_to_all_bulbs();
  for (int i = 0; i < model->size(); i++) {
    auto& b = (*model)[i];
    auto& s = *tcp_sockets[i];
    QByteArray cmd_str;
    cmd_str.append("{\"id\":");
    cmd_str.append(get_id());
    cmd_str.append(",\"method\":\"toggle\",\"params\":[]}\r\n");
    s.write(cmd_str.data());
  }
}

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
  //Q_UNUSED(value);
  //int pos = ui->horizontalSlider->value();
  //QString slider_value = QString("%1").arg(pos) + "%";
  //QByteArray *cmd_str =new QByteArray;
  //cmd_str->clear();
  //cmd_str->append("{\"id\":");

  //int device_idx = ui->comboBox->currentIndex();
  //if (bulb.size() > 0) {
  //  cmd_str->append(bulb[device_idx].get_id());
  //  qDebug() << "combox index  = " << device_idx;

  //  cmd_str->append(",\"method\":\"set_bright\",\"params\":[");
  //  cmd_str->append(QString("%1").arg(pos));
  //  cmd_str->append(", \"smooth\", 500]}\r\n");
  //  tcp_socket.write(cmd_str->data());
  //  qDebug() << cmd_str->data();
  //} else {
  //  qDebug()<<"Bulb is empty";
  //}
}

void MainWindow::on_qpb_initialize_db_clicked()
{
  storage.initialize_db();
}

void MainWindow::on_qpb_color_dialog_clicked()
{
  connect_to_all_bulbs();
  QColor color = QColorDialog::getColor(Qt::yellow, this);
  auto hue = color.hsvHue();
  if (hue == -1) {
    hue = 0;
  }
  auto sat = static_cast<int>(color.hsvSaturationF() * 100);
  for (int i = 0; i < model->size(); i++) {
    auto& b = (*model)[i];
    auto& s = *tcp_sockets[i];
    QByteArray* cmd_str = new QByteArray();
    cmd_str->clear();
    cmd_str->append(QString("{\"id\":%1,\"method\":\"set_hsv\",\"params\":[%2,%3,\"sudden\",500]}\r\n")
      .arg(get_id(), QString::number(hue), QString::number(sat)));
    s.write(cmd_str->data());
    cmd_str->clear();
    cmd_str->append(QString("{\"id\":%1,\"method\":\"set_bright\",\"params\":[%2,\"sudden\",500]}\r\n")
      .arg(get_id(), QString::number(ui->horizontalSlider->value())));
    s.write(cmd_str->data());
  }
}

void MainWindow::on_qpb_qpb_wreak_havoc_clicked()
{
  connect_to_all_bulbs();
  for (int i = 0; i < model->size(); i++) {
    auto& b = (*model)[i];
    auto& s = *tcp_sockets[i];
    QByteArray cmd_str;
    cmd_str.append(
      QString("{\"id\":%1,\"method\":\"set_music\",\"params\":[1,\"%2\",%3]}\r\n")
      .arg(get_id(), local_ip, QString::number(tcp_server.serverPort()))
      );
    s.write(cmd_str.data());
  }
  auto timer = new QTimer(this);
  timer->setInterval(200);
  timer->start();
  connect(timer, &QTimer::timeout, this, &MainWindow::wreak_havoc);
}

static int cycle = 0;

void MainWindow::wreak_havoc()
{
  cycle++;
  connect_to_all_bulbs();
  auto hue = 0;// rand() % 360;
  if (hue == -1) {
    hue = 0;
  }
  auto sat = (cycle % 2) * 100;// rand() % 100;
  if (unlimited_tcp_sockets.size() != model->size()) {
    return;
  }
  for (int i = 0; i < model->size(); i++) {
    auto& b = (*model)[i];
    if (i >= unlimited_tcp_sockets.size()) {
      return;
    }
    auto& s = *unlimited_tcp_sockets[i];
    QByteArray cmd_str;
    //cmd_str.clear();
    //cmd_str.append(QString("{\"id\":%1,\"method\":\"set_hsv\",\"params\":[%2,%3,\"sudden\",500]}\r\n")
    //  .arg(get_id(), QString::number(hue), QString::number(sat)));
    //s.write(cmd_str.data());
    cmd_str.clear();
    cmd_str.append(QString("{\"id\":%1,\"method\":\"set_bright\",\"params\":[%2,\"sudden\",0]}\r\n")
      .arg(get_id(), QString::number((cycle % 2) == 0 ? 1 : 100)));
    s.write(cmd_str.data());
  }
}

void MainWindow::on_ready_read_tcp_socket()
{
  QTcpSocket *socket = static_cast<QTcpSocket*>(sender());
  QByteArray buffer;
  while (socket->bytesAvailable() > 0)
  {
    buffer.append(socket->readAll());
    qDebug() << buffer;
  }
}

void MainWindow::on_new_tcp_server_connection()
{
  auto& s = *tcp_server.nextPendingConnection();
  connect(&s, &QTcpSocket::readyRead,
    this, &MainWindow::on_ready_read_tcp_socket);
  unlimited_tcp_sockets.append(&s);
}