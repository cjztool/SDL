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
#include "../../SDL_internal.h"

#ifdef SDL_FILESYSTEM_ANDROID

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent filesystem routines                                */

#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "SDL_error.h"
#include "SDL_system.h"
#include "../SDL_linuxfilesystem_c.h"

char *
SDL_GetBasePath(void)
{
    /* The current working directory is / on Android */
    return NULL;
}

char *
SDL_GetPrefPath(const char *org, const char *app)
{
    const char *path = SDL_AndroidGetInternalStoragePath();
    if (path) {
        size_t pathlen = SDL_strlen(path)+2;
        char *fullpath = (char *)SDL_malloc(pathlen);
        if (!fullpath) {
            SDL_OutOfMemory();
            return NULL;
        }
        SDL_snprintf(fullpath, pathlen, "%s/", path);
        return fullpath;
    }
    return NULL;
}

SDL_bool SDL_IsRootPath(const char* path)
{
	const char* res = "res/";
	if (!path || (path[0] != '/' && (SDL_strlen(path) < 4 || SDL_strncmp(path, res, 4)))) {
		return SDL_FALSE;
	}
	return SDL_TRUE;
}

SDL_bool SDL_IsDirectory(const char* dir)
{
	if (*dir == '/') {
		return linux_IsDirectory(dir);
	}

	// Try from the asset system
	return Android_JNI_IsDirectory(dir);
}

SDL_bool SDL_IsFile(const char* file)
{
	if (*file == '/') {
		return linux_IsFile(file);
	}

	// Try from the asset system
	return Android_JNI_IsFile(file);
}

SDL_DIR* SDL_OpenDir(char const* dir)
{
	return linux_OpenDir(dir);
}

SDL_dirent2* SDL_ReadDir(SDL_DIR* dir)
{
	return linux_ReadDir(dir);
}

int SDL_CloseDir(SDL_DIR* dir)
{
	return linux_CloseDir(dir);
}

SDL_bool SDL_GetStat(const char* name, SDL_dirent* stat)
{
	return linux_GetStat(name, stat);
}

SDL_bool SDL_MakeDirectory(const char* dir)
{
	return linux_MakeDirectory(dir);
}

SDL_bool SDL_DeleteFiles(const char* name)
{
	return linux_DeleteFiles(name);
}

SDL_bool SDL_CopyFiles(const char* src, const char* dst)
{
	return linux_CopyFiles(src, dst);
}

SDL_bool SDL_RenameFile(const char* oldname, const char* newname)
{
	return linux_RenameFile(oldname, newname);
}

void SDL_GetPlatformVariable(void** env, void** context, void** v3)
{
	// JavaVM* jvm, jobject context
	*env = (void*)Android_JNI_GetJavaVM();
	*context = (void*)Android_JNI_GetActivityObject();
}

#endif /* SDL_FILESYSTEM_ANDROID */

/* vi: set ts=4 sw=4 expandtab: */
