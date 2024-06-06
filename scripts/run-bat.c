#include <Windows.h>
#include <stdio.h>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")


int wmain(int argc, const wchar_t *argv[]) {
    wchar_t path[MAX_PATH];
    wchar_t filename[MAX_PATH];

    if (GetModuleFileNameW(NULL, path, MAX_PATH) == 0) {
        wprintf(L"Error getting module file name.\n");
        return 1;
    }

    // Get xxx.bat
    wchar_t *filename_result = PathFindFileNameW(path);
    
    wcscpy_s(filename, MAX_PATH, filename_result);
    PathRenameExtensionW(filename, L".bat");

    // Get ../../bin-bat/xxx.bat
    PathRemoveFileSpecW(path);
    PathRemoveFileSpecW(path);
    PathCombineW(path, path, L"bin-bat");
    PathCombineW(path, path, filename);

    // Get arguments
    wchar_t *command_line = GetCommandLineW();
    size_t command_len = wcslen(command_line);
    wchar_t *arguments = NULL;

    if (argc == 1) {
        arguments = L"";
    } else if (command_line[0] == '"') {
        arguments = wcschr(command_line + 1, L'"') + 2;
    } else {
        arguments = wcschr(command_line, L' ') + 1;
    }

    // wprintf(L"path: %s\n", path);
    // wprintf(L"arguments: %s\n", arguments);

    const wchar_t *prefix = L"cmd.exe /C ";
    size_t cmd_arguments_len = wcslen(prefix) + 2 /*""*/ + wcslen(path) + 1 /* */ + command_len; // prefix + "path" + ` ` + arguments
    wchar_t *cmd_arguments = malloc(cmd_arguments_len * sizeof(wchar_t));
    const wchar_t *format = NULL;
    if (wcsstr(path, L" ") || wcsstr(path, L"\t")) {
        format = L"%s\"%s\" %s";
    } else {
        format = L"%s%s %s";
    }
    swprintf_s(cmd_arguments, cmd_arguments_len, format, prefix, path, arguments);

    // wprintf(L"Runing: %s\n", cmd_arguments);

    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi;

    if (!CreateProcessW(NULL, cmd_arguments, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi))
    {
        wprintf(L"Run ShellExecute error, return code: %lld\n", (long long)GetLastError());
    }
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD dwExitCode = 0;
    GetExitCodeProcess(pi.hProcess, &dwExitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return dwExitCode;
}
