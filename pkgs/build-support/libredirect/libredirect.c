#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <spawn.h>
#include <dirent.h>

#define MAX_REDIRECTS 128

static int nrRedirects = 0;
static char * from[MAX_REDIRECTS];
static char * to[MAX_REDIRECTS];

static int isInitialized = 0;

// FIXME: might run too late.
static void init() __attribute__((constructor));

static void init()
{
    if (isInitialized) return;

    char * spec = getenv("NIX_REDIRECTS");
    if (!spec) return;

    // Ensure we only run this code once.
    // We do not do `unsetenv("NIX_REDIRECTS")` to ensure that redirects
    // also get initialized for subprocesses.
    isInitialized = 1;

    char * spec2 = malloc(strlen(spec) + 1);
    strcpy(spec2, spec);

    char * pos = spec2, * eq;
    while ((eq = strchr(pos, '='))) {
        *eq = 0;
        from[nrRedirects] = pos;
        pos = eq + 1;
        to[nrRedirects] = pos;
        nrRedirects++;
        if (nrRedirects == MAX_REDIRECTS) break;
        char * end = strchr(pos, ':');
        if (!end) break;
        *end = 0;
        pos = end + 1;
    }

}

static const char * rewrite(const char * path, char * buf)
{
    if (path == NULL) return path;
    for (int n = 0; n < nrRedirects; ++n) {
        int len = strlen(from[n]);
        if (strncmp(path, from[n], len) != 0) continue;
        if (snprintf(buf, PATH_MAX, "%s%s", to[n], path + len) >= PATH_MAX)
            abort();
        return buf;
    }

    return path;
}

static int open_needs_mode(int flags)
{
#ifdef O_TMPFILE
    return (flags & O_CREAT) || (flags & O_TMPFILE) == O_TMPFILE;
#else
    return flags & O_CREAT;
#endif
}

/* The following set of Glibc library functions is very incomplete -
   it contains only what we needed for programs in Nixpkgs. Just add
   more functions as needed. */

int open(const char * path, int flags, ...)
{
    int (*open_real) (const char *, int, mode_t) = dlsym(RTLD_NEXT, "open");
    mode_t mode = 0;
    if (open_needs_mode(flags)) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }
    char buf[PATH_MAX];
    return open_real(rewrite(path, buf), flags, mode);
}

int open64(const char * path, int flags, ...)
{
    int (*open64_real) (const char *, int, mode_t) = dlsym(RTLD_NEXT, "open64");
    mode_t mode = 0;
    if (open_needs_mode(flags)) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }
    char buf[PATH_MAX];
    return open64_real(rewrite(path, buf), flags, mode);
}

int openat(int dirfd, const char * path, int flags, ...)
{
    int (*openat_real) (int, const char *, int, mode_t) = dlsym(RTLD_NEXT, "openat");
    mode_t mode = 0;
    if (open_needs_mode(flags)) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }
    char buf[PATH_MAX];
    return openat_real(dirfd, rewrite(path, buf), flags, mode);
}

FILE * fopen(const char * path, const char * mode)
{
    FILE * (*fopen_real) (const char *, const char *) = dlsym(RTLD_NEXT, "fopen");
    char buf[PATH_MAX];
    return fopen_real(rewrite(path, buf), mode);
}

FILE * __nss_files_fopen(const char * path)
{
    FILE * (*__nss_files_fopen_real) (const char *) = dlsym(RTLD_NEXT, "__nss_files_fopen");
    char buf[PATH_MAX];
    return __nss_files_fopen_real(rewrite(path, buf));
}

FILE * fopen64(const char * path, const char * mode)
{
    FILE * (*fopen64_real) (const char *, const char *) = dlsym(RTLD_NEXT, "fopen64");
    char buf[PATH_MAX];
    return fopen64_real(rewrite(path, buf), mode);
}

int __xstat(int ver, const char * path, struct stat * st)
{
    int (*__xstat_real) (int ver, const char *, struct stat *) = dlsym(RTLD_NEXT, "__xstat");
    char buf[PATH_MAX];
    return __xstat_real(ver, rewrite(path, buf), st);
}

int __xstat64(int ver, const char * path, struct stat64 * st)
{
    int (*__xstat64_real) (int ver, const char *, struct stat64 *) = dlsym(RTLD_NEXT, "__xstat64");
    char buf[PATH_MAX];
    return __xstat64_real(ver, rewrite(path, buf), st);
}

int stat(const char * path, struct stat * st)
{
    int (*__stat_real) (const char *, struct stat *) = dlsym(RTLD_NEXT, "stat");
    char buf[PATH_MAX];
    return __stat_real(rewrite(path, buf), st);
}

