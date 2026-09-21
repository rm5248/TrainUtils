/* SPDX-License-Identifier: GPL-2.0 */
#ifndef CDIEDITORWIDGET_H
#define CDIEDITORWIDGET_H

#include <QWidget>

#include "cdi.h"
#include "grouptype.h"

class QTreeWidgetItem;

namespace Ui {
class CdiEditorWidget;
}

/**
 * Lets the user pick a CDI XML file and generates a GUI to edit its
 * values from the parsed structure. Reading/writing the actual node
 * configuration memory is not implemented yet - this only builds the
 * GUI from the XML.
 */
class CdiEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CdiEditorWidget(QWidget *parent = nullptr);
    ~CdiEditorWidget();

private Q_SLOTS:
    void on_openButton_clicked();
    void onTreeSelectionChanged();

private:
    void rebuildTree();
    void buildGroupChildren(QTreeWidgetItem* parentItem, const QVector<CDIVariant>& elements);
    void clearFields();
    void addFieldForVariant(const CDIVariant& variant);
    void addFormRow(const QString& name, const QString& description, QWidget* field);

    Ui::CdiEditorWidget *ui;
    CDI m_cdi;
};

#endif // CDIEDITORWIDGET_H
