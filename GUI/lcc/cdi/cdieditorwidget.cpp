/* SPDX-License-Identifier: GPL-2.0 */
#include "cdieditorwidget.h"
#include "ui_cdieditorwidget.h"

#include <limits>

#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QLineEdit>
#include <QMenu>
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

// Data stored on each tree item: the CdiLayoutGroup(in m_layout) that it shows
static constexpr int GroupRole = Qt::UserRole;

// Background of a field whose value has been changed, but not written to the node
static const char* MODIFIED_FIELD_STYLE = "background-color: #ffb347; color: black;";

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

    // Reading/writing only makes sense when we have a node
    ui->readAllButton->hide();
    ui->writeButton->hide();
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
    ui->readAllButton->show();
    ui->writeButton->show();
    updateButtons();

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

void CdiEditorWidget::readConfigurationMemory(bool force){
    if(!m_node || m_currentSegmentRead){
        // Either no node, or we are already reading
        return;
    }

    m_errors.clear();
    ui->errorLabel->hide();
    m_forceRead = force;

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
        SegmentMemory* mem = segmentForSpace(space);
        if(!mem || (mem->hasMemory() && !m_forceRead)){
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
        // Don't let the user edit values that are about to change
        ui->fieldsContainer->setEnabled(false);
        updateButtons();
        mem->readAllMemory();
        return;
    }

    // Nothing left to read
    ui->cdiProgress->hide();
    ui->filePathLabel->setText(m_nodeDescription);
    ui->fieldsContainer->setEnabled(true);
    refreshFieldValues();
    updateButtons();
}

void CdiEditorWidget::segmentMemoryReady(){
    if(sender() != m_currentSegmentRead){
        return;
    }

    // Show the values that we just read
    refreshFieldValues();
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
    // Any changes refer to the old CDI
    m_pending.clear();
    updateButtons();

    m_layout = cdiLayout(m_cdi);
    for(const CdiLayoutGroup& segment : std::as_const(m_layout)){
        QTreeWidgetItem* segmentItem = new QTreeWidgetItem(ui->cdiTree);
        segmentItem->setText(0, segment.label);
        segmentItem->setData(0, GroupRole, QVariant::fromValue(static_cast<const void*>(&segment)));
        buildGroupChildren(segmentItem, segment);
    }

    ui->cdiTree->expandToDepth(0);
}

void CdiEditorWidget::buildGroupChildren(QTreeWidgetItem* parentItem, const CdiLayoutGroup& group){
    // Values(int/string/eventid) are shown as fields on the right side when
    // their containing group is selected, not as their own tree entries.
    for(const CdiLayoutGroup& child : group.children){
        QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
        item->setText(0, child.label);
        item->setData(0, GroupRole, QVariant::fromValue(static_cast<const void*>(&child)));
        buildGroupChildren(item, child);
    }
}

void CdiEditorWidget::onTreeSelectionChanged(){
    clearFields();

    QList<QTreeWidgetItem*> selected = ui->cdiTree->selectedItems();
    if(selected.isEmpty()){
        return;
    }

    const CdiLayoutGroup* group = static_cast<const CdiLayoutGroup*>(
                selected.first()->data(0, GroupRole).value<const void*>());
    if(!group){
        return;
    }

    for(const CdiField& field : group->fields){
        addFieldForVariant(field.variant, field.space, field.address);
    }
}

void CdiEditorWidget::clearFields(){
    m_fields.clear();
    while(ui->fieldsLayout->rowCount() > 0){
        ui->fieldsLayout->removeRow(0);
    }
}

void CdiEditorWidget::addFieldForVariant(const CDIVariant& variant, int space, uint32_t address){
    QWidget* widget = createFieldWidget(variant);
    if(!widget){
        return;
    }

    widget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(widget, &QWidget::customContextMenuRequested,
            this, [this, widget](const QPoint& pos){
        showFieldContextMenu(widget, pos);
    });

    addFormRow(cdiVariantName(variant), cdiVariantDescription(variant), widget);

    m_fields.push_back(FieldBinding{variant, space, address, widget, {}});
    loadFieldValue(m_fields.last());
}

