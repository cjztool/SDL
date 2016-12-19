/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../SDL_internal.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent filesystem routines                                */

#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

#include "SDL_error.h"
#include "SDL_linuxfilesystem_c.h"


SDL_bool linux_IsRootPath(const char* path)
{
	if (!path || path[0] != '/') {
		return SDL_FALSE;
	}
	return SDL_TRUE;
}

SDL_bool linux_IsDirectory(const char* dir)
{
	if (*dir == '/') {
		struct stat dir_stat;
		if (stat(dir, &dir_stat) == -1) {
			return SDL_FALSE;
		}
		return S_ISDIR(dir_stat.st_mode)? SDL_TRUE: SDL_FALSE;
	}

	return SDL_FALSE;
}

SDL_bool linux_IsFile(const char* file)
{
	if (*file == '/') {
		struct stat st;
        if (stat(file, &st) == -1) {
            return SDL_FALSE;
        }
		return S_ISREG(st.st_mode)? SDL_TRUE: SDL_FALSE;
	}

	return SDL_FALSE;
}

typedef struct
{
	DIR* dir;
	struct dirent* entry;
} SDL_LinuxDIR;

SDL_DIR* linux_OpenDir(char const* dir)
{
	SDL_DIR* result = NULL;
	SDL_LinuxDIR* data;

	// Must be a valid name
	if (!dir || !dir[0] || dir[0] != '/') {
		return NULL;
	}

	result = (SDL_DIR*)SDL_malloc(sizeof(SDL_DIR));
	if (!result) {
		return NULL;
	}
    data = (SDL_LinuxDIR*)SDL_malloc(sizeof(*data));
    if (!data) {
		SDL_free(result);
		return NULL;
    }

	data->dir = opendir(dir);
	if (data->dir) {
		data->entry = readdir(data->dir);

		SDL_strlcpy(result->directory, dir, sizeof(result->directory));
		result->driverdata = data;

	} else {
		SDL_free(data);
		SDL_free(result);
        result = NULL;
	}
	
	return result;
}

SDL_dirent2* linux_ReadDir(SDL_DIR* dir)
{
	SDL_LinuxDIR* data;
	
	if (!dir) {
		return NULL;
	}
	data = (SDL_LinuxDIR*)dir->driverdata;

	if (!data->entry) {
		return NULL;

	} else {
		struct stat st;
		int dir_size = (int)SDL_strlen(dir->directory);
		int size = dir_size + 1 + (int)SDL_strlen(data->entry->d_name) + 1;
		char* tmp = SDL_malloc(size);
		SDL_memcpy(tmp, dir->directory, dir_size);
		tmp[dir_size] = '/';
		SDL_memcpy(tmp + dir_size + 1, data->entry->d_name, SDL_strlen(data->entry->d_name));
		tmp[size - 1] = '\0';

		stat(tmp, &st);

        SDL_strlcpy(dir->dirent.name, data->entry->d_name, sizeof(dir->dirent.name));
		dir->dirent.mode = S_ISDIR(st.st_mode)? SDL_S_IFDIR: SDL_S_IFREG;
		dir->dirent.size = st.st_size;
		dir->dirent.ctime = st.st_ctime;
		dir->dirent.mtime = st.st_mtime;
		dir->dirent.atime = st.st_atime;

		data->entry = readdir(data->dir);

		SDL_free(tmp);
		return &dir->dirent;
	}
}

int linux_CloseDir(SDL_DIR* dir)
{
	int ret = -1;
	if (dir) {
		SDL_LinuxDIR* data = (SDL_LinuxDIR*)dir->driverdata;
		// Close the search handle, if not already done.
		if (data->dir) {
			closedir(data->dir);
		}
		SDL_free(dir->driverdata);
		SDL_free(dir);

		ret = 0;
	}

	return ret;
}

SDL_bool linux_GetStat(const char* name, SDL_dirent* dirent)
{
	struct stat st;
	if (!name || !name[0]) {
		return SDL_FALSE;
	}
	
	if (stat(name, &st) == -1) {
		return SDL_FALSE;
	}
	dirent->mode = S_ISDIR(st.st_mode)? SDL_S_IFDIR: SDL_S_IFREG;
	dirent->size = st.st_size;
	dirent->ctime = st.st_ctime;
	dirent->mtime = st.st_mtime;
	dirent->atime = st.st_atime;
	return SDL_TRUE;
}

