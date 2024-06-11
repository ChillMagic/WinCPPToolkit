#include <Windows.h>
#include <stdio.h>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include "run-aux-lib.h"


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


extern struct Path Path_from(const wchar_t *str) {
    struct Path result;
    wcscpy_s(result.data, MAX_PATH, str);
    return result;
}

extern void Path_join(struct Path *path, const wchar_t *content) {
    PathCombineW(path->data, path->data, content);
}

extern void Path_to_parent(struct Path *path) {
    PathRemoveFileSpecW(path->data);
}

extern void Path_with_suffix(struct Path *path, const wchar_t *suffix) {
    PathRenameExtensionW(path->data, suffix);
}

extern const wchar_t* Path_name(const struct Path *path) {
    return PathFindFileNameW(path->data);
}


extern struct Path get_program_path() {
    struct Path result;

    if (GetModuleFileNameW(NULL, result.data, MAX_PATH) == 0) {
        wprintf(L"Error getting module file name.\n");
        exit(1);
    }

    return result;
}

extern wchar_t *get_program_arguments(int argc) {
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

    return arguments;
}

// Return xx.exe/../..
extern struct Path get_root_path(const wchar_t *program_path) {
    struct Path result = Path_from(program_path);

    Path_to_parent(&result);
    Path_to_parent(&result);

    return result;
}

extern struct Path get_bin_aux_file_path(const wchar_t *program_path, const wchar_t *suffix) {
    struct Path result = get_root_path(program_path);

    struct Path temp_program_path = Path_from(program_path);
    Path_with_suffix(&temp_program_path, suffix);
    const wchar_t *filename = Path_name(&temp_program_path);

    PathCombineW(result.data, result.data, L"bin-aux");
    PathCombineW(result.data, result.data, filename);

    return result;
}


// Result need `free`
extern wchar_t* read_file(const wchar_t *file_path) {
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


extern int execute_with_arguments(const wchar_t *process, const wchar_t *arguments) {
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
