#include <stdio.h>
#include <stdlib.h>

#include "run-aux-lib.h"
#include "arg2cmdline.h"


int match_flag(int argc, const wchar_t *argv[]) {
    const wchar_t *search_flags[] = {
        L"--build", L"--install", L"--open", L"-P", L"-E", L"--find-package", L"--workflow", L"--help",
    };

    for (int index = 1; index < argc; index++) {
        const wchar_t *arg = argv[index];
        for (int search_index = 0; search_index < sizeof(search_flags) / sizeof(const wchar_t *); search_index++) {
            if (wcscmp(arg, search_flags[search_index]) == 0) {
                return 1;
            }
        }
    }

    return 0;
}

int match_gen_flag(int argc, const wchar_t *argv[]) {
    for (int index = 1; index < argc; index++) {
        const wchar_t *arg = argv[index];
        if (arg[0] == '-' && arg[1] == 'G') {
            return 1;
        }
    }

    return 0;
}

// Require `free`
static wchar_t* format_wstring_with_str1(const wchar_t *format, const wchar_t *str1) {
    size_t buffer_base_size = wcslen(format) - 2 /* '%s' */ + 1 /* '\0' */;
    size_t buffer_size = buffer_base_size + wcslen(str1);
    wchar_t *buffer = calloc(buffer_size, sizeof(wchar_t));
    swprintf_s(buffer, buffer_size, format, str1);
    return buffer;
}

// Require `free`
static wchar_t* format_wstring_with_str2(const wchar_t *format, const wchar_t *str1, const wchar_t *str2) {
    size_t buffer_base_size = wcslen(format) - 2 /* '%s' */ + 1 /* '\0' */;
    size_t buffer_size = buffer_base_size + wcslen(str1) + wcslen(str2);
    wchar_t *buffer = calloc(buffer_size, sizeof(wchar_t));
    swprintf_s(buffer, buffer_size, format, str1, str2);
    return buffer;
}

// Require `free`
static wchar_t* format_wstring_with_str3(const wchar_t *format, const wchar_t *str1, const wchar_t *str2, const wchar_t *str3) {
    size_t buffer_base_size = wcslen(format) - 2 /* '%s' */ + 1 /* '\0' */;
    size_t buffer_size = buffer_base_size + wcslen(str1) + wcslen(str2) + wcslen(str3);
    wchar_t *buffer = calloc(buffer_size, sizeof(wchar_t));
    swprintf_s(buffer, buffer_size, format, str1, str2, str3);
    return buffer;
}


int wmain(int argc, const wchar_t *argv[]) {
    struct Path program_path = get_program_path();
    const wchar_t *original_program_arguments = get_program_arguments(argc);
    struct Path path = get_command_path(program_path.data, L".command");
    wchar_t *process_command = arg2cmdline(path.data);

    const wchar_t *program_arguments = NULL;

    if (match_flag(argc, argv)) {
        program_arguments = original_program_arguments;
    } else {
        // Check -G flag
        const wchar_t *gen_ninja_flag = match_gen_flag(argc, argv) ? L"" : L"-GNinja ";
        // Get toolchain flag
        wchar_t *toolchain_flag = NULL;
        {
            struct Path toolchain_path = get_root_path(program_path.data);
            Path_join(&toolchain_path, L"toolchain.cmake");
            wchar_t *buffer = format_wstring_with_str1(L"-DCMAKE_TOOLCHAIN_FILE=%s", toolchain_path.data);
            toolchain_flag = arg2cmdline(buffer);
            free(buffer);
        }
        program_arguments = format_wstring_with_str3(L"%s%s %s", gen_ninja_flag, toolchain_flag, original_program_arguments);
        free(toolchain_flag);
    }

    // Do execute
    int result = execute_with_arguments(process_command, program_arguments);
    free(process_command);
    if (program_arguments != original_program_arguments) {
        free((wchar_t*)program_arguments);
    }
    return result;
}
