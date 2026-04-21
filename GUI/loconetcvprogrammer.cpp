/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetcvprogrammer.h"
#include "ui_loconetcvprogrammer.h"
#include "loconet/loconetconnection.h"
#include "loconet/loconetprogrammer.h"
#include "loconet/loconetprogrammingresult.h"
#include "loconet/loconetprogrammingrequest.h"

LoconetCvProgrammer::LoconetCvProgrammer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoconetCvProgrammer)
{
    ui->setupUi(this);
}

LoconetCvProgrammer::~LoconetCvProgrammer()
{
    delete ui;
}

void LoconetCvProgrammer::setLoconetConnection(std::shared_ptr<LoconetConnection> conn) {
    m_conn = conn;
    m_programmer = conn->newProgrammer();
}

void LoconetCvProgrammer::on_modeCombo_currentIndexChanged(int index) {
    // Loco address only applies to ops-mode programming
    bool opsMode = (index != 0);
    ui->locoAddressSpin->setEnabled(opsMode);
}

void LoconetCvProgrammer::on_readButton_clicked() {
    submitRequest(false);
}

void LoconetCvProgrammer::on_writeButton_clicked() {
    submitRequest(true);
}

void LoconetCvProgrammer::submitRequest(bool isWrite) {
    if (!m_programmer)
        return;

    LoconetProgrammingRequest::Mode mode;
    switch (ui->modeCombo->currentIndex()) {
    case 1:  mode = LoconetProgrammingRequest::Mode::OpsModeNoFeedback;   break;
    case 2:  mode = LoconetProgrammingRequest::Mode::OpsModeWithFeedback; break;
    default: mode = LoconetProgrammingRequest::Mode::ServiceTrack;        break;
    }

    LoconetProgrammingRequest req;
    req.setMode(mode)
       .setWrite(isWrite)
       .setCv(ui->cvNumberSpin->value())
       .setData(static_cast<uint8_t>(ui->cvValueSpin->value()))
       .setLocoAddress(ui->locoAddressSpin->value());

    m_pendingIsRead = !isWrite;
    m_pendingResult = m_programmer->submitProgrammingRequest(req);
    connect(m_pendingResult.get(), &LoconetProgrammingResult::programmingResultCompleted,
            this, &LoconetCvProgrammer::programmingComplete);

    setBusy(true);
    ui->statusLabel->setText(isWrite ? "Writing..." : "Reading...");
}

void LoconetCvProgrammer::programmingComplete() {
    if (!m_pendingResult)
        return;

    using Reason = LoconetProgrammingResult::CompletedReason;
    switch (m_pendingResult->completedReason()) {
    case Reason::Success:
        if (m_pendingIsRead) {
            ui->cvValueSpin->setValue(m_pendingResult->result());
            ui->statusLabel->setText(QString("Read complete. CV value: %1").arg(m_pendingResult->result()));
        } else {
            ui->statusLabel->setText("Write complete.");
        }
        break;
    case Reason::NoDecoderDetected:
        ui->statusLabel->setText("Error: No decoder detected on programming track.");
        break;
    case Reason::NoWriteAck:
        ui->statusLabel->setText("Error: Decoder did not acknowledge write.");
        break;
    case Reason::FailedToDetectReadAck:
        ui->statusLabel->setText("Error: Failed to detect read acknowledgement.");
        break;
    case Reason::UserAborted:
        ui->statusLabel->setText("Aborted by user.");
        break;
    case Reason::ProgrammerBusy:
        ui->statusLabel->setText("Error: Programmer busy — try again.");
        break;
    case Reason::NotImplemented:
        ui->statusLabel->setText("Error: Operation not supported by command station.");
        break;
    default:
        ui->statusLabel->setText("Unknown result.");
        break;
    }

    m_pendingResult.reset();
    setBusy(false);
}

void LoconetCvProgrammer::setBusy(bool busy) {
    ui->readButton->setEnabled(!busy);
    ui->writeButton->setEnabled(!busy);
    ui->modeCombo->setEnabled(!busy);
    ui->cvNumberSpin->setEnabled(!busy);
    ui->cvValueSpin->setEnabled(!busy);
    if (!busy && ui->modeCombo->currentIndex() != 0)
        ui->locoAddressSpin->setEnabled(true);
    else if (busy)
        ui->locoAddressSpin->setEnabled(false);
}
