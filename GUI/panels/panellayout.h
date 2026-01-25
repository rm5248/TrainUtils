#ifndef PANELLAYOUT_H
#define PANELLAYOUT_H

#include <QObject>
#include <QLayout>
#include <QVector>

class PanelLayout : public QLayout
{
    Q_OBJECT
public:
    explicit PanelLayout(QWidget *parent = nullptr);

    void addItem(QLayoutItem *item) override;
    QSize sizeHint() const override;
    void setGeometry(const QRect& rect) override;
    QLayoutItem *itemAt(int index) const override;
    QLayoutItem *takeAt(int index) override;
    int count() const;

Q_SIGNALS:

private:
    QVector<QLayoutItem*> m_items;

};

#endif // PANELLAYOUT_H
