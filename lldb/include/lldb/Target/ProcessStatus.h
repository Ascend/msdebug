#ifdef MS_DEBUGGER
#ifndef LLDB_TARGET_PROCESS_STATUS_H
#define LLDB_TARGET_PROCESS_STATUS_H


namespace lldb_private {

class ProcessStatus {
public:
    ProcessStatus(ProcessStatus const &) = delete;

    ProcessStatus &operator=(ProcessStatus const &) = delete;
public:
    static ProcessStatus &Instance() {
        static ProcessStatus inst;
        return inst;
    }

    void SetStatus(bool stop_in_device) {
        m_stop_in_device = stop_in_device;
    }

    bool IsStopInDevice() const {
        return m_stop_in_device;
    }

protected:
    ProcessStatus(void) = default;

    ~ProcessStatus(void) = default;

private:
    bool m_stop_in_device{false};
};
} // namespace lldb_private

#endif
#endif
