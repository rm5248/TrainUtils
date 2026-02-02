#include <QVBoxLayout>
#include <QMetaProperty>

#include "paneltoolswidget.h"
#include "ui_paneltoolswidget.h"

PanelToolsWidget::PanelToolsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PanelToolsWidget)
{
    ui->setupUi(this);
    new QVBoxLayout(ui->toolboxWidget);
}

PanelToolsWidget::~PanelToolsWidget()
{
    delete ui;
}

void PanelToolsWidget::setCurrentSelectedWidget(QWidget* widget){
    for(QWidget* wid : m_currentProps){
        ui->toolboxWidget->layout()->removeWidget(wid);
        delete wid;
    }
    m_currentProps.clear();

    if(widget == nullptr){
        return;
    }

    // Loop through all of the properties of this widget,
    // add them to the properties list
    const QMetaObject* metaObj = widget->metaObject();
    for(int idx = 0; idx < metaObj->propertyCount(); idx++){
        QMetaProperty prop = metaObj->property(idx);
        QString propName = prop.name();
        if(!propName.startsWith("traingui_")){
            continue;
        }
        propName = propName.remove(0, 9);
        QLabel* lbl = new QLabel(this);
        lbl->setText(propName);
        ui->toolboxWidget->layout()->addWidget(lbl);
        m_currentProps.append(lbl);
    }
}
