/*
 * ilma_pkg.c — ILMA package manager
 *
 * Downloads packages from the ilma-packages GitHub registry,
 * installs them to ~/.ilma/packages/<name>/, and lists installed
 * or available packages.
 */

#include "ilma_pkg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#define REGISTRY_URL \
    "https://raw.githubusercontent.com/raihan-js/ilma-packages/main/registry.json"
#define REGISTRY_TMP "/tmp/ilma_registry.json"
#define MAX_PKG_NAME 128
#define MAX_URL      512
#define MAX_LINE     1024
#define MAX_PATH_LEN 512

/* ── Helpers ──────────────────────────────────────────────── */

/* Build the path to ~/.ilma/packages/ */
static int get_packages_dir(char* buf, size_t buf_size) {
    const char* home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "Could not determine home directory.\n");
        return -1;
    }
    snprintf(buf, buf_size, "%s/.ilma/packages", home);
    return 0;
}

/* Create directory path recursively (mkdir -p equivalent) */
static void mkdirs(const char* path) {
    char tmp[MAX_PATH_LEN];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

/* Download a URL to a local file. Returns 0 on success. */
static int download_file(const char* url, const char* dest) {
    char cmd[MAX_URL + MAX_PATH_LEN + 64];

    /* Try curl first */
    snprintf(cmd, sizeof(cmd), "curl -sL \"%s\" -o \"%s\" 2>/dev/null", url, dest);
    if (system(cmd) == 0) {
        /* Verify the file was actually written */
        struct stat st;
        if (stat(dest, &st) == 0 && st.st_size > 0) {
            return 0;
        }
    }

    /* Fall back to wget */
    snprintf(cmd, sizeof(cmd), "wget -qO \"%s\" \"%s\" 2>/dev/null", dest, url);
    if (system(cmd) == 0) {
        struct stat st;
        if (stat(dest, &st) == 0 && st.st_size > 0) {
            return 0;
        }
    }

    return -1;
}

/* Read entire file into a malloc'd buffer. Caller must free(). */
static char* read_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (len <= 0) {
        fclose(f);
        return NULL;
    }

    char* buf = malloc((size_t)len + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t n = fread(buf, 1, (size_t)len, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

/*
 * Extract a JSON string value for a given key near a position.
 * Looks for "key": "value" starting from *pos in the buffer.
 * Writes the value into out (up to out_size-1 chars).
 * Returns pointer past the extracted value, or NULL on failure.
 */
static const char* json_extract_string(const char* pos, const char* key,
                                        char* out, size_t out_size) {
    char pattern[MAX_PKG_NAME + 8];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char* found = strstr(pos, pattern);
    if (!found) return NULL;

    /* Skip past "key" */
    found += strlen(pattern);

    /* Skip whitespace and colon */
    while (*found && (*found == ' ' || *found == ':' || *found == '\t'))
        found++;

    /* Expect opening quote */
    if (*found != '"') return NULL;
    found++;

    /* Copy until closing quote */
    size_t i = 0;
    while (*found && *found != '"' && i < out_size - 1) {
        out[i++] = *found++;
    }
    out[i] = '\0';

    if (*found == '"') found++;
    return found;
}

/* ── Download and cache registry ──────────────────────────── */

static char* fetch_registry(void) {
    if (download_file(REGISTRY_URL, REGISTRY_TMP) != 0) {
        return NULL;
    }
    return read_file(REGISTRY_TMP);
}

/* ═════════════════════════════════════════════════════════════
 *  ilma_pkg_install — Download and install a package by name
 * ═════════════════════════════════════════════════════════════ */

int ilma_pkg_install(const char* package_name) {
    if (!package_name || package_name[0] == '\0') {
        fprintf(stderr, "Usage: ilma get <package-name>\n");
        return 1;
    }

    /* Download registry */
    char* registry = fetch_registry();
    if (!registry) {
        printf("Could not reach package registry. Check your internet connection.\n");
        return 1;
    }

    /* Search for the package in the registry */
    const char* pos = registry;
    char name[MAX_PKG_NAME];
    char version[64];
    char description[256];
    char url[MAX_URL];
    int found = 0;

    /* Iterate through package entries */
    while ((pos = strstr(pos, "\"name\"")) != NULL) {
        const char* entry_start = pos;
        const char* next;

        next = json_extract_string(entry_start, "name", name, sizeof(name));
        if (!next) { pos++; continue; }

        json_extract_string(entry_start, "version", version, sizeof(version));
        json_extract_string(entry_start, "description", description, sizeof(description));
        json_extract_string(entry_start, "url", url, sizeof(url));

        if (strcmp(name, package_name) == 0) {
            found = 1;
            break;
        }
        pos = next;
    }

    if (!found) {
        printf("Package '%s' not found. Run 'ilma packages --available'\n", package_name);
        free(registry);
        return 1;
    }

    printf("Installing %s (v%s)...\n", name, version);

    /* Download the tarball */
    char tarball_path[MAX_PATH_LEN];
    snprintf(tarball_path, sizeof(tarball_path), "/tmp/ilma_pkg_%s.tar.gz", name);

    if (download_file(url, tarball_path) != 0) {
        printf("Could not reach package registry. Check your internet connection.\n");
        free(registry);
        return 1;
    }

    /* Create ~/.ilma/packages/<name>/ */
    char pkg_dir[MAX_PATH_LEN];
    char packages_dir[MAX_PATH_LEN];
    if (get_packages_dir(packages_dir, sizeof(packages_dir)) != 0) {
        free(registry);
        return 1;
    }
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/%s", packages_dir, name);
    mkdirs(pkg_dir);

    /* Extract the tarball */
    char cmd[MAX_PATH_LEN * 2 + 64];
    snprintf(cmd, sizeof(cmd), "tar -xzf \"%s\" -C \"%s\" 2>/dev/null", tarball_path, pkg_dir);
    int ret = system(cmd);

    /* Clean up tarball */
    unlink(tarball_path);

    if (ret != 0) {
        printf("Failed to extract package '%s'.\n", name);
        free(registry);
        return 1;
    }

    printf("Installed %s successfully\n", name);
    free(registry);
    return 0;
}

/* ═════════════════════════════════════════════════════════════
 *  ilma_pkg_list_installed — Show packages in ~/.ilma/packages/
 * ═════════════════════════════════════════════════════════════ */

int ilma_pkg_list_installed(void) {
    char packages_dir[MAX_PATH_LEN];
    if (get_packages_dir(packages_dir, sizeof(packages_dir)) != 0) {
        return 1;
    }

    DIR* dir = opendir(packages_dir);
    if (!dir) {
        printf("No packages installed. Try: ilma get math\n");
        return 0;
    }

    struct dirent* entry;
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (entry->d_name[0] == '.') continue;

        /* Only list directories */
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "%s/%s", packages_dir, entry->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            printf("  %s\n", entry->d_name);
            count++;
        }
    }
    closedir(dir);

    if (count == 0) {
        printf("No packages installed. Try: ilma get math\n");
    }

    return 0;
}

/* ═════════════════════════════════════════════════════════════
 *  ilma_pkg_list_available — Download registry and list packages
 * ═════════════════════════════════════════════════════════════ */

int ilma_pkg_list_available(void) {
    char* registry = fetch_registry();
    if (!registry) {
        printf("Could not reach package registry. Check your internet connection.\n");
        return 1;
    }

    printf("Available packages:\n\n");

    const char* pos = registry;
    char name[MAX_PKG_NAME];
    char version[64];
    char description[256];
    int count = 0;

    while ((pos = strstr(pos, "\"name\"")) != NULL) {
        const char* entry_start = pos;
        const char* next;

        next = json_extract_string(entry_start, "name", name, sizeof(name));
        if (!next) { pos++; continue; }

        json_extract_string(entry_start, "version", version, sizeof(version));
        json_extract_string(entry_start, "description", description, sizeof(description));

        printf("  %s (v%s) — %s\n", name, version, description);
        count++;
        pos = next;
    }

    if (count == 0) {
        printf("  (no packages found in registry)\n");
    }

    printf("\nInstall with: ilma get <package-name>\n");
    free(registry);
    return 0;
}
