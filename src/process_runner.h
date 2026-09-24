#ifndef PROCESS_RUNNER_H
#define PROCESS_RUNNER_H

#include <windows.h>
#include <string>
#include <vector>

// Функция перевода вывода консоли Windows (CP866/OEM) в UTF-8 для веб-интерфейса
inline std::string convert_oem_to_utf8(const std::string& input) {
    if (input.empty()) return "";

    // 1. Преобразование OEM (CP866) -> UTF-16 (WideChar)
    int wlen = MultiByteToWideChar(CP_OEMCP, 0, input.c_str(), -1, NULL, 0);
    if (wlen <= 0) return input;

    std::vector<wchar_t> wbuf(wlen);
    MultiByteToWideChar(CP_OEMCP, 0, input.c_str(), -1, wbuf.data(), wlen);

    // 2. Преобразование UTF-16 -> UTF-8
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), -1, NULL, 0, NULL, NULL);
    if (ulen <= 0) return input;

    std::vector<char> ubuf(ulen);
    WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), -1, ubuf.data(), ulen, NULL, NULL);

    return std::string(ubuf.data());
}

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
    std::string raw_output;
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        raw_output += buffer;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hRead);

    // Автоматическая конвертация вывода консоли в чистый UTF-8
    output = convert_oem_to_utf8(raw_output);

    return exitCode == 0;
}

#endif