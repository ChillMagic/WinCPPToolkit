#pragma once
#ifndef RunAuxLibHeader
#define RunAuxLibHeader
#include <wchar.h>

#ifdef MAX_PATH
#   define _MAX_PATH_ MAX_PATH
#else
#   define _MAX_PATH_ 260
#endif

struct Path {
    wchar_t data[_MAX_PATH_];
};

struct Path Path_from(const wchar_t *str);
void Path_join(struct Path *path, const wchar_t *content);
void Path_to_parent(struct Path *path);
void Path_with_suffix(struct Path *path, const wchar_t *suffix);
const wchar_t* Path_name(const struct Path *path);

struct Path get_program_path();
wchar_t *get_program_arguments(int argc);
struct Path get_root_path(const wchar_t *program_path);
struct Path get_bin_aux_file_path(const wchar_t *program_path, const wchar_t *suffix);

wchar_t* read_file(const wchar_t *file_path); // Result need `free`

int execute_with_arguments(const wchar_t *process, const wchar_t *arguments);

#endif
