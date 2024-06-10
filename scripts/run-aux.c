#include <Windows.h>
#include <stdio.h>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")


struct Path {
    wchar_t data[MAX_PATH];
};

// Result need `free`
static wchar_t* read_file(const wchar_t *file_path) {
    FILE *f = NULL;
    _wfopen_s(&f, file_path, L"r, ccs=UTF-8");
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    wchar_t* content = (wchar_t*)calloc(file_size + 1, sizeof(wchar_t));
    fread(content, sizeof(wchar_t), file_size, f);
    fclose(f);

    return content;
}

static struct Path get_filename_with_suffix(const wchar_t *program_path, const wchar_t *suffix) {
    struct Path result;

    wchar_t *filename_result = PathFindFileNameW(program_path);
    wcscpy_s(result.data, MAX_PATH, filename_result);
    PathRenameExtensionW(result.data, suffix);

    return result;
}


// Return xx.exe/../..
static struct Path get_root_path(const wchar_t *program_path) {
    struct Path result;

    wcscpy_s(result.data, MAX_PATH, program_path);
    PathRemoveFileSpecW(result.data);
    PathRemoveFileSpecW(result.data);

    return result;
}


static struct Path get_bin_aux_file_path(const wchar_t *program_path, const wchar_t *suffix) {
    struct Path result = get_root_path(program_path);

    struct Path filename = get_filename_with_suffix(program_path, suffix);
    PathCombineW(result.data, result.data, L"bin-aux");
    PathCombineW(result.data, result.data, filename.data);

    return result;
}


static struct Path get_command_path(const wchar_t *program_path) {
    struct Path result = get_root_path(program_path);

    struct Path command_file_path = get_bin_aux_file_path(program_path, L".command");
    wchar_t *content = read_file(command_file_path.data);

    PathCombineW(result.data, result.data, content);
    free(content);

    return result;
}

static struct Path get_batch_path(const wchar_t *program_path) {
    return get_bin_aux_file_path(program_path, L".bat");
}


static int execute(const wchar_t *command_line) {
#ifdef DEBUG
    wprintf(L"command_line: %s\n", command_line);
#endif

    // Get command_line (wchar_t *)
    size_t len = wcslen(command_line) + 1;
    wchar_t *buffer = calloc(len, sizeof(wchar_t));
    wcscpy_s(buffer, len, command_line);

    // Do execute
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi;

    if (!CreateProcessW(NULL, buffer, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi))
    {
        wprintf(L"Run ShellExecute error, return code: %lld\n", (long long)GetLastError());
    }
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD dwExitCode = 0;
    GetExitCodeProcess(pi.hProcess, &dwExitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    free(buffer);

    return dwExitCode;
}

static int execute_with_arguments(const wchar_t *process, const wchar_t *arguments) {
#ifdef DEBUG
    wprintf(L"process: %s\n", process);
    wprintf(L"arguments: %s\n", arguments);
#endif

    size_t command_line_size = wcslen(process) + 1 /* */ + wcslen(arguments) + 1 /* '\0' */; // process + ` ` + arguments
    wchar_t *command_line = calloc(command_line_size, sizeof(wchar_t));
    swprintf_s(command_line, command_line_size, L"%s %s", process, arguments);
    int result = execute(command_line);
    free(command_line);
    return result;
}


int wmain(int argc, const wchar_t *argv[]) {
    // Get program_path
    wchar_t program_path[MAX_PATH];
    if (GetModuleFileNameW(NULL, program_path, MAX_PATH) == 0) {
        wprintf(L"Error getting module file name.\n");
        return 1;
    }

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

    // Get process command
#ifdef COMMAND_MODE
    const wchar_t *prefix = L"";
    struct Path path = get_command_path(program_path);
#elif BATCH_MODE
    const wchar_t *prefix = L"cmd.exe /C ";
    struct Path path = get_batch_path(program_path);
#endif
    size_t process_command_size = wcslen(prefix) + 2 /*""*/ + wcslen(path.data) + 1 /* '\0' */;
    wchar_t *process_command = calloc(process_command_size, sizeof(wchar_t));
    const wchar_t *format = NULL;
    if (wcsstr(path.data, L" ") || wcsstr(path.data, L"\t")) {
        format = L"%s\"%s\"";
    } else {
        format = L"%s%s";
    }
    swprintf_s(process_command, process_command_size, format, prefix, path.data);

    // Do execute
    int result = execute_with_arguments(process_command, arguments);
    free(process_command);
    return result;
}
