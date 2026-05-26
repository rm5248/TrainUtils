#ifndef SPEEDMATCHER_H
#define SPEEDMATCHER_H

#include <QWidget>

namespace Ui {
class SpeedMatcher;
}

class SpeedMatcher : public QWidget
{
    Q_OBJECT

public:
    explicit SpeedMatcher(QWidget *parent = nullptr);
    ~SpeedMatcher();

private:
    Ui::SpeedMatcher *ui;
};

#endif // SPEEDMATCHER_H
