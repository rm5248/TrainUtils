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

Q_SIGNALS:
    void allowMovingChanged(bool new_moving);
    void addDCCTurnout();
    void drawConnectionPointsChanged(bool new_draw_connections);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    // void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private Q_SLOTS:
    void on_addTurnoutButton_clicked();

private:
    Ui::PanelToolsWidget *ui;
    QVector<QWidget*> m_currentProps;
};

#endif // PANELTOOLSWIDGET_H
