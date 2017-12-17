#include <cstdint>
#include <functional>

#include <QTableView>
#include <QColorDialog>
#include <QVBoxLayout>
#include <QScreen>
#include <QLabel>
#include <QComboBox>

#include "main_window.hpp"
#include "ui_main_window.h"
#include "header_view_with_checkbox.hpp"

main_window::main_window(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::main_window)
{
  ui->setupUi(this);
  model = new bulb_model();
  connect(ui->qpb_initialize_db, &QPushButton::clicked,
    this, &main_window::on_qpb_initialize_db_clicked);

  {
    auto qtwBulbs = new QTableView(this->centralWidget());
    qtwBulbs->setSelectionBehavior(QAbstractItemView::SelectRows);
    qtwBulbs->verticalHeader()->hide();
    header_view_with_checkbox *header_view_with_checkbox = new ::header_view_with_checkbox(Qt::Horizontal);
    connect(header_view_with_checkbox, &header_view_with_checkbox::on_checkbox_click,
      model, &bulb_model::change_selection_state_for_all_bulbs);
    qtwBulbs->setHorizontalHeader(header_view_with_checkbox);
    qtwBulbs->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->centralWidget->layout()->addWidget(qtwBulbs);
    qtwBulbs->setModel(model);
    qtwBulbs->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    qtwBulbs->horizontalHeader()->resizeSection(0, 20);
  }

  {
    auto qcbSelectScreen = new QComboBox(this);
    screenIndex = 0;
    for (auto& s : QApplication::screens()) {
      qcbSelectScreen->addItem(s->name(), screenIndex);
      screenIndex++;
    }
    screenIndex = 0;
    ui->centralWidget->layout()->addWidget(qcbSelectScreen);
    connect(qcbSelectScreen, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index){
      screenIndex = index;
    });
    auto imgWidget = new QLabel(this);
    ui->centralWidget->layout()->addWidget(imgWidget);
    auto timer = new QTimer(this);
    timer->setInterval(500);
    auto updateScreen = [=]() {
      auto s0 = QApplication::screens()[screenIndex];
      auto r = s0->geometry();
      auto pixmap = s0->grabWindow(screenIndex, r.left(), r.top(), r.width(), r.height());
      imgWidget->setPixmap(pixmap.scaled(480, 262));
    };
    connect(timer, &QTimer::timeout, updateScreen);
    updateScreen();
    timer->start();
  }
  model->discover_bulbs();
}

main_window::~main_window()
{
  delete ui;
}

void main_window::on_qpb_discover_clicked()
{
  model->discover_bulbs();
}

void main_window::on_qpb_connect_clicked()
{
  model->connect_to_all_bulbs();
}

void main_window::on_qpb_toggle_clicked()
{
  model->toggle_bulbs();
}

void main_window::on_horizontalSlider_valueChanged(int value)
{
  model->set_brightness(value);
}

void main_window::on_q_slider_color_temperature_valueChanged(int value)
{
  model->set_color_temperature(value);
}

void main_window::on_qpb_initialize_db_clicked()
{
  storage.initialize_db();
}

void main_window::on_qpb_color_dialog_clicked()
{
  QColor color = QColorDialog::getColor(Qt::yellow, this);
  model->set_color(color);
}

void main_window::on_qpb_wreak_havoc_clicked()
{
  model->begin_havok();
}