QWidget* CdiEditorWidget::createFieldWidget(const CDIVariant& variant){
    if(auto strType = std::get_if<std::shared_ptr<StringType>>(&variant)){
        StringType* s = strType->get();
        QLineEdit* edit = new QLineEdit(ui->fieldsContainer);
        if(s->size() > 1){
            // Leave room for the terminating NULL
            edit->setMaxLength(s->size() - 1);
        }
        connect(edit, &QLineEdit::textEdited, this, [this, edit](){ fieldEdited(edit); });
        return edit;
    }else if(auto intType = std::get_if<std::shared_ptr<IntType>>(&variant)){
        IntType* i = intType->get();
        if(i->map().has_value()){
            QComboBox* combo = new QComboBox(ui->fieldsContainer);
            for(const MapType::Relation& relation : i->map()->relations()){
                combo->addItem(relation.value, MapType::propertyToVariant(relation.property));
            }
            connect(combo, &QComboBox::activated, this, [this, combo](){ fieldEdited(combo); });
            return combo;
        }

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
        connect(spin, &QSpinBox::valueChanged, this, [this, spin](){ fieldEdited(spin); });
        return spin;
    }else if(std::holds_alternative<std::shared_ptr<EventIDType>>(variant)){
        QLineEdit* edit = new QLineEdit(ui->fieldsContainer);
        edit->setInputMask(QStringLiteral(">HH.HH.HH.HH.HH.HH.HH.HH;_"));
        connect(edit, &QLineEdit::textEdited, this, [this, edit](){ fieldEdited(edit); });
        return edit;
    }

    return nullptr;
}

void CdiEditorWidget::loadFieldValue(FieldBinding& binding){
    QVector<uint8_t> bytes;
    binding.original.clear();

    if(memoryBytes(binding.space, binding.address, cdiVariantSize(binding.variant), &bytes)){
        setFieldValue(binding, bytes);
        // Encode what is displayed, so that we compare like with like when the user
        // changes the value(e.g. a string may have garbage after its NULL)
        encodeField(binding, &binding.original);
    }

    binding.widget->setEnabled(!m_node || !binding.original.isEmpty());

    auto pending = m_pending.constFind(FieldKey(binding.space, binding.address));
    if(pending != m_pending.constEnd()){
        setFieldValue(binding, pending->data);
    }

    updateHighlight(binding);
}

void CdiEditorWidget::refreshFieldValues(){
    for(FieldBinding& binding : m_fields){
        loadFieldValue(binding);
    }
}

