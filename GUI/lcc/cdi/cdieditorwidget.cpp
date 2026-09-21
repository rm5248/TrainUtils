/* SPDX-License-Identifier: GPL-2.0 */
#include "cdieditorwidget.h"
#include "ui_cdieditorwidget.h"

#include <limits>

#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QTreeWidgetItem>
#include <QXmlStreamReader>

#include "cdivariant.h"
#include "eventidtype.h"
#include "inttype.h"
#include "maptype.h"
#include "stringtype.h"

CdiEditorWidget::CdiEditorWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CdiEditorWidget)
{
    ui->setupUi(this);

    connect(ui->cdiTree, &QTreeWidget::itemSelectionChanged,
            this, &CdiEditorWidget::onTreeSelectionChanged);
}

CdiEditorWidget::~CdiEditorWidget()
{
    delete ui;
}

void CdiEditorWidget::on_openButton_clicked(){
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open CDI XML"),
                                                      QString(),
                                                      tr("CDI XML Files (*.xml);;All Files (*)"));
    if(fileName.isEmpty()){
        return;
    }

    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QMessageBox::warning(this, tr("Open CDI XML"),
                              tr("Could not open file: %1").arg(fileName));
        return;
    }

    QXmlStreamReader reader(&file);
    CDI newCdi = CDI::createFromXML(&reader);
    file.close();

    if(reader.hasError()){
        QMessageBox::warning(this, tr("Open CDI XML"),
                              tr("XML parse error: %1").arg(reader.errorString()));
        return;
    }

    m_cdi = newCdi;
    ui->filePathLabel->setText(fileName);
    rebuildTree();
}

void CdiEditorWidget::rebuildTree(){
    clearFields();
    ui->cdiTree->clear();

    for(const Segment& segment : m_cdi.segments()){
        QTreeWidgetItem* segmentItem = new QTreeWidgetItem(ui->cdiTree);
        segmentItem->setText(0, segment.name());
        segmentItem->setData(0, Qt::UserRole,
                              QVariant::fromValue(static_cast<void*>(
                                  const_cast<QVector<CDIVariant>*>(&segment.elements()))));
        buildGroupChildren(segmentItem, segment.elements());
    }

    ui->cdiTree->expandToDepth(0);
}

void CdiEditorWidget::buildGroupChildren(QTreeWidgetItem* parentItem, const QVector<CDIVariant>& elements){
    for(const CDIVariant& variant : elements){
        if(!cdiVariantIsGroup(variant)){
            // Leaf entries (int/string/eventid) are shown as fields on the
            // right side when their containing node is selected, not as
            // their own tree entries.
            continue;
        }

        std::shared_ptr<GroupType> group = std::get<std::shared_ptr<GroupType>>(variant);
        int repetitions = group->replication() > 1 ? group->replication() : 1;

        if(repetitions == 1 && group->name().isEmpty() && group->elements().isEmpty()){
            // A bare <group offset='n'/> used purely to pad/align storage -
            // nothing to show.
            continue;
        }

        QString label = group->repname().isEmpty() ? group->name() : group->repname();

        for(int rep = 1; rep <= repetitions; rep++){
            QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
            item->setText(0, repetitions > 1 ? QStringLiteral("%1 %2").arg(label).arg(rep) : label);
            item->setData(0, Qt::UserRole,
                          QVariant::fromValue(static_cast<void*>(
                              const_cast<QVector<CDIVariant>*>(&group->elements()))));
            buildGroupChildren(item, group->elements());
        }
    }
}

void CdiEditorWidget::onTreeSelectionChanged(){
    clearFields();

    QList<QTreeWidgetItem*> selected = ui->cdiTree->selectedItems();
    if(selected.isEmpty()){
        return;
    }

    QVariant data = selected.first()->data(0, Qt::UserRole);
    if(!data.isValid()){
        return;
    }

    auto* elements = static_cast<QVector<CDIVariant>*>(data.value<void*>());
    if(!elements){
        return;
    }

    for(const CDIVariant& variant : *elements){
        if(!cdiVariantIsGroup(variant)){
            addFieldForVariant(variant);
        }
    }
}

void CdiEditorWidget::clearFields(){
    while(ui->fieldsLayout->rowCount() > 0){
        ui->fieldsLayout->removeRow(0);
    }
}

void CdiEditorWidget::addFieldForVariant(const CDIVariant& variant){
    if(auto strType = std::get_if<std::shared_ptr<StringType>>(&variant)){
        StringType* s = strType->get();
        QLineEdit* edit = new QLineEdit(ui->fieldsContainer);
        if(s->size() > 0){
            edit->setMaxLength(s->size());
        }
        addFormRow(s->name(), s->description(), edit);
    }else if(auto intType = std::get_if<std::shared_ptr<IntType>>(&variant)){
        IntType* i = intType->get();
        if(i->map().has_value()){
            QComboBox* combo = new QComboBox(ui->fieldsContainer);
            for(const MapType::Relation& relation : i->map()->relations()){
                combo->addItem(relation.value, MapType::propertyToVariant(relation.property));
            }
            addFormRow(i->name(), i->description(), combo);
        }else{
            QSpinBox* spin = new QSpinBox(ui->fieldsContainer);
            int defaultMax = std::numeric_limits<int>::max();
            switch(i->size()){
            case 1: defaultMax = 255; break;
            case 2: defaultMax = 65535; break;
            case 3: defaultMax = 16777215; break;
            default: break;
            }
            spin->setRange(i->min().value_or(0), i->max().value_or(defaultMax));
            if(i->defaultValue().has_value()){
                spin->setValue(i->defaultValue().value());
            }
            addFormRow(i->name(), i->description(), spin);
        }
    }else if(auto eventType = std::get_if<std::shared_ptr<EventIDType>>(&variant)){
        EventIDType* e = eventType->get();
        QLineEdit* edit = new QLineEdit(ui->fieldsContainer);
        edit->setPlaceholderText(QStringLiteral("00.00.00.00.00.00.00.00"));
        addFormRow(e->name(), e->description(), edit);
    }
}

void CdiEditorWidget::addFormRow(const QString& name, const QString& description, QWidget* field){
    if(!description.isEmpty()){
        field->setToolTip(description);
    }
    ui->fieldsLayout->addRow(name, field);
}
