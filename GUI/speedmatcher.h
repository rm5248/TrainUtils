/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SPEEDMATCHER_H
#define SPEEDMATCHER_H

#include <QWidget>
#include <QTimer>
#include <memory>
#include <array>

#include "trainutils_state.h"

namespace Ui { class SpeedMatcher; }

class LoconetConnection;
class LoconetThrottle;
class LoconetProgrammer;
class SpeedoConnection;

class SpeedMatcher : public QWidget
{
    Q_OBJECT
public:
    explicit SpeedMatcher(TrainUtilsState* state,
                          QWidget* parent = nullptr);
    ~SpeedMatcher();

private Q_SLOTS:
    void onStartClicked();
    void onSpeedUpdated(int kph);
    void onStabilizationTick();
    void onPhaseTimerExpired();

    void on_speedoSelector_activated(int index);

    void on_dccSelector_activated(int index);

    void on_locoAddressEdit_textChanged(const QString &arg1);

    void on_abortButton_clicked();

private:
    void checkIfReady();
    void beginMeasurement();
    void startStep(int step);   // step 1-based
    void recordCurrentStep();
    void beginProgramming();
    void finishMatching();
    void setStatus(const QString& msg);

    // Returns the target speed in km/h for the given 1-based step.
    int targetSpeedKph(int step) const;

    // Returns the CV value (0-255) to write for CV 67+(step-1).
    int computeCVValue(int step) const;

    enum class State {
        Idle,
        AcquiringThrottle,
        Stabilizing,
        StoppingLoco,
    };

    static constexpr int NUM_STEPS       = 28;
    static constexpr int STABLE_REQUIRED = 3;    // consecutive stable ticks
    static constexpr int STABLE_TOLERANCE= 2;    // km/h window considered stable
    static constexpr int TICK_MS         = 500;  // stabilization poll interval
    static constexpr int MAX_WAIT_TICKS  = 30;   // 15 s max per step before giving up
    static constexpr int PHASE_DELAY_MS  = 1500; // delay after throttle acquire / loco stop

    Ui::SpeedMatcher* ui;
    std::shared_ptr<LoconetConnection> m_loconet;
    std::shared_ptr<SpeedoConnection> m_speedo;

    std::shared_ptr<LoconetThrottle>   m_throttle;
    std::shared_ptr<LoconetProgrammer> m_programmer;

    State m_state{State::Idle};
    int   m_currentStep{0};  // 1-based step currently being measured
    std::array<int, NUM_STEPS> m_measuredKph{};

    QTimer m_stabilizationTimer;   // fires every TICK_MS during Stabilizing
    QTimer m_phaseTimer;           // one-shot delay between phases

    int m_currentSpeedKph{0};  // latest reading from SpeedoConnection
    int m_lastCheckedKph{-1};
    int m_stableCount{0};
    int m_waitTicks{0};
    TrainUtilsState* m_trainUtilsState = nullptr;
};

#endif // SPEEDMATCHER_H
