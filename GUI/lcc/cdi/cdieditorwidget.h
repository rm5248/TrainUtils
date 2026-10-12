/* SPDX-License-Identifier: GPL-2.0 */
#ifndef CDIEDITORWIDGET_H
#define CDIEDITORWIDGET_H

#include <QMap>
#include <QSet>
#include <QPair>
#include <QStringList>
#include <QWidget>
#include <memory>

#include "cdi.h"
#include "cdilayout.h"
#include "grouptype.h"
#include "maptype.h"

class QTreeWidgetItem;
class LCCNode;
class SegmentMemory;

namespace Ui {
class CdiEditorWidget;
}

/**
 * Generates a GUI to edit the values described by a CDI.  The CDI either
 * comes from an XML file that the user picks, or from an LCC node (see
 * setLCCNode).  When tied to a node, the values are read from the node's
 * memory, and changed values can be written back to the node.
 */
class CdiEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CdiEditorWidget(QWidget *parent = nullptr);
    ~CdiEditorWidget();

    /**
     * Tie this editor to the given node.  If the CDI for the node has not
     * been read yet, it is read from the node and the editor is populated
     * once the read completes.
     */
    void setLCCNode(std::shared_ptr<LCCNode> node);

protected:
    void showEvent(QShowEvent* event) override;

private Q_SLOTS:
    void on_openButton_clicked();
    void onTreeSelectionChanged();
    void nodeCDIRead();
    void nodeCDIReadFailure(uint16_t error_code, QString error_string);
    void cdiDownloadProgress(uint64_t bytes_rx, uint64_t bytes_total);
    void segmentMemoryReady();
    void segmentMemoryReadFailed(uint16_t error_code, QString message);
    void on_readAllButton_clicked();
    void on_writeButton_clicked();
    void rangeRead(uint32_t address, int length);
    void rangeReadFailed(uint32_t address, int length, uint16_t error_code, QString message);
    void rangeWritten(uint32_t address, int length);
    void rangeWriteFailed(uint32_t address, int length, uint16_t error_code, QString message);

private:
    // A field is identified by its space and address
    typedef QPair<int, uint32_t> FieldKey;

    /**
     * A field that is currently being displayed
     */
    struct FieldBinding {
        CDIVariant variant;
        int space;
        uint32_t address;
        QWidget* widget;
        // The value from the node, as the widget would encode it.  Empty if unknown.
        QVector<uint8_t> original;
    };

    /**
     * A value that the user has changed, that has not been written to the node yet
     */
    struct PendingChange {
        QVector<uint8_t> data;
        QString name;
    };

    void rebuildTree();
    void buildGroupChildren(QTreeWidgetItem* parentItem, const CdiLayoutGroup& group);
    void clearFields();
    void addFieldForVariant(const CDIVariant& variant, int space, uint32_t address);
    QWidget* createFieldWidget(const CDIVariant& variant);
    void loadFieldValue(FieldBinding& binding);
    void refreshFieldValues();
    void setFieldValue(const FieldBinding& binding, const QVector<uint8_t>& bytes);
    bool encodeField(const FieldBinding& binding, QVector<uint8_t>* out);
    void fieldEdited(QWidget* widget);
    void updateHighlight(const FieldBinding& binding);
    void showFieldContextMenu(QWidget* widget, const QPoint& pos);
    FieldBinding* bindingForWidget(QWidget* widget);
    void readField(const FieldBinding& binding);
    void writeChange(const FieldKey& key);
    void sendUpdateComplete();
    void removeWrittenChanges();
    void updateButtons();
    bool isBusy();
    SegmentMemory* segmentForSpace(int space);
    void readConfigurationMemory(bool force = false);
    void readNextSegment();
    void showError(const QString& error);

    /**
     * Get the bytes of memory for the given space/address from the node.
     *
     * @return false if the memory has not been read(yet)
     */
    bool memoryBytes(int space, uint32_t address, int length, QVector<uint8_t>* out);
    static bool mapPropertyMatches(const MapType::Property& property, uint64_t value);
    void addFormRow(const QString& name, const QString& description, QWidget* field);

    Ui::CdiEditorWidget *ui;
    CDI m_cdi;
    // Where everything in m_cdi is stored.  The tree items point into this.
    QVector<CdiLayoutGroup> m_layout;
    std::shared_ptr<LCCNode> m_node;
    QString m_nodeDescription;
    QVector<int> m_segmentsToRead;
    int m_totalSegmentsToRead = 0;
    SegmentMemory* m_currentSegmentRead = nullptr;
    QStringList m_errors;
    bool m_splitterSized = false;
    bool m_forceRead = false;
    QVector<FieldBinding> m_fields;
    QMap<FieldKey, PendingChange> m_pending;
    // Number of writes to the node that have not finished
    int m_writesOutstanding = 0;
    /** Writes that we made, so we can tell them apart from writes made by something else */
    QSet<FieldKey> m_writesInFlight;
};

#endif // CDIEDITORWIDGET_H
