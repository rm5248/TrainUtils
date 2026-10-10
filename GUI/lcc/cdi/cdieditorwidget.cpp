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
#include "lcc/lccnode.h"
#include "lcc/segmentmemory.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.lcc.CdiEditorWidget" );

// Data stored on each tree item, telling us where its elements live in memory
static constexpr int ElementsRole = Qt::UserRole;
static constexpr int SpaceRole = Qt::UserRole + 1;
static constexpr int AddressRole = Qt::UserRole + 2;

CdiEditorWidget::CdiEditorWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CdiEditorWidget)
{
    ui->setupUi(this);

    connect(ui->cdiTree, &QTreeWidget::itemSelectionChanged,
            this, &CdiEditorWidget::onTreeSelectionChanged);

    // When resizing, keep the tree at ~30% of the width and the fields at ~70%
    ui->splitter->setStretchFactor(0, 3);
    ui->splitter->setStretchFactor(1, 7);
}

CdiEditorWidget::~CdiEditorWidget()
{
    delete ui;
}

void CdiEditorWidget::showEvent(QShowEvent* event){
    QWidget::showEvent(event);

    // Our actual width isn't known until we are shown, so set the initial split here
    if(!m_splitterSized){
        m_splitterSized = true;
        int width = ui->splitter->width();
        ui->splitter->setSizes({width * 3 / 10, width - (width * 3 / 10)});
    }
}

void CdiEditorWidget::setLCCNode(std::shared_ptr<LCCNode> node){
    m_node = node;
    ui->openButton->hide();

    // Stay connected so that if the CDI is re-read from the node, we update as well
    connect(m_node.get(), &LCCNode::cdiRead,
            this, &CdiEditorWidget::nodeCDIRead);
    connect(m_node.get(), &LCCNode::cdiReadFailure,
            this, &CdiEditorWidget::nodeCDIReadFailure);

    if(m_node->hasCDI()){
        nodeCDIRead();
        return;
    }

    ui->filePathLabel->setText(tr("Reading CDI from node..."));
    SegmentMemory* mem = m_node->readCDI();
    if(mem){
        ui->cdiProgress->setValue(0);
        ui->cdiProgress->show();
        connect(mem, &SegmentMemory::downloadProgress,
                this, &CdiEditorWidget::cdiDownloadProgress);
    }
}

void CdiEditorWidget::nodeCDIRead(){
    ui->cdiProgress->hide();
    m_cdi = m_node->cdi();

    Identification ident = m_cdi.identification();
    m_nodeDescription = QStringLiteral("%1 %2 (HW: %3 SW: %4)")
                               .arg(ident.manufacturer(),
                                    ident.model(),
                                    ident.hardwareVersion(),
                                    ident.softwareVersion());
    ui->filePathLabel->setText(m_nodeDescription);
    rebuildTree();
    readConfigurationMemory();
}

void CdiEditorWidget::readConfigurationMemory(){
    if(!m_node || m_currentSegmentRead){
        // Either no node, or we are already reading
        return;
    }

    m_errors.clear();
    ui->errorLabel->hide();

    m_segmentsToRead.clear();
    for(const Segment& segment : m_cdi.segments()){
        if(!m_segmentsToRead.contains(segment.space())){
            m_segmentsToRead.push_back(segment.space());
        }
    }
    m_totalSegmentsToRead = m_segmentsToRead.size();

    readNextSegment();
}

void CdiEditorWidget::readNextSegment(){
    m_currentSegmentRead = nullptr;

    while(!m_segmentsToRead.isEmpty()){
        int space = m_segmentsToRead.takeFirst();
        SegmentMemory* mem = m_node->segmentMemory(space);
        if(!mem || mem->hasMemory()){
            continue;
        }

        // Segments are read one at a time, as the connection can't handle multiple
        // outstanding requests to the same node
        m_currentSegmentRead = mem;
        connect(mem, &SegmentMemory::memoryReady,
                this, &CdiEditorWidget::segmentMemoryReady, Qt::UniqueConnection);
        connect(mem, &SegmentMemory::memoryReadFailed,
                this, &CdiEditorWidget::segmentMemoryReadFailed, Qt::UniqueConnection);
        connect(mem, &SegmentMemory::downloadProgress,
                this, &CdiEditorWidget::cdiDownloadProgress, Qt::UniqueConnection);

        ui->filePathLabel->setText(tr("Reading configuration memory: space 0x%1 (%2 of %3)...")
                                   .arg(space, 2, 16, QLatin1Char('0'))
                                   .arg(m_totalSegmentsToRead - m_segmentsToRead.size())
                                   .arg(m_totalSegmentsToRead));
        ui->cdiProgress->setValue(0);
        ui->cdiProgress->show();
        mem->readAllMemory();
        return;
    }

    // Nothing left to read
    ui->cdiProgress->hide();
    ui->filePathLabel->setText(m_nodeDescription);
}

