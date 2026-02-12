#include "propertyeditor.h"
#include "ui_propertyeditor.h"

PropertyEditor::PropertyEditor(QWidget *parent, QObject* modifyObj, QMetaProperty prop)
    : QWidget(parent)
    , ui(new Ui::PropertyEditor)
{
    ui->setupUi(this);
    m_objToModify = modifyObj;
    m_objProperty = prop;

    ui->name_label->setText(prop.name());
    if(prop.isEnumType()){
        ui->lineEdit->hide();
        QMetaEnum enumMeta = prop.enumerator();
        for(int x = 0; x < enumMeta.keyCount(); x++){
            ui->comboBox->addItem(enumMeta.key(x));
        }
    }else{
        ui->comboBox->hide();
    }
}

PropertyEditor::~PropertyEditor()
{
    delete ui;
}

void PropertyEditor::on_comboBox_activated(int index)
{
    QMetaEnum enumMeta = m_objProperty.enumerator();
    m_objProperty.write(m_objToModify, enumMeta.value(index));
}

