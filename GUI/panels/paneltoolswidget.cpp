#include <QVBoxLayout>
#include <QMetaProperty>
#include <QMouseEvent>
#include <QInputDialog>

#include "paneltoolswidget.h"
#include "ui_paneltoolswidget.h"
#include "turnoutdisplay.h"
#include "propertyeditor.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("traingui.PanelToolsWidget");

PanelToolsWidget::PanelToolsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PanelToolsWidget)
{
    ui->setupUi(this);
    QVBoxLayout* propsVbox = new QVBoxLayout(ui->toolboxWidget);
    propsVbox->setContentsMargins(0, 0, 0, 0);

    // QVBoxLayout* elementsVbox = new QVBoxLayout(ui->elementsDisplay);
    // TurnoutDisplay* td = new TurnoutDisplay();
    // td->configureInteraction(false);
    // td->setProperty("layoutElement", true);
    // elementsVbox->addWidget(td);

    connect(ui->allowMoving, &QCheckBox::clicked,
            this, &PanelToolsWidget::allowMovingChanged);
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
        PropertyEditor* propEditor = new PropertyEditor(this, widget, prop);

        ui->toolboxWidget->layout()->addWidget(propEditor);
        m_currentProps.append(propEditor);
    }
}

void PanelToolsWidget::mousePressEvent(QMouseEvent* event){
    // QWidget* widgetAtPos = childAt(event->pos());
    // LOG4CXX_DEBUG_FMT(logger, "press button: {} pos: {},{} widget: {}",
    //                   (int)event->button(),
    //                   event->pos().x(),
    //                   event->pos().y(),
    //                   widgetAtPos ? "valid" : "invalid");
    // if(widgetAtPos->property("layoutElement").isValid()){
    //     // cool, this is something that we can drag and drop
    //     // TODO: figure out how to do this drag and drop properly.
    //     // TurnoutDisplay* td = new TurnoutDisplay(nullptr, Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus | Qt::Popup);
    //     // td->show();
    // }
}

// void mouseMoveEvent(QMouseEvent *event) override;
void PanelToolsWidget::mouseReleaseEvent(QMouseEvent *event){

}

void PanelToolsWidget::on_addTurnoutButton_clicked()
{
    // int dccVal = QInputDialog::getInt(this, "DCC switch number", "Please put DCC switch number", 1, 1, 2048);
    Q_EMIT addDCCTurnout();
}

