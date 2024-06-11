#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>

#include "run-aux-lib.h"
#include "arg2cmdline.h"


static struct Path get_command_path(const wchar_t *program_path) {
    struct Path result = get_root_path(program_path);

    struct Path command_file_path = get_bin_aux_file_path(program_path, L".command");
    wchar_t *content = read_file(command_file_path.data);

    Path_join(&result, content);
    free(content);

    return result;
}

static struct Path get_batch_path(const wchar_t *program_path) {
    return get_bin_aux_file_path(program_path, L".bat");
}


int wmain(int argc, const wchar_t *argv[]) {
    struct Path program_path = get_program_path();
    wchar_t *program_arguments = get_program_arguments(argc);

    // Get process command
#ifdef COMMAND_MODE
    struct Path path = get_command_path(program_path.data);
    wchar_t *process_command = arg2cmdline(path.data);
#elif BATCH_MODE
    const wchar_t *prefix = L"cmd.exe /C ";
    struct Path path = get_batch_path(program_path.data);
    wchar_t *path_quoted = arg2cmdline(path.data);
    size_t process_command_size = wcslen(prefix) + wcslen(path_quoted) + 1 /* '\0' */;
    wchar_t *process_command = calloc(process_command_size, sizeof(wchar_t));
    swprintf_s(process_command, process_command_size, L"%s%s", prefix, path_quoted);
    free(path_quoted);
#endif

    // Do execute
    int result = execute_with_arguments(process_command, program_arguments);
    free(process_command);
    return result;
}
