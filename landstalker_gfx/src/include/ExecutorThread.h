#ifndef _EXECUTOR_THREAD_H_
#define _EXECUTOR_THREAD_H_

#include <wx/process.h>
#include <wx/thread.h>
#include <wx/msgqueue.h>

class ExecutorThread : public wxThread
{
public:
    enum ThreadMessage
    {
        ProcessComplete,
        ExitThread,
        MessageLast
    };

    ExecutorThread(wxEvtHandler*, wxProcess*, wxMessageQueue<ThreadMessage>& q);
    ~ExecutorThread();

private:
    ExitCode Entry() wxOVERRIDE;
    void DrainInput();

    wxMessageQueue<ThreadMessage>& m_queue;
    wxEvtHandler* m_handler;
    wxProcess* m_process;
    char* m_buffer;
    size_t m_bufferSize;
};

wxDECLARE_EVENT(wxEVT_THREAD_STDIN, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_THREAD_STDERR, wxThreadEvent);

#endif // _EXECUTOR_THREAD_H_