/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCNETWORKVIEW_H
#define LCCNETWORKVIEW_H

#include <QWidget>
#include <memory>

namespace Ui {
class LCCNetworkView;
}

class LCCConnection;

class LCCNetworkView : public QWidget
{
    Q_OBJECT

public:
    explicit LCCNetworkView(QWidget *parent = nullptr);
    ~LCCNetworkView();

    void setLCCConnection(std::shared_ptr<LCCConnection> lcc);

private:
    Ui::LCCNetworkView *ui;
    std::shared_ptr<LCCConnection> m_connection;
};

#endif // LCCNETWORKVIEW_H