void CdiEditorWidget::setFieldValue(const FieldBinding& binding, const QVector<uint8_t>& bytes){
    const CDIVariant& variant = binding.variant;
    QSignalBlocker blocker(binding.widget);

    if(std::holds_alternative<std::shared_ptr<StringType>>(variant)){
        QLineEdit* edit = static_cast<QLineEdit*>(binding.widget);
        // Strings are null-terminated, unless they fill the entire field
        qsizetype len = bytes.indexOf(0);
        if(len < 0){
            len = bytes.size();
        }
        edit->setText(QString::fromUtf8(reinterpret_cast<const char*>(bytes.constData()), len));
    }else if(auto intType = std::get_if<std::shared_ptr<IntType>>(&variant)){
        IntType* i = intType->get();
        uint64_t value = 0;
        // LCC is big-endian
        for(uint8_t b : bytes){
            value = (value << 8) | b;
        }

        if(QComboBox* combo = qobject_cast<QComboBox*>(binding.widget)){
            int selectedIndex = -1;
            for(int x = 0; x < combo->count(); x++){
                if(x < i->map()->relations().size() &&
                        mapPropertyMatches(i->map()->relations()[x].property, value)){
                    selectedIndex = x;
                    break;
                }
                if(x >= i->map()->relations().size() &&
                        combo->itemData(x).toULongLong() == value){
                    selectedIndex = x;
                    break;
                }
            }
            if(selectedIndex < 0){
                // The node has a value that isn't in the map - show it rather than hiding it
                combo->addItem(tr("Unknown value (%1)").arg(value), QVariant::fromValue(static_cast<qulonglong>(value)));
                selectedIndex = combo->count() - 1;
            }
            combo->setCurrentIndex(selectedIndex);
        }else if(QSpinBox* spin = qobject_cast<QSpinBox*>(binding.widget)){
            int intValue = value > static_cast<uint64_t>(std::numeric_limits<int>::max()) ?
                        std::numeric_limits<int>::max() : static_cast<int>(value);
            if(intValue < spin->minimum() || intValue > spin->maximum()){
                // Show what the node actually has, even if it is out of range
                LOG4CXX_WARN_FMT(logger, "Value {} for '{}' is outside of range {}-{}",
                                 value, i->name().toStdString(), spin->minimum(), spin->maximum());
                spin->setRange(std::min(intValue, spin->minimum()), std::max(intValue, spin->maximum()));
            }
            spin->setValue(intValue);
        }
    }else if(std::holds_alternative<std::shared_ptr<EventIDType>>(variant)){
        QLineEdit* edit = static_cast<QLineEdit*>(binding.widget);
        QStringList parts;
        for(uint8_t b : bytes){
            parts.push_back(QStringLiteral("%1").arg(b, 2, 16, QLatin1Char('0')).toUpper());
        }
        edit->setText(parts.join('.'));
    }
}

bool CdiEditorWidget::encodeField(const FieldBinding& binding, QVector<uint8_t>* out){
    const CDIVariant& variant = binding.variant;
    int size = cdiVariantSize(variant);
    out->clear();

    if(std::holds_alternative<std::shared_ptr<StringType>>(variant)){
        if(size <= 0){
            return false;
        }
        QLineEdit* edit = static_cast<QLineEdit*>(binding.widget);
        QByteArray utf8 = edit->text().toUtf8();
        // Always leave room for the terminating NULL
        utf8.truncate(size - 1);
        for(char c : utf8){
            out->push_back(static_cast<uint8_t>(c));
        }
        // Pad the rest of the field with NULLs
        out->resize(size, 0);
        return true;
    }else if(std::holds_alternative<std::shared_ptr<IntType>>(variant)){
        uint64_t value = 0;
        if(QComboBox* combo = qobject_cast<QComboBox*>(binding.widget)){
            if(combo->currentIndex() < 0){
                return false;
            }
            bool ok = false;
            QVariant data = combo->currentData();
            if(data.typeId() == QMetaType::QString){
                value = data.toString().toULongLong(&ok, 0);
            }else{
                value = data.toULongLong(&ok);
            }
            if(!ok){
                return false;
            }
        }else if(QSpinBox* spin = qobject_cast<QSpinBox*>(binding.widget)){
            value = static_cast<uint64_t>(spin->value());
        }else{
            return false;
        }

        // LCC is big-endian
        for(int x = size - 1; x >= 0; x--){
            out->push_back((value >> (x * 8)) & 0xFF);
        }
        return true;
    }else if(std::holds_alternative<std::shared_ptr<EventIDType>>(variant)){
        QLineEdit* edit = static_cast<QLineEdit*>(binding.widget);
        if(!edit->hasAcceptableInput()){
            return false;
        }
        QByteArray bytes = QByteArray::fromHex(edit->text().remove('.').toLatin1());
        if(bytes.size() != 8){
            return false;
        }
        for(char c : bytes){
            out->push_back(static_cast<uint8_t>(c));
        }
        return true;
    }

    return false;
}

CdiEditorWidget::FieldBinding* CdiEditorWidget::bindingForWidget(QWidget* widget){
    for(FieldBinding& binding : m_fields){
        if(binding.widget == widget){
            return &binding;
        }
    }
    return nullptr;
}

