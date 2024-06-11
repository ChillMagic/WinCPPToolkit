// Referenced the implementation of list2cmdline from CPython's subprocess.py.

#include <wchar.h>
#include <stdlib.h>

static void _append_bs(wchar_t **current, size_t bs_count) {
    for (size_t i = 0; i < bs_count; i++) {
        **current = '\\';
        (*current)++;
    }
}

static size_t _arg2cmdline(wchar_t *out_result, const wchar_t *arg, size_t arg_len) {
    wchar_t *current = out_result;

    size_t bs_count = 0;
    int need_quote = wcschr(arg, L' ') != NULL || wcschr(arg, L'\t') != NULL || arg[0] == '\0';
    if (need_quote) {
        *current++ = '"';
    }

    for (size_t index = 0; index < arg_len; index++) {
        wchar_t c = arg[index];
        switch (c) {
        case '\\':
            bs_count += 1;
            break;
        case '"':
            _append_bs(&current, bs_count * 2);
            bs_count = 0;
            *current++ = '\\';
            *current++ = '"';
            break;
        default:
            if (bs_count) {
                _append_bs(&current, bs_count);
                bs_count = 0;
            }
            *current++ = c;
            break;
        }
    }

    if (bs_count) {
        _append_bs(&current, bs_count);
    }
    if (need_quote) {
        _append_bs(&current, bs_count);
        *current++ = '"';
    }

    return current - out_result;
}

static size_t _max_cmdline_size_for_arg(const wchar_t *arg, size_t arg_len) {
    return arg_len * 2 /* double '\\' */ + 2 /* '""' */ + 1 /* '\0' or ' ' */;
}

static size_t _max_cmdline_size_for_arglist(int argc, const wchar_t *argv[]) {
    size_t result = 1; // '\0'
    for (int index = 0; index < argc; index++) {
        result += _max_cmdline_size_for_arg(argv[index], wcslen(argv[index]));
    }
    return result;
}


wchar_t* arg2cmdline(const wchar_t *arg) {
    size_t arg_len = wcslen(arg);
    wchar_t *result = calloc(_max_cmdline_size_for_arg(arg, arg_len), sizeof(wchar_t));
    if (result) {
        _arg2cmdline(result, arg, arg_len);
    }
    return result;
}

wchar_t* arglist2cmdline(int argc, const wchar_t *argv[]) {
    wchar_t *result = calloc(_max_cmdline_size_for_arglist(argc, argv), sizeof(wchar_t));
    if (result == NULL) {
        return NULL;
    }

    wchar_t *current = result;
    for (int index = 0; index < argc; index++) {
        size_t len = _arg2cmdline(current, argv[index], wcslen(argv[index]));
        current += len;
        if (index < argc - 1) {
            *current++ = L' ';
        }
    }

    return result;
}
