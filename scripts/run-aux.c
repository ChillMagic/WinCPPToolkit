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


int wmain(int argc, const wchar_t *argv[]) {
    wchar_t program_path[MAX_PATH];
    if (GetModuleFileNameW(NULL, program_path, MAX_PATH) == 0) {
        wprintf(L"Error getting module file name.\n");
        return 1;
    }

#ifdef COMMAND_MODE
    struct Path path = get_command_path(program_path);
#elif BATCH_MODE
    struct Path path = get_batch_path(program_path);
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
    size_t cmd_arguments_len = wcslen(prefix) + 2 /*""*/ + wcslen(path.data) + 1 /* */ + command_len; // prefix + "path" + ` ` + arguments
    wchar_t *cmd_arguments = malloc(cmd_arguments_len * sizeof(wchar_t));
    const wchar_t *format = NULL;
    if (wcsstr(path.data, L" ") || wcsstr(path.data, L"\t")) {
        format = L"%s\"%s\" %s";
    } else {
        format = L"%s%s %s";
    }
    swprintf_s(cmd_arguments, cmd_arguments_len, format, prefix, path.data, arguments);

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