void CdiEditorWidget::fieldEdited(QWidget* widget){
    FieldBinding* binding = bindingForWidget(widget);
    if(!binding || binding->original.isEmpty()){
        // We don't know what the node has, so there's nothing to compare against
        return;
    }

    FieldKey key(binding->space, binding->address);
    QVector<uint8_t> bytes;
    if(encodeField(*binding, &bytes) && bytes != binding->original){
        QString name = cdiVariantName(binding->variant);
        if(name.isEmpty()){
            name = tr("space 0x%1 address %2").arg(binding->space, 2, 16, QLatin1Char('0')).arg(binding->address);
        }
        m_pending[key] = PendingChange{bytes, name};
    }else{
        // Either back to the original value, or not a valid value(yet)
        m_pending.remove(key);
    }

    updateHighlight(*binding);
    updateButtons();
}

void CdiEditorWidget::updateHighlight(const FieldBinding& binding){
    bool modified = m_pending.contains(FieldKey(binding.space, binding.address));
    binding.widget->setStyleSheet(modified ? MODIFIED_FIELD_STYLE : "");
}

void CdiEditorWidget::showFieldContextMenu(QWidget* widget, const QPoint& pos){
    FieldBinding* binding = bindingForWidget(widget);
    if(!binding || !m_node){
        return;
    }

    FieldBinding fieldCopy = *binding;
    FieldKey key(binding->space, binding->address);

    // Keep the normal cut/copy/paste entries for text fields
    QLineEdit* edit = qobject_cast<QLineEdit*>(widget);
    QMenu* menu = edit ? edit->createStandardContextMenu() : new QMenu(widget);
    QAction* first = menu->actions().isEmpty() ? nullptr : menu->actions().first();

    QAction* readAction = new QAction(tr("Read value"), menu);
    readAction->setEnabled(!isBusy());
    connect(readAction, &QAction::triggered, this, [this, fieldCopy](){ readField(fieldCopy); });

    QAction* writeAction = new QAction(tr("Write value"), menu);
    writeAction->setEnabled(!isBusy() && m_pending.contains(key));
    connect(writeAction, &QAction::triggered, this, [this, key](){ writeChange(key); });

    menu->insertAction(first, readAction);
    menu->insertAction(first, writeAction);
    if(first){
        menu->insertSeparator(first);
    }

    menu->exec(widget->mapToGlobal(pos));
    delete menu;
}

SegmentMemory* CdiEditorWidget::segmentForSpace(int space){
    if(!m_node){
        return nullptr;
    }

    SegmentMemory* mem = m_node->segmentMemory(space);
    if(mem){
        connect(mem, &SegmentMemory::rangeRead,
                this, &CdiEditorWidget::rangeRead, Qt::UniqueConnection);
        connect(mem, &SegmentMemory::rangeReadFailed,
                this, &CdiEditorWidget::rangeReadFailed, Qt::UniqueConnection);
        connect(mem, &SegmentMemory::rangeWritten,
                this, &CdiEditorWidget::rangeWritten, Qt::UniqueConnection);
        connect(mem, &SegmentMemory::rangeWriteFailed,
                this, &CdiEditorWidget::rangeWriteFailed, Qt::UniqueConnection);
    }
    return mem;
}

void CdiEditorWidget::readField(const FieldBinding& binding){
    SegmentMemory* mem = segmentForSpace(binding.space);
    if(!mem || !mem->readRange(binding.address, cdiVariantSize(binding.variant))){
        showError(tr("Unable to read '%1'").arg(cdiVariantName(binding.variant)));
    }
}

void CdiEditorWidget::rangeRead(uint32_t address, int length){
    SegmentMemory* mem = qobject_cast<SegmentMemory*>(sender());
    if(!mem){
        return;
    }

    // Whatever the user had changed in this range has been replaced by what the node has
    for(auto it = m_pending.begin(); it != m_pending.end(); ){
        if(it.key().first == mem->segmentId() &&
                it.key().second >= address &&
                it.key().second < address + length){
            it = m_pending.erase(it);
        }else{
            ++it;
        }
    }

    refreshFieldValues();
    updateButtons();
}

