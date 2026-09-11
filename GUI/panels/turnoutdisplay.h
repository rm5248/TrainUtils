#ifndef TURNOUTDISPLAY_H
#define TURNOUTDISPLAY_H

#include <QWidget>
#include <QDateTime>
#include <QRect>

#include "../common/turnout.h"
#include "connectable.h"

class TurnoutDisplay : public QWidget, public Connectable
{
    Q_OBJECT
    // Q_PROPERTY(QString traingui_turnout READ turnout WRITE setTurnout)
public:
    enum class TurnoutType{
        Left,
        Right
    };
    Q_ENUM(TurnoutType)
    Q_PROPERTY(TurnoutType traingui_turnout_type READ turnoutType WRITE setTurnoutType)
    Q_PROPERTY(QString traingui_name READ name WRITE setName NOTIFY nameChanged)

    explicit TurnoutDisplay(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    void setTurnout(std::shared_ptr<Turnout> turnout);
    TurnoutType turnoutType();

    QString name() const;

    double rotation() const;

    // The turnout artwork is drawn centered within a larger, fixed-size
    // widget so that rotating it never clips it against the widget bounds.
    // This is that inner drawing area, in widget-local (unrotated) coordinates.
    QRect contentRect() const;

    QVector<QPoint> connectionPoints() override;

Q_SIGNALS:
    void connectionPointsUpdated();
    void nameChanged();

public Q_SLOTS:
    void configureInteraction(bool interaction);
    void setTurnoutType(TurnoutType type);
    void setName(QString name);
    void setRotation(double degrees);

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent* event) override;
    // void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void stateChanged();

private:
    // Size of the actual turnout artwork.
    static constexpr int kContentWidth = 50;
    static constexpr int kContentHeight = 50;
    // Size of the widget itself: large enough that the content, rotated by
    // any angle, still fits (the diagonal of a 50x50 square is ~71px).
    static constexpr int kBoundingSize = 72;

    std::shared_ptr<Turnout> m_turnout;
    bool m_interactive = true;
    QDateTime m_mousePressStart;
    QPoint m_mousePressLocation;
    TurnoutType m_turnoutType = TurnoutType::Right;
    QVector<QPoint> m_connectionPoints;
    bool m_updateConnectionPoints = true;
    QString m_name;
    double m_rotation = 0.0;
};

#endif // TURNOUTDISPLAY_H
