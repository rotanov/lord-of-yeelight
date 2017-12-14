#include <QtGui>
#include <QHeaderView>
#include <QStyleOptionViewItem>

#include "header_view_with_checkbox.hpp"

namespace Ui {
  class CustomHeader;
}

HeaderViewWithCheckbox::HeaderViewWithCheckbox(Qt::Orientation orientation, QWidget * parent)
  : QHeaderView(orientation, parent)
{

}

void HeaderViewWithCheckbox::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
  QHeaderView::paintSection(painter, rect, logicalIndex);
  if (logicalIndex == 0) {
    QStyleOptionButton option;
    option.rect = QRect(3, 4, 13, 13);
    if (is_selected) {
      option.state = QStyle::State_On;
    } else {
      option.state = QStyle::State_Off;
    }
    this->style()->drawControl(QStyle::CE_CheckBox, &option, painter);
  }
}

void HeaderViewWithCheckbox::mousePressEvent(QMouseEvent *event)
{
  int section = logicalIndexAt(event->pos());

  if (event->button() == Qt::LeftButton && section == 0) {
    if (is_selected) {
      is_selected = false;
    } else {
      is_selected = true;
    }

    this->viewport()->update();
    emit on_checkbox_click(is_selected);
  }

  QHeaderView::sectionClicked(section);
  QHeaderView::mousePressEvent(event);
}
