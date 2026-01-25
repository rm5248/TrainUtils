#include "panellayout.h"

PanelLayout::PanelLayout(QWidget *parent)
    : QLayout{parent}
{

}

void PanelLayout::addItem(QLayoutItem *item){
    m_items.push_back(item);
}

QSize PanelLayout::sizeHint() const{
    return QSize(400,400);
}

void PanelLayout::setGeometry(const QRect& rect){
    QLayout::setGeometry(rect);
}

QLayoutItem *PanelLayout::itemAt(int index) const{
    if(m_items.length() <= index){
        return m_items[index];
    }
    return nullptr;
}

QLayoutItem *PanelLayout::takeAt(int index){
    if(m_items.length() <= index){
        QLayoutItem* item = m_items[index];
        m_items.erase(m_items.begin() + index);
        return item;
    }
    return nullptr;
}

int PanelLayout::count() const{
    return m_items.size();
}
