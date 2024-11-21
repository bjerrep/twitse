#include "measurementsequenser.h"
#include "device.h"
#include "log.h"


/* Try to get a hold on the maximum number of client which may concurrently perform measurements.
 * Coldstarting where all clients are trying to get in sync is the problematic case. Too many clients
 * (seen the issue with 4 clients) running concurrently has been observed to saturate the wifi conncections
 * on individual clients which make the measurements end up beeing rejected.
 * This class hooks into the measurement start request and measurement finalize in the clients and
 * place clients in a pending queue if they should be started later due to too many ongoing client
 * measurements.
  */
MeasurementSequencer::MeasurementSequencer(QObject* parent)
    : QObject(parent)
{
}


/* Mostly an development convinience, called when all clients have disapeared (i.e. killed to simulate a coldstart)
 */
void MeasurementSequencer::reset()
{
    m_maxConcurrentFilter = MAX_CONCURRENT_FILTER;
}

/* The allowed concurrent measurements is derived from the m_maxConcurrentFilter up/down counter.
 * The actual value for concurrent measurements will be in the range [1,MAX_CONCURRENT_MEASUREMENTS]
 * The ideal value will be 1 meaning that only one measurement is in progress in any given time which
 * should intuitively give the best measurements.
 */
int MeasurementSequencer::calcConcurrentMeasurements() const
{
    return 1 + (m_maxConcurrentFilter / FILTER_STEPS);
}


void MeasurementSequencer::overloaded()
{
    m_maxConcurrentFilter = std::min(MAX_CONCURRENT_FILTER, m_maxConcurrentFilter + 1);
}


void MeasurementSequencer::notOverloaded()
{
    m_maxConcurrentFilter = std::max(0, m_maxConcurrentFilter - 1);
}


/* Note that for the sequencing to work it requires that a measurement start is always followed
 * by a measurement finalize no matter what might have gone wrong in either.
 */
void MeasurementSequencer::slotRequestMeasurementStart(Device *device)
{
    QMutexLocker lock(&m_mutex);

    int maxConcurrent = calcConcurrentMeasurements();

    if (m_activeDevices.size() < maxConcurrent)
    {
        m_activeDevices.append(device);
        device->measurementStart();
    }
    else
    {
        trace->debug(YELLOW "measurement start request queued for {}" RESET, device->name());
        m_pendingDevices.append(device);
    }
}


void MeasurementSequencer::slotMeasurementFinalized(Device *device)
{
    QMutexLocker lock(&m_mutex);

    int index = m_activeDevices.indexOf(device);

    if (device && index >= 0)
    {
        m_activeDevices.remove(index);
    }

    if (!m_pendingDevices.size())
    {
        notOverloaded();
        return;
    }

    int maxConcurrent = calcConcurrentMeasurements();

    if (m_activeDevices.size() + m_pendingDevices.size() < maxConcurrent)
    {
        notOverloaded();
    }
    else
    {
        overloaded();
    }

    maxConcurrent = calcConcurrentMeasurements();

    if (m_activeDevices.size() < maxConcurrent)
    {
        Device* next = m_pendingDevices.takeFirst();
        m_activeDevices.append(next);
        trace->debug(YELLOW "starting pending {}" RESET, next->name());
        next->measurementStart();
    }
    if (m_pendingDevices.size() > 5)
    {
        // suspect that the start/finalizing sequencing is actually broken and issue a warning
        trace->warn("measurement finalized, active={} pending={} maxConcurrent={}", m_activeDevices.size(), m_pendingDevices.size(), maxConcurrent );
    }
    else
    {
        trace->info(YELLOW "measurement finalized, active={} pending={} maxConcurrent={}" RESET, m_activeDevices.size(), m_pendingDevices.size(), maxConcurrent );
    }
}
