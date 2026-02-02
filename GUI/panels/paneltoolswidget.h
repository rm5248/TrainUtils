#ifndef PANELTOOLSWIDGET_H
#define PANELTOOLSWIDGET_H

#include <QWidget>

namespace Ui {
class PanelToolsWidget;
}

class PanelToolsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PanelToolsWidget(QWidget *parent = nullptr);
    ~PanelToolsWidget();

    /**
     * Set the currently selected widget for panel editing.
     * @param widget
     */
    void setCurrentSelectedWidget(QWidget* widget);

private:
    Ui::PanelToolsWidget *ui;
    QVector<QWidget*> m_currentProps;
};

#endif // PANELTOOLSWIDGET_H
