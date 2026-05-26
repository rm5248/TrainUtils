/* SPDX-License-Identifier: GPL-2.0 */
#include "speedmatcher.h"
#include "ui_speedmatcher.h"

#include "loconet/loconetconnection.h"
#include "loconet/loconetthrottle.h"
#include "loconet/loconetprogrammer.h"
#include "loconet/loconetprogrammingrequest.h"
#include "loconet/loconetprogrammingresult.h"
#include "speedo/speedoconnection.h"

#include <cmath>
#include <algorithm>

static constexpr double MPH_TO_KPH = 1.60934;

// CV 67 is the first extended speed table entry (speed step 1).
static constexpr int CV_SPEED_TABLE_BASE = 67;

SpeedMatcher::SpeedMatcher(std::shared_ptr<LoconetConnection> loconet,
                           std::shared_ptr<SpeedoConnection>  speedo,
                           QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::SpeedMatcher)
    , m_loconet(loconet)
    , m_speedo(speedo)
{
    ui->setupUi(this);

    connect(ui->startButton, &QPushButton::clicked,
            this, &SpeedMatcher::onStartClicked);
    connect(m_speedo.get(), &SpeedoConnection::speedUpdated,
            this, &SpeedMatcher::onSpeedUpdated);

    m_stabilizationTimer.setInterval(TICK_MS);
    m_stabilizationTimer.setSingleShot(false);
    connect(&m_stabilizationTimer, &QTimer::timeout,
            this, &SpeedMatcher::onStabilizationTick);

    m_phaseTimer.setSingleShot(true);
    connect(&m_phaseTimer, &QTimer::timeout,
            this, &SpeedMatcher::onPhaseTimerExpired);

    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(NUM_STEPS);
}

SpeedMatcher::~SpeedMatcher()
{
    delete ui;
}

// ─── slots ───────────────────────────────────────────────────────────────────

void SpeedMatcher::onStartClicked()
{
    if (m_state != State::Idle)
        return;

    int address = ui->locoAddressEdit->text().toInt();
    if (address <= 0) {
        setStatus("Enter a valid DCC address.");
        return;
    }

    m_throttle  = m_loconet->newThrottle();
    m_programmer = m_loconet->newProgrammer();

    bool isLong = (address >= 128);
    m_throttle->selectLocomotive(address, isLong);
    m_throttle->setDirection(LocomotiveDirection::Forward);

    m_state = State::AcquiringThrottle;
    ui->startButton->setEnabled(false);
    setStatus("Acquiring throttle…");

    // Give the command station time to dispatch the loco before we command speeds.
    m_phaseTimer.start(PHASE_DELAY_MS);
}

void SpeedMatcher::onSpeedUpdated(int kph)
{
    m_currentSpeedKph = kph;
}

void SpeedMatcher::onStabilizationTick()
{
    ++m_waitTicks;

    int diff = std::abs(m_currentSpeedKph - m_lastCheckedKph);
    if (diff <= STABLE_TOLERANCE) {
        ++m_stableCount;
    } else {
        m_stableCount = 0;
        m_lastCheckedKph = m_currentSpeedKph;
    }

    bool stable   = (m_stableCount >= STABLE_REQUIRED);
    bool timedOut = (m_waitTicks >= MAX_WAIT_TICKS);

    if (stable || timedOut) {
        m_stabilizationTimer.stop();
        recordCurrentStep();
    }
}

void SpeedMatcher::onPhaseTimerExpired()
{
    switch (m_state) {
    case State::AcquiringThrottle:
        beginMeasurement();
        break;
    case State::StoppingLoco:
        beginProgramming();
        break;
    default:
        break;
    }
}

// ─── measurement phase ───────────────────────────────────────────────────────

void SpeedMatcher::beginMeasurement()
{
    m_currentStep = 0;
    m_measuredKph.fill(0);
    ui->progressBar->setValue(0);
    startStep(1);
}