int access(const char * path, int mode)
{
    int (*access_real) (const char *, int mode) = dlsym(RTLD_NEXT, "access");
    char buf[PATH_MAX];
    return access_real(rewrite(path, buf), mode);
}

int posix_spawn(pid_t * pid, const char * path,
    const posix_spawn_file_actions_t * file_actions,
    const posix_spawnattr_t * attrp,
    char * const argv[], char * const envp[])
{
    int (*posix_spawn_real) (pid_t *, const char *,
        const posix_spawn_file_actions_t *,
        const posix_spawnattr_t *,
        char * const argv[], char * const envp[]) = dlsym(RTLD_NEXT, "posix_spawn");
    char buf[PATH_MAX];
    return posix_spawn_real(pid, rewrite(path, buf), file_actions, attrp, argv, envp);
}

int posix_spawnp(pid_t * pid, const char * file,
    const posix_spawn_file_actions_t * file_actions,
    const posix_spawnattr_t * attrp,
    char * const argv[], char * const envp[])
{
    int (*posix_spawnp_real) (pid_t *, const char *,
        const posix_spawn_file_actions_t *,
        const posix_spawnattr_t *,
        char * const argv[], char * const envp[]) = dlsym(RTLD_NEXT, "posix_spawnp");
    char buf[PATH_MAX];
    return posix_spawnp_real(pid, rewrite(file, buf), file_actions, attrp, argv, envp);
}

int execv(const char * path, char * const argv[])
{
    int (*execv_real) (const char * path, char * const argv[]) = dlsym(RTLD_NEXT, "execv");
    char buf[PATH_MAX];
    return execv_real(rewrite(path, buf), argv);
}

int execvp(const char * path, char * const argv[])
{
    int (*_execvp) (const char *, char * const argv[]) = dlsym(RTLD_NEXT, "execvp");
    char buf[PATH_MAX];
    return _execvp(rewrite(path, buf), argv);
}

int execve(const char * path, char * const argv[], char * const envp[])
{
    int (*_execve) (const char *, char * const argv[], char * const envp[]) = dlsym(RTLD_NEXT, "execve");
    char buf[PATH_MAX];
    return _execve(rewrite(path, buf), argv, envp);
}

DIR * opendir(const char * path)
{
    char buf[PATH_MAX];
    DIR * (*_opendir) (const char*) = dlsym(RTLD_NEXT, "opendir");

    return _opendir(rewrite(path, buf));
}

#define SYSTEM_CMD_MAX 512

char *replace_substring(char * source, char * buf, char * replace_string, char * start_ptr, char * suffix_ptr) {
    char head[SYSTEM_CMD_MAX] = {0};
    strncpy(head, source, start_ptr - source);

    char tail[SYSTEM_CMD_MAX] = {0};
    if(suffix_ptr < source + strlen(source)) {
       strcpy(tail, suffix_ptr);
    }

    sprintf(buf, "%s%s%s", head, replace_string, tail);
    return buf;
}

char *replace_string(char * buf, char * from, char * to) {
    int num_matches = 0;
    char * matches[SYSTEM_CMD_MAX];
    int from_len = strlen(from);
    for(int i=0; i<strlen(buf); i++){
       char *cmp_start = buf + i;
       if(strncmp(from, cmp_start, from_len) == 0){
          matches[num_matches] = cmp_start;
          num_matches++;
       }
    }
    int len_diff = strlen(to) - strlen(from);
    for(int n = 0; n < num_matches; n++) {
       char replaced[SYSTEM_CMD_MAX];
       replace_substring(buf, replaced, to, matches[n], matches[n]+from_len);
       strcpy(buf, replaced);
       for(int nn = n+1; nn < num_matches; nn++) {
          matches[nn] += len_diff;
       }
    }
    return buf;
}

void rewriteSystemCall(const char * command, char * buf) {
    strcpy(buf, command);
    for (int n = 0; n < nrRedirects; ++n) {
       replace_string(buf, from[n], to[n]);
    }
}

int system(const char *command)
{
    int (*_system) (const char*) = dlsym(RTLD_NEXT, "system");

    char newCommand[SYSTEM_CMD_MAX];
    rewriteSystemCall(command, newCommand);
    return _system(newCommand);
}

int mkdir(const char *path, mode_t mode)
{
    int (*mkdir_real) (const char *path, mode_t mode) = dlsym(RTLD_NEXT, "mkdir");
    char buf[PATH_MAX];
    return mkdir_real(rewrite(path, buf), mode);
}

int mkdirat(int dirfd, const char *path, mode_t mode)
{
    int (*mkdirat_real) (int dirfd, const char *path, mode_t mode) = dlsym(RTLD_NEXT, "mkdirat");
    char buf[PATH_MAX];
    return mkdirat_real(dirfd, rewrite(path, buf), mode);
}