void CdiEditorWidget::segmentMemoryReady(){
    if(sender() != m_currentSegmentRead){
        return;
    }

    // Re-create the displayed fields so that they pick up the values we just read
    onTreeSelectionChanged();
    readNextSegment();
}

void CdiEditorWidget::segmentMemoryReadFailed(uint16_t error_code, QString message){
    if(sender() != m_currentSegmentRead){
        return;
    }

    LOG4CXX_ERROR_FMT(logger, "Unable to read configuration memory space {:X}: {}",
                      m_currentSegmentRead->segmentId(), message.toStdString());
    showError(tr("Unable to read memory space 0x%1: %2")
              .arg(m_currentSegmentRead->segmentId(), 2, 16, QLatin1Char('0'))
              .arg(message));

    // Keep going with the other segments - they may still be readable
    readNextSegment();
}

bool CdiEditorWidget::memoryBytes(int space, uint32_t address, int length, QVector<uint8_t>* out){
    if(!m_node || length <= 0){
        return false;
    }

    SegmentMemory* mem = m_node->segmentMemory(space);
    if(!mem || !mem->hasMemory() || address < mem->startingAddress()){
        return false;
    }

    const QVector<uint8_t> data = mem->data();
    qsizetype index = address - mem->startingAddress();
    if(index + length > data.size()){
        return false;
    }

    *out = data.mid(index, length);
    return true;
}

void CdiEditorWidget::nodeCDIReadFailure(uint16_t error_code, QString error_string){
    ui->cdiProgress->hide();
    ui->filePathLabel->setText(tr("Unable to read CDI"));
    showError(tr("Unable to read CDI: %1").arg(error_string));
}

void CdiEditorWidget::showError(const QString& error){
    m_errors.push_back(error);
    ui->errorLabel->setText(m_errors.join('\n'));
    ui->errorLabel->show();
}

void CdiEditorWidget::cdiDownloadProgress(uint64_t bytes_rx, uint64_t bytes_total){
    if(bytes_total == 0){
        return;
    }
    ui->cdiProgress->setValue(static_cast<int>(bytes_rx * 100 / bytes_total));
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
        segmentItem->setText(0, segment.name().isEmpty() ?
                                    tr("Space 0x%1").arg(segment.space(), 2, 16, QLatin1Char('0')) :
                                    segment.name());
        segmentItem->setData(0, ElementsRole,
                              QVariant::fromValue(static_cast<void*>(
                                  const_cast<QVector<CDIVariant>*>(&segment.elements()))));
        segmentItem->setData(0, SpaceRole, segment.space());
        segmentItem->setData(0, AddressRole, static_cast<uint>(segment.origin()));
        buildGroupChildren(segmentItem, segment.elements(), segment.space(), segment.origin());
    }

    ui->cdiTree->expandToDepth(0);
}

void CdiEditorWidget::buildGroupChildren(QTreeWidgetItem* parentItem, const QVector<CDIVariant>& elements, int space, uint32_t address){
    for(const CDIVariant& variant : elements){
        address += cdiVariantOffset(variant);
        uint32_t elementAddress = address;
        address += cdiVariantSize(variant);

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
        int repetitionSize = cdiElementsSize(group->elements());

        for(int rep = 1; rep <= repetitions; rep++){
            uint32_t repetitionAddress = elementAddress + (rep - 1) * repetitionSize;
            QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
            item->setText(0, repetitions > 1 ? QStringLiteral("%1 %2").arg(label).arg(rep) : label);
            item->setData(0, ElementsRole,
                          QVariant::fromValue(static_cast<void*>(
                              const_cast<QVector<CDIVariant>*>(&group->elements()))));
            item->setData(0, SpaceRole, space);
            item->setData(0, AddressRole, static_cast<uint>(repetitionAddress));
            buildGroupChildren(item, group->elements(), space, repetitionAddress);
        }
    }
}

void CdiEditorWidget::onTreeSelectionChanged(){
    clearFields();

    QList<QTreeWidgetItem*> selected = ui->cdiTree->selectedItems();
    if(selected.isEmpty()){
        return;
    }

    QTreeWidgetItem* item = selected.first();
    QVariant data = item->data(0, ElementsRole);
    if(!data.isValid()){
        return;
    }

    auto* elements = static_cast<QVector<CDIVariant>*>(data.value<void*>());
    if(!elements){
        return;
    }

    int space = item->data(0, SpaceRole).toInt();
    uint32_t address = item->data(0, AddressRole).toUInt();

    for(const CDIVariant& variant : *elements){
        address += cdiVariantOffset(variant);
        if(!cdiVariantIsGroup(variant)){
            addFieldForVariant(variant, space, address);
        }
        address += cdiVariantSize(variant);
    }
}

