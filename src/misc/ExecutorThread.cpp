#include <misc/ExecutorThread.h>

wxDEFINE_EVENT(wxEVT_THREAD_STDIN, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_THREAD_STDERR, wxThreadEvent);

ExecutorThread::ExecutorThread(wxEvtHandler* h, wxProcess* p,
    wxMessageQueue<ThreadMessage>& q)
    :wxThread(wxTHREAD_JOINABLE), m_queue(q)
{
    m_process = p;
    m_handler = h;
    // Small reads let stdout and stderr alternate instead of draining a large
    // burst from one pipe while the other (often carrying compiler errors)
    // waits behind it.
    m_bufferSize = 4096;
    m_buffer = new char[m_bufferSize];
}

ExecutorThread::~ExecutorThread()
{
    delete[] m_buffer;
    delete m_process;
}

wxThread::ExitCode ExecutorThread::Entry()
{
    ExitCode c;

    while (1)
    {
        // Check if termination was requested.
        if (TestDestroy())
        {
            wxProcess::Kill(m_process->GetPid());
            c = reinterpret_cast<ExitCode>(1);
            break;
        }

        ThreadMessage m = MessageLast;
        wxMessageQueueError e = m_queue.ReceiveTimeout(10, m);

        // Check if a message was received or we timed out.
        if (e == wxMSGQUEUE_NO_ERROR)
        {
            if (m == ProcessComplete)
            {
                DrainInput();
                c = reinterpret_cast<ExitCode>(0);
                break;

            }
            else if (m == ExitThread)
            {
                wxProcess::Kill(m_process->GetPid());
                c = reinterpret_cast<ExitCode>(1);
                break;
            }
        }
        else if (e == wxMSGQUEUE_TIMEOUT)
        {
            DrainInput();
        }
    }

    return c;
}

void ExecutorThread::DrainInput()
{
    auto queue_output = [this](wxEventType type, wxInputStream* stream)
    {
        stream->Read(m_buffer, m_bufferSize);
        if (stream->LastRead() == 0)
        {
            return;
        }
        wxThreadEvent* event = new wxThreadEvent(type);
        event->SetString(wxString(m_buffer, stream->LastRead()));
        m_handler->QueueEvent(event);
    };

    for (;;)
    {
        bool read_any = false;
        if (m_process->IsInputAvailable())
        {
            queue_output(wxEVT_THREAD_STDIN, m_process->GetInputStream());
            read_any = true;
        }
        if (m_process->IsErrorAvailable())
        {
            queue_output(wxEVT_THREAD_STDERR, m_process->GetErrorStream());
            read_any = true;
        }
        if (!read_any)
        {
            break;
        }
    }
}
