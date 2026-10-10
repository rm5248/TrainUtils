/* SPDX-License-Identifier: GPL-2.0 */
#ifndef CDIEDITORWIDGET_H
#define CDIEDITORWIDGET_H

#include <QStringList>
#include <QWidget>
#include <memory>

#include "cdi.h"
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
 * setLCCNode).  Reading/writing the actual node configuration memory is
 * not implemented yet - this only builds the GUI from the XML.
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

private:
    void rebuildTree();
    void buildGroupChildren(QTreeWidgetItem* parentItem, const QVector<CDIVariant>& elements, int space, uint32_t address);
    void clearFields();
    void addFieldForVariant(const CDIVariant& variant, int space, uint32_t address);
    void readConfigurationMemory();
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
    std::shared_ptr<LCCNode> m_node;
    QString m_nodeDescription;
    QVector<int> m_segmentsToRead;
    int m_totalSegmentsToRead = 0;
    SegmentMemory* m_currentSegmentRead = nullptr;
    QStringList m_errors;
    bool m_splitterSized = false;
};

#endif // CDIEDITORWIDGET_H