void SpeedMatcher::startStep(int step)
{
    m_currentStep    = step;
    m_state          = State::Stabilizing;
    m_stableCount    = 0;
    m_waitTicks      = 0;
    m_lastCheckedKph = -1;

    // Map step 1-28 to throttle percentage 0-100.
    int pct = static_cast<int>(std::round(step * 100.0 / NUM_STEPS));
    m_throttle->setSpeed(pct);

    setStatus(QString("Measuring step %1 / %2…").arg(step).arg(NUM_STEPS));
    m_stabilizationTimer.start();
}

void SpeedMatcher::recordCurrentStep()
{
    m_measuredKph[m_currentStep - 1] = m_currentSpeedKph;
    ui->progressBar->setValue(m_currentStep);

    if (m_currentStep < NUM_STEPS) {
        startStep(m_currentStep + 1);
    } else {
        // All steps measured — stop the locomotive.
        m_state = State::StoppingLoco;
        m_throttle->setSpeed(0);
        setStatus("Measurement complete. Stopping locomotive…");
        m_phaseTimer.start(PHASE_DELAY_MS);
    }
}

// ─── programming phase ────────────────────────────────────────────────────────

void SpeedMatcher::beginProgramming()
{
    if (m_measuredKph[NUM_STEPS - 1] <= 0) {
        setStatus("Error: no speed reading at full throttle. Aborting.");
        finishMatching();
        return;
    }

    setStatus("Writing CV67–94…");
    ui->progressBar->setValue(0);

    int address = ui->locoAddressEdit->text().toInt();

    // Submit all 28 CV writes. Ops-mode no-feedback; they are queued and sent
    // back-to-back — no waiting required between them.
    int completedCount = 0;
    for (int step = 1; step <= NUM_STEPS; ++step) {
        int cvNum   = CV_SPEED_TABLE_BASE + (step - 1);
        int cvValue = computeCVValue(step);

        LoconetProgrammingRequest req;
        req.setMode(LoconetProgrammingRequest::Mode::OpsModeNoFeedback)
           .setWrite(true)
           .setCv(cvNum)
           .setData(static_cast<uint8_t>(cvValue))
           .setLocoAddress(address);

        auto result = m_programmer->submitProgrammingRequest(req);

        // Capture by value; use a shared counter to update progress once all done.
        connect(result.get(), &LoconetProgrammingResult::programmingResultCompleted,
                this, [this, result, step]() {
                    ui->progressBar->setValue(step);
                    if (step == NUM_STEPS)
                        finishMatching();
                });
    }
}

void SpeedMatcher::finishMatching()
{
    if (m_throttle) {
        m_throttle->releaseLocomotive();
        m_throttle.reset();
    }
    m_programmer.reset();

    m_state = State::Idle;
    ui->startButton->setEnabled(true);
    ui->progressBar->setValue(NUM_STEPS);
    setStatus("Speed matching complete.");
}

// ─── helpers ─────────────────────────────────────────────────────────────────

int SpeedMatcher::targetSpeedKph(int step) const
{
    double maxKph = ui->maxSpeedSpin->value();
    if (ui->mphRadio->isChecked())
        maxKph *= MPH_TO_KPH;
    return static_cast<int>(std::round(step * maxKph / NUM_STEPS));
}

int SpeedMatcher::computeCVValue(int step) const
{
    // Scale a fresh linear 0–255 table so that step 28 produces the user's
    // requested max speed.  The ratio (targetMax / measuredMax) corrects the
    // overall speed of the locomotive; the n/28 factor distributes evenly
    // across the extended speed table.
    double measuredMaxKph = static_cast<double>(m_measuredKph[NUM_STEPS - 1]);
    if (measuredMaxKph <= 0.0)
        return 1;

    double targetMaxKph = static_cast<double>(targetSpeedKph(NUM_STEPS));
    double cv = 255.0 * (step / static_cast<double>(NUM_STEPS))
                      * (targetMaxKph / measuredMaxKph);
    return std::clamp(static_cast<int>(std::round(cv)), 1, 255);
}

void SpeedMatcher::setStatus(const QString& msg)
{
    ui->statusLabel->setText(msg);
}
