#include <ExecutorThread.h>

wxDEFINE_EVENT(wxEVT_THREAD_STDIN, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_THREAD_STDERR, wxThreadEvent);

ExecutorThread::ExecutorThread(wxEvtHandler* h, wxProcess* p,
    wxMessageQueue<ThreadMessage>& q)
    :wxThread(wxTHREAD_JOINABLE), m_queue(q)
{
    m_process = p;
    m_handler = h;
    m_bufferSize = 1024 * 1024;
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
    if (!m_process->IsInputOpened())
    {
        return;
    }

    wxString fromInputStream, fromErrorStream;
    wxInputStream* stream;

    while (m_process->IsInputAvailable())
    {
        stream = m_process->GetInputStream();
        stream->Read(m_buffer, m_bufferSize);
        fromInputStream << wxString(m_buffer, stream->LastRead());
    }

    while (m_process->IsErrorAvailable())
    {
        stream = m_process->GetErrorStream();
        stream->Read(m_buffer, m_bufferSize);
        fromErrorStream << wxString(m_buffer, stream->LastRead());
    }

    if (!fromInputStream.IsEmpty())
    {
        wxThreadEvent* event = new wxThreadEvent(wxEVT_THREAD_STDIN);
        event->SetString(fromInputStream);
        m_handler->QueueEvent(event);
    }

    if (!fromErrorStream.IsEmpty())
    {
        wxThreadEvent* event = new wxThreadEvent(wxEVT_THREAD_STDERR);
        event->SetString(fromErrorStream);
        m_handler->QueueEvent(event);
    }
}
