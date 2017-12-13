#pragma once

#include <QObject>
#include <QTableView>
#include <QHeaderView>

class HeaderViewWithCheckbox : public QHeaderView
{
  Q_OBJECT
public:
  HeaderViewWithCheckbox(Qt::Orientation orientation, QWidget * parent = 0);
protected:
  void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;
  void mousePressEvent(QMouseEvent *event) override;
private:
  bool is_selected;
signals:
  void on_checkbox_click(bool is_selected);
};