void CdiEditorWidget::rangeReadFailed(uint32_t address, int length, uint16_t error_code, QString message){
    showError(tr("Unable to read address %1: %2").arg(address).arg(message));
}

void CdiEditorWidget::writeChange(const FieldKey& key){
    auto pending = m_pending.constFind(key);
    if(pending == m_pending.constEnd()){
        return;
    }

    SegmentMemory* mem = segmentForSpace(key.first);
    if(!mem || !mem->writeRange(key.second, pending->data)){
        showError(tr("Unable to write '%1'").arg(pending->name));
        return;
    }

    m_writesOutstanding++;
    ui->filePathLabel->setText(tr("Writing %n value(s)...", "", m_writesOutstanding));
    updateButtons();
}

void CdiEditorWidget::on_writeButton_clicked(){
    m_errors.clear();
    ui->errorLabel->hide();

    const QList<FieldKey> keys = m_pending.keys();
    for(const FieldKey& key : keys){
        writeChange(key);
    }
}

void CdiEditorWidget::removeWrittenChanges(){
    // Anything that the node now has(our copy of the memory is updated as writes
    // succeed) is no longer a change
    for(auto it = m_pending.begin(); it != m_pending.end(); ){
        QVector<uint8_t> current;
        if(memoryBytes(it.key().first, it.key().second, it->data.size(), &current) &&
                current == it->data){
            it = m_pending.erase(it);
        }else{
            ++it;
        }
    }
}

void CdiEditorWidget::rangeWritten(uint32_t address, int length){
    m_writesOutstanding = std::max(0, m_writesOutstanding - 1);
    removeWrittenChanges();
    refreshFieldValues();

    if(m_writesOutstanding == 0){
        ui->filePathLabel->setText(m_nodeDescription);
    }else{
        ui->filePathLabel->setText(tr("Writing %n value(s)...", "", m_writesOutstanding));
    }
    updateButtons();
}

void CdiEditorWidget::rangeWriteFailed(uint32_t address, int length, uint16_t error_code, QString message){
    m_writesOutstanding = std::max(0, m_writesOutstanding - 1);

    SegmentMemory* mem = qobject_cast<SegmentMemory*>(sender());
    QString name = tr("address %1").arg(address);
    if(mem){
        auto pending = m_pending.constFind(FieldKey(mem->segmentId(), address));
        if(pending != m_pending.constEnd()){
            name = QStringLiteral("'%1'").arg(pending->name);
        }
    }
    showError(tr("Unable to write %1: %2").arg(name).arg(message));

    // Part of the value may have been written
    removeWrittenChanges();
    refreshFieldValues();

    if(m_writesOutstanding == 0){
        ui->filePathLabel->setText(m_nodeDescription);
    }
    updateButtons();
}

void CdiEditorWidget::on_readAllButton_clicked(){
    if(!m_pending.isEmpty()){
        QMessageBox::StandardButton answer =
                QMessageBox::question(this, tr("Re-read all values"),
                                      tr("There are %n value(s) that have not been written to the node.  "
                                         "Re-reading will discard these changes.  Continue?", "", m_pending.size()));
        if(answer != QMessageBox::Yes){
            return;
        }
    }

    m_pending.clear();
    readConfigurationMemory(true);
    refreshFieldValues();
    updateButtons();
}

bool CdiEditorWidget::isBusy(){
    return m_currentSegmentRead != nullptr || m_writesOutstanding > 0;
}

void CdiEditorWidget::updateButtons(){
    ui->readAllButton->setEnabled(m_node && !isBusy());
    ui->writeButton->setEnabled(m_node && !isBusy() && !m_pending.isEmpty());
    ui->writeButton->setText(m_pending.isEmpty() ?
                                 tr("Write changes") :
                                 tr("Write changes (%1)").arg(m_pending.size()));
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
