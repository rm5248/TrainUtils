/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETCVPROGRAMMER_H
#define LOCONETCVPROGRAMMER_H

#include <QWidget>
#include <memory>

namespace Ui {
class LoconetCvProgrammer;
}

class LoconetConnection;
class LoconetProgrammer;
class LoconetProgrammingResult;

class LoconetCvProgrammer : public QWidget
{
    Q_OBJECT

public:
    explicit LoconetCvProgrammer(QWidget *parent = nullptr);
    ~LoconetCvProgrammer();

    void setLoconetConnection(std::shared_ptr<LoconetConnection> conn);

private Q_SLOTS:
    void on_readButton_clicked();
    void on_writeButton_clicked();
    void on_modeCombo_currentIndexChanged(int index);
    void programmingComplete();

private:
    void submitRequest(bool isWrite);
    void setBusy(bool busy);

    Ui::LoconetCvProgrammer *ui;
    std::shared_ptr<LoconetConnection> m_conn;
    std::shared_ptr<LoconetProgrammer> m_programmer;
    std::shared_ptr<LoconetProgrammingResult> m_pendingResult;
    bool m_pendingIsRead{false};
};

#endif // LOCONETCVPROGRAMMER_H
