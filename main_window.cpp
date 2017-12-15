#include <cstdint>
#include <functional>

#include <QTableView>
#include <QColorDialog>
#include <QVBoxLayout>
#include <QScreen>
#include <QLabel>

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
    HeaderViewWithCheckbox *headerWithCheckbox = new HeaderViewWithCheckbox(Qt::Horizontal);
    connect(headerWithCheckbox, &HeaderViewWithCheckbox::on_checkbox_click,
      model, &bulb_model::change_selection_state_for_all_bulbs);
    qtwBulbs->setHorizontalHeader(headerWithCheckbox);
    qtwBulbs->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->centralWidget->layout()->addWidget(qtwBulbs);
    qtwBulbs->setModel(model);
    qtwBulbs->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    qtwBulbs->horizontalHeader()->resizeSection(0, 20);
  }

  {
    auto s0 = QApplication::screens()[0];
    auto r = s0->geometry();
    auto pixmap = s0->grabWindow(0, r.left(), r.top(), r.width(), r.height());
    auto imgWidget = new QLabel(this);
    imgWidget->setPixmap(pixmap.scaled(480, 262));
    ui->centralWidget->layout()->addWidget(imgWidget);
    auto timer = new QTimer(this);
    timer->setInterval(500);
    connect(timer, &QTimer::timeout, [=]() {
      auto r = s0->geometry();
      auto pixmap = s0->grabWindow(0, r.left(), r.top(), r.width(), r.height());
      imgWidget->setPixmap(pixmap.scaled(480, 262));
    });
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