void CdiEditorWidget::clearFields(){
    while(ui->fieldsLayout->rowCount() > 0){
        ui->fieldsLayout->removeRow(0);
    }
}

void CdiEditorWidget::addFieldForVariant(const CDIVariant& variant, int space, uint32_t address){
    QVector<uint8_t> bytes;
    bool haveValue = memoryBytes(space, address, cdiVariantSize(variant), &bytes);

    if(auto strType = std::get_if<std::shared_ptr<StringType>>(&variant)){
        StringType* s = strType->get();
        QLineEdit* edit = new QLineEdit(ui->fieldsContainer);
        if(s->size() > 0){
            edit->setMaxLength(s->size());
        }
        if(haveValue){
            // Strings are null-terminated, unless they fill the entire field
            qsizetype len = bytes.indexOf(0);
            if(len < 0){
                len = bytes.size();
            }
            edit->setText(QString::fromUtf8(reinterpret_cast<const char*>(bytes.constData()), len));
        }
        addFormRow(s->name(), s->description(), edit);
    }else if(auto intType = std::get_if<std::shared_ptr<IntType>>(&variant)){
        IntType* i = intType->get();
        uint64_t value = 0;
        if(haveValue){
            // LCC is big-endian
            for(uint8_t b : bytes){
                value = (value << 8) | b;
            }
        }

        if(i->map().has_value()){
            QComboBox* combo = new QComboBox(ui->fieldsContainer);
            int selectedIndex = -1;
            for(const MapType::Relation& relation : i->map()->relations()){
                combo->addItem(relation.value, MapType::propertyToVariant(relation.property));
                if(haveValue && mapPropertyMatches(relation.property, value)){
                    selectedIndex = combo->count() - 1;
                }
            }
            if(haveValue && selectedIndex < 0){
                // The node has a value that isn't in the map - show it rather than hiding it
                combo->addItem(tr("Unknown value (%1)").arg(value), QVariant::fromValue(value));
                selectedIndex = combo->count() - 1;
            }
            if(selectedIndex >= 0){
                combo->setCurrentIndex(selectedIndex);
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
            if(haveValue){
                int intValue = value > static_cast<uint64_t>(std::numeric_limits<int>::max()) ?
                            std::numeric_limits<int>::max() : static_cast<int>(value);
                if(intValue < spin->minimum() || intValue > spin->maximum()){
                    // Show what the node actually has, even if it is out of range
                    LOG4CXX_WARN_FMT(logger, "Value {} for '{}' is outside of range {}-{}",
                                     value, i->name().toStdString(), spin->minimum(), spin->maximum());
                    spin->setRange(std::min(intValue, spin->minimum()), std::max(intValue, spin->maximum()));
                }
                spin->setValue(intValue);
            }else if(i->defaultValue().has_value()){
                spin->setValue(i->defaultValue().value());
            }
            addFormRow(i->name(), i->description(), spin);
        }
    }else if(auto eventType = std::get_if<std::shared_ptr<EventIDType>>(&variant)){
        EventIDType* e = eventType->get();
        QLineEdit* edit = new QLineEdit(ui->fieldsContainer);
        edit->setPlaceholderText(QStringLiteral("00.00.00.00.00.00.00.00"));
        if(haveValue){
            QStringList parts;
            for(uint8_t b : bytes){
                parts.push_back(QStringLiteral("%1").arg(b, 2, 16, QLatin1Char('0')).toUpper());
            }
            edit->setText(parts.join('.'));
        }
        addFormRow(e->name(), e->description(), edit);
    }
}

void CdiEditorWidget::addFormRow(const QString& name, const QString& description, QWidget* field){
    if(!description.isEmpty()){
        field->setToolTip(description);
    }
    ui->fieldsLayout->addRow(name, field);
}

bool CdiEditorWidget::mapPropertyMatches(const MapType::Property& property, uint64_t value){
    if(auto i = std::get_if<int>(&property)){
        return *i >= 0 && static_cast<uint64_t>(*i) == value;
    }else if(auto u = std::get_if<uint64_t>(&property)){
        return *u == value;
    }else if(auto str = std::get_if<QString>(&property)){
        bool ok = false;
        qulonglong parsed = str->toULongLong(&ok, 0);
        return ok && parsed == value;
    }

    return false;
}