SDL_bool linux_MakeDirectory(const char* dir)
{
	size_t size = SDL_strlen(dir);
	// last characer cannot / or \, FindFirstFile cannot process path that terminated by / or \.
	if (!dir || !dir[0] || dir[size - 1] == '/') {
		return SDL_FALSE;
	}
	const mode_t AccessMode = 00770;
	mkdir(dir, AccessMode);

	return SDL_TRUE;
}

SDL_bool linux_DeleteFiles(const char* name)
{
	SDL_DIR* dir;
	SDL_dirent2* dirent;
	int ret = 0;
	char* full_name = NULL;

	if (!name || !name[0]) {
		return SDL_FALSE;
	}
	if (SDL_IsFile(name)) {
		ret = remove(name);
		return ret == 0? SDL_TRUE: SDL_FALSE;
	}

	dir = SDL_OpenDir(name);
	if (!dir) {
		return SDL_FALSE;
	}
	
	while ((dirent = SDL_ReadDir(dir))) {
		if (!full_name) {
			full_name = (char*)SDL_malloc(1024);
		}
		sprintf(full_name, "%s/%s", name, dirent->name);
		if (SDL_DIRENT_DIR(dirent->mode)) {
			if (SDL_strcmp(dirent->name, ".") && SDL_strcmp(dirent->name, "..")) {
				if (!linux_DeleteFiles(full_name)) {
					ret = -1;
					break;
				}
			}
		} else {
			// file
			ret = remove(full_name);
			if (ret != 0) {
				break;
			}
		}
	}
	SDL_CloseDir(dir);
	if (full_name) {
		SDL_free(full_name);
	}

	if (ret != 0) {
		return SDL_FALSE;
	}
	ret = rmdir(name);
	return ret == 0? SDL_TRUE: SDL_FALSE;
}

SDL_bool linux_CopyFiles(const char* src, const char* dst)
{
	if (!src || !src[0] || !dst || !dst[0]) {
		return SDL_FALSE;
	}
	return SDL_FALSE;
}

// oldname is full name.
// newname is terminate name. full new name is base_path(oldname) + newname
SDL_bool linux_RenameFile(const char* oldname, const char* newname)
{
	if (!oldname || !oldname[0] || !newname || !newname[0]) {
		return SDL_FALSE;
	}

	// oldname is full name.
	int last_split_pos = -1, at, size = (int)SDL_strlen(oldname);
	if (oldname[size - 1] == '\\' || oldname[size - 1] == '/') {
		return SDL_FALSE;
	}
	for (at = size - 1; at >= 0; at -- ) {
		if (oldname[at] == '\\' || oldname[at] == '/') {
			last_split_pos = at;
			break;
		}
	}
	if (last_split_pos == -1) {
		return SDL_FALSE;
	}
	if (!SDL_strcasecmp(oldname + last_split_pos + 1, newname)) {
		return SDL_TRUE;
	}

	// newname is termimate name.
	size = (int)SDL_strlen(newname);
	for (at = 0; at < size; at ++ ) {
		if (newname[at] == '\\' || newname[at] == '/' || newname[at] == ':') {
			return SDL_FALSE;
		}
	}

	// form newname2
	char* newname2 = SDL_malloc(last_split_pos + 1 + size + 1);
	SDL_memcpy(newname2, oldname, last_split_pos + 1);
	SDL_memcpy(newname2 + last_split_pos + 1, newname, size);
	newname2[last_split_pos + 1 + size] = '\0';

	if (SDL_IsDirectory(oldname)) {
		// desire rename directory.
		if (SDL_IsFile(newname2)) {
			SDL_free(newname2);
			return SDL_FALSE;

		} else if (SDL_IsDirectory(newname2)) {
			if (!SDL_DeleteFiles(newname2)) {
				SDL_free(newname2);
				return SDL_FALSE;
			}
		}
	} else if (SDL_IsFile(oldname)) {
		// desire rename file.
		if (SDL_IsDirectory(newname2)) {
			SDL_free(newname2);
			return SDL_FALSE;

		} else if (SDL_IsFile(newname2)) {
			if (!SDL_DeleteFiles(newname2)) {
				SDL_free(newname2);
				return SDL_FALSE;
			}
		}
	} else {
		SDL_free(newname2);
		return SDL_FALSE;
	}
	int ret = rename(oldname, newname2);

	SDL_free(newname2);
	return ret == 0? SDL_TRUE: SDL_FALSE;
}

/* vi: set ts=4 sw=4 expandtab: */
