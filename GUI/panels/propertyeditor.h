#ifndef PROPERTYEDITOR_H
#define PROPERTYEDITOR_H

#include <QWidget>
#include <QMetaProperty>

namespace Ui {
class PropertyEditor;
}

class PropertyEditor : public QWidget
{
    Q_OBJECT

public:
    explicit PropertyEditor(QWidget *parent, QObject* modifyObj, QMetaProperty prop);
    ~PropertyEditor();

private Q_SLOTS:
    void on_comboBox_activated(int index);

private:
    Ui::PropertyEditor *ui;
    QObject* m_objToModify;
    QMetaProperty m_objProperty;
};

#endif // PROPERTYEDITOR_H
