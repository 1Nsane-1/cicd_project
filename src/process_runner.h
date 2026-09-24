#ifndef PROCESS_RUNNER_H
#define PROCESS_RUNNER_H

#include <windows.h>
#include <string>
#include <vector>

inline bool run_command(const std::string& cmd, std::string& output) {
    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hRead, &hWrite, &saAttr, 0)) return false;
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    PROCESS_INFORMATION pi;
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    std::vector<char> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back('\0');

    BOOL success = CreateProcessA(
        NULL, cmdBuffer.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi
    );

    CloseHandle(hWrite);

    if (!success) {
        CloseHandle(hRead);
        output = "Failed to launch process: " + cmd;
        return false;
    }

    char buffer[512];
    DWORD bytesRead;
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hRead);

    return exitCode == 0;
}

#endif