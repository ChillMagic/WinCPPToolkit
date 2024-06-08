#include <Windows.h>
#include <stdio.h>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")


#ifdef COMMAND_MODE
static void get_command(wchar_t out_path[MAX_PATH], const wchar_t *program_path) {
    wchar_t command_file_path[MAX_PATH];
    wcscpy_s(command_file_path, MAX_PATH, program_path);
    // Get xxx.command
    wchar_t filename[MAX_PATH];
    wchar_t *filename_result = PathFindFileNameW(command_file_path);
    wcscpy_s(filename, MAX_PATH, filename_result);
    PathRenameExtensionW(filename, L".command");

    // Get ../../bin-bat/xxx.command
    PathRemoveFileSpecW(command_file_path);
    PathRemoveFileSpecW(command_file_path);
    PathCombineW(command_file_path, command_file_path, L"bin-bat");
    PathCombineW(command_file_path, command_file_path, filename);

    // Get content
    FILE *f = NULL;
    _wfopen_s(&f, command_file_path, L"r, ccs=UTF-8");
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    wchar_t* content = (wchar_t*)calloc(file_size + 1, sizeof(wchar_t));
    fread(content, sizeof(wchar_t), file_size, f);
    fclose(f);

    // Path join
    wcscpy_s(out_path, MAX_PATH, program_path);
    PathRemoveFileSpecW(out_path);
    PathRemoveFileSpecW(out_path);
    PathCombineW(out_path, out_path, content);
}
#elif BATCH_MODE
static void get_batch_path(wchar_t out_path[MAX_PATH], const wchar_t *program_path) {
    // Get xxx.bat
    wchar_t *filename_result = PathFindFileNameW(program_path);
    wchar_t filename[MAX_PATH];
    wcscpy_s(filename, MAX_PATH, filename_result);
    PathRenameExtensionW(filename, L".bat");

    // Get ../../bin-bat/xxx.bat
    wcscpy_s(out_path, MAX_PATH, program_path);
    PathRemoveFileSpecW(out_path);
    PathRemoveFileSpecW(out_path);
    PathCombineW(out_path, out_path, L"bin-bat");
    PathCombineW(out_path, out_path, filename);
}
#endif


int wmain(int argc, const wchar_t *argv[]) {
    wchar_t program_path[MAX_PATH];
    if (GetModuleFileNameW(NULL, program_path, MAX_PATH) == 0) {
        wprintf(L"Error getting module file name.\n");
        return 1;
    }

    wchar_t path[MAX_PATH];
#ifdef COMMAND_MODE
    get_command(path, program_path);
#elif BATCH_MODE
    get_batch_path(path, program_path);
#endif

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

#ifdef COMMAND_MODE
    const wchar_t *prefix = L"";
#elif BATCH_MODE
    const wchar_t *prefix = L"cmd.exe /C ";
#endif
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
