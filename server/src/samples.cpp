#include "samples.h"
#include "multicast.h"
#include "log.h"
#include "systemtime.h"
#include "apputils.h"
#include "globals.h"

extern int g_developmentMask;


ClientSampleRun::ClientSampleRun(const QString &client, int count)
    : m_name(client),
      m_count(count)
{
    m_startTime = s_systemTime->getRunningTime_secs();
}

// -------------------------------------------


void Samples::setPeriod(int period_ms)
{
    m_period_ms = period_ms;
}


void Samples::timerEvent(QTimerEvent *)
{
    if (m_sampleRuns.size())
    {
        if (++m_client_it == m_sampleRuns.end())
        {
            m_client_it = m_sampleRuns.begin();
        }

        if ((*m_client_it)->m_count == 0)
        {
            trace->debug("{} sample run completed in {:.1f} secs",
                         (*m_client_it)->m_name.toStdString(),
                         s_systemTime->getRunningTime_secs() - (*m_client_it)->m_startTime);
            emit signalSampleRunStatusUpdate((*m_client_it)->m_name, false);
            if (m_sampleRuns.contains(*m_client_it))
            {
                delete (*m_client_it);
                m_sampleRuns.erase(m_client_it);
            }
            m_client_it = m_sampleRuns.begin();
        }
        else
        {
            (*m_client_it)->m_count--;
            emit signalSendTimeSample((*m_client_it)->m_name);
        }
    }

    if (m_sampleRuns.empty())
    {
        timerOff(this, m_timerId);
    }
    else
    {
        if (!develPeriodSweep)
        {
            if (qrand() % 10 == 0)
            {
                int skewrange = m_period_ms / 2;
                skewrange += !(skewrange % 2);
                int skew = (qrand() % skewrange) - skewrange / 2;
                timerOff(this, m_timerId);
                m_timerId = startTimer(m_period_ms + skew, Qt::PreciseTimer);
            }
        }
    }
}


void Samples::removeClient(const QString& clientname)
{
    for(QList<ClientSampleRun*>::Iterator it = m_sampleRuns.begin(); it != m_sampleRuns.end(); it++)
    {
        if ((*it)->m_name == clientname)
        {
            if (m_sampleRuns.contains(*it))
            {
                delete (*it);
                m_sampleRuns.erase(it);
            }
            m_client_it = m_sampleRuns.begin();
            emit signalSampleRunStatusUpdate(clientname, false);
            return;
        }
    }
    trace->debug("nothing to remove for client '{}'", clientname.toStdString());
}


void Samples::slotRequestSamples(const QString& clientname, int count, int period_ms)
{
    m_period_ms = period_ms;
    timerOff(this, m_timerId);
    m_timerId = startTimer(m_period_ms, Qt::PreciseTimer);

    m_sampleRuns.append(new ClientSampleRun(clientname, count));
    m_client_it = m_sampleRuns.begin();
    emit signalSampleRunStatusUpdate((*m_client_it)->m_name, true);
}
