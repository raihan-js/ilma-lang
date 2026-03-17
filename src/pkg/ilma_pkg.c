/*
 * ilma_pkg.c — ILMA package manager
 *
 * Downloads packages from ilma-lang.dev/packages/,
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

#define DEFAULT_REGISTRY_URL "https://www.ilma-lang.dev/packages/registry.json"
#define REGISTRY_TMP         "/tmp/ilma_registry.json"
#define MAX_PKG_NAME         128
#define MAX_URL              1024
#define MAX_LINE             2048
#define MAX_PATH_LEN         512

/* ── Helpers ──────────────────────────────────────────────── */

static const char* get_registry_url(void) {
    const char* env = getenv("ILMA_REGISTRY");
    return env ? env : DEFAULT_REGISTRY_URL;
}

static int get_packages_dir(char* buf, size_t buf_size) {
    const char* home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "Could not determine home directory.\n");
        return -1;
    }
    snprintf(buf, buf_size, "%s/.ilma/packages", home);
    return 0;
}

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

static int download_file(const char* url, const char* dest) {
    char cmd[MAX_URL + MAX_PATH_LEN + 128];
    /* Try curl first */
    snprintf(cmd, sizeof(cmd),
             "curl -sL --max-time 30 \"%s\" -o \"%s\" 2>/dev/null",
             url, dest);
    if (system(cmd) == 0) {
        struct stat st;
        if (stat(dest, &st) == 0 && st.st_size > 0) return 0;
    }
    /* Fall back to wget */
    snprintf(cmd, sizeof(cmd),
             "wget -q --timeout=30 -O \"%s\" \"%s\" 2>/dev/null",
             dest, url);
    if (system(cmd) == 0) {
        struct stat st;
        if (stat(dest, &st) == 0 && st.st_size > 0) return 0;
    }
    return -1;
}

/* For file:// URLs: copy the local file */
static int download_or_copy(const char* url, const char* dest) {
    if (strncmp(url, "file://", 7) == 0) {
        const char* path = url + 7;
        char cmd[MAX_URL + MAX_PATH_LEN + 32];
        snprintf(cmd, sizeof(cmd), "cp \"%s\" \"%s\" 2>/dev/null", path, dest);
        if (system(cmd) == 0) {
            struct stat st;
            if (stat(dest, &st) == 0 && st.st_size > 0) return 0;
        }
        return -1;
    }
    return download_file(url, dest);
}

static char* read_file_pkg(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len <= 0) { fclose(f); return NULL; }
    char* buf = malloc((size_t)len + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, (size_t)len, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

/*
 * Extract a JSON string value for a given key.
 * Looks for "key": "value" starting from pos.
 */
static const char* json_extract_string(const char* pos, const char* key,
                                        char* out, size_t out_size) {
    char pattern[MAX_PKG_NAME + 8];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* found = strstr(pos, pattern);
    if (!found) return NULL;
    found += strlen(pattern);
    while (*found && (*found == ' ' || *found == ':' || *found == '\t' || *found == '\n' || *found == '\r'))
        found++;
    if (*found != '"') return NULL;
    found++;
    size_t i = 0;
    while (*found && *found != '"' && i < out_size - 1)
        out[i++] = *found++;
    out[i] = '\0';
    if (*found == '"') found++;
    return found;
}

/* Build a package download URL from the base_url in registry */
static void build_pkg_url(const char* registry_json, const char* name,
                           const char* registry_url,
                           char* url_out, size_t url_size,
                           char* pkg_json_url_out, size_t pkg_json_url_size) {
    /* Try to extract base_url from registry */
    char base_url[MAX_URL];
    base_url[0] = '\0';
    json_extract_string(registry_json, "base_url", base_url, sizeof(base_url));

    if (base_url[0] == '\0') {
        /* Derive base_url from registry_url: strip /registry.json */
        strncpy(base_url, registry_url, sizeof(base_url) - 1);
        char* slash = strrchr(base_url, '/');
        if (slash) *slash = '\0';
    }

    /* For file:// URLs, adjust path */
    if (strncmp(base_url, "file://", 7) == 0) {
        snprintf(url_out, url_size, "%s/%s/%s.ilma", base_url, name, name);
        snprintf(pkg_json_url_out, pkg_json_url_size, "%s/%s/package.json", base_url, name);
    } else {
        snprintf(url_out, url_size, "%s/%s/%s.ilma", base_url, name, name);
        snprintf(pkg_json_url_out, pkg_json_url_size, "%s/%s/package.json", base_url, name);
    }
}

/* ── Fetch registry ────────────────────────────────────────── */

static char* fetch_registry(void) {
    const char* registry_url = get_registry_url();

    /* Handle file:// URLs locally */
    if (strncmp(registry_url, "file://", 7) == 0) {
        char* content = read_file_pkg(registry_url + 7);
        if (!content) {
            fprintf(stderr, "Cannot read local registry: %s\n", registry_url + 7);
        }
        return content;
    }

    if (download_file(registry_url, REGISTRY_TMP) != 0) {
        return NULL;
    }
    return read_file_pkg(REGISTRY_TMP);
}

/* ═════════════════════════════════════════════════════════════
 *  ilma_pkg_install — Download and install a package by name
 * ═════════════════════════════════════════════════════════════ */

int ilma_pkg_install(const char* package_name) {
    if (!package_name || package_name[0] == '\0') {
        fprintf(stderr, "Usage: ilma get <package-name>\n");
        return 1;
    }

    const char* registry_url = get_registry_url();

    printf("Fetching registry...\n");
    char* registry = fetch_registry();
    if (!registry) {
        fprintf(stderr,
            "Could not reach package registry.\n"
            "Check your internet connection or set ILMA_REGISTRY to a local path.\n"
            "Registry: %s\n", registry_url);
        return 1;
    }

    /* Search for the package */
    const char* pos = registry;
    char name[MAX_PKG_NAME];
    char version[64];
    char description[512];
    char url_field[MAX_URL];
    int found = 0;

    while ((pos = strstr(pos, "\"name\"")) != NULL) {
        const char* entry_start = pos;
        const char* next;
        next = json_extract_string(entry_start, "name", name, sizeof(name));
        if (!next) { pos++; continue; }
        json_extract_string(entry_start, "version", version, sizeof(version));
        json_extract_string(entry_start, "description", description, sizeof(description));
        json_extract_string(entry_start, "url", url_field, sizeof(url_field));
        if (strcmp(name, package_name) == 0) { found = 1; break; }
        pos = next;
    }

    if (!found) {
        fprintf(stderr, "Package '%s' not found in registry.\n", package_name);
        fprintf(stderr, "Run: ilma packages --available\n");
        free(registry);
        return 1;
    }

    printf("Installing %s v%s...\n", name, version);

    /* Build download URL */
    char ilma_url[MAX_URL];
    char pkg_json_url[MAX_URL];

    if (url_field[0] != '\0') {
        /* Use the explicit URL from registry */
        strncpy(ilma_url, url_field, sizeof(ilma_url) - 1);
        /* Derive package.json URL */
        char* last_slash = strrchr(ilma_url, '/');
        if (last_slash) {
            size_t dir_len = (size_t)(last_slash - ilma_url);
            snprintf(pkg_json_url, sizeof(pkg_json_url), "%.*s/package.json",
                     (int)dir_len, ilma_url);
        } else {
            pkg_json_url[0] = '\0';
        }
    } else {
        build_pkg_url(registry, name, registry_url,
                      ilma_url, sizeof(ilma_url),
                      pkg_json_url, sizeof(pkg_json_url));
    }

    /* Create ~/.ilma/packages/<name>/ */
    char packages_dir[MAX_PATH_LEN];
    char pkg_dir[MAX_PATH_LEN];
    if (get_packages_dir(packages_dir, sizeof(packages_dir)) != 0) {
        free(registry);
        return 1;
    }
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/%s", packages_dir, name);
    mkdirs(pkg_dir);

    /* Download the .ilma file */
    char ilma_dest[MAX_PATH_LEN];
    snprintf(ilma_dest, sizeof(ilma_dest), "%s/%s.ilma", pkg_dir, name);

    if (download_or_copy(ilma_url, ilma_dest) != 0) {
        fprintf(stderr, "Failed to download %s\n", ilma_url);
        free(registry);
        return 1;
    }

    /* Download package.json (optional) */
    if (pkg_json_url[0] != '\0') {
        char pj_dest[MAX_PATH_LEN];
        snprintf(pj_dest, sizeof(pj_dest), "%s/package.json", pkg_dir);
        download_or_copy(pkg_json_url, pj_dest);
        /* Don't fail if package.json not found */
    }

    printf("\xe2\x9c\x93 Installed %s \xe2\x86\x92 %s\n", name, ilma_dest);
    printf("  Use with: use %s\n", name);
    free(registry);
    return 0;
}

/* ═════════════════════════════════════════════════════════════
 *  ilma_pkg_list_installed
 * ═════════════════════════════════════════════════════════════ */

int ilma_pkg_list_installed(void) {
    char packages_dir[MAX_PATH_LEN];
    if (get_packages_dir(packages_dir, sizeof(packages_dir)) != 0) return 1;

    DIR* dir = opendir(packages_dir);
    if (!dir) {
        printf("No packages installed. Try: ilma get math\n");
        return 0;
    }

    struct dirent* entry;
    int count = 0;
    printf("Installed packages:\n\n");

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "%s/%s", packages_dir, entry->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            /* Try to read version from package.json */
            char pj_path[MAX_PATH_LEN];
            snprintf(pj_path, sizeof(pj_path), "%s/%s/package.json", packages_dir, entry->d_name);
            char* pj = read_file_pkg(pj_path);
            char ver[64] = "?";
            if (pj) {
                json_extract_string(pj, "version", ver, sizeof(ver));
                free(pj);
            }
            printf("  %-20s v%s\n", entry->d_name, ver);
            count++;
        }
    }
    closedir(dir);

    if (count == 0) {
        printf("No packages installed. Try: ilma get math\n");
    } else {
        printf("\n%d package(s) installed.\n", count);
    }
    return 0;
}

/* ═════════════════════════════════════════════════════════════
 *  ilma_pkg_list_available
 * ═════════════════════════════════════════════════════════════ */

int ilma_pkg_list_available(void) {
    char* registry = fetch_registry();
    if (!registry) {
        fprintf(stderr,
            "Could not reach package registry.\n"
            "Set ILMA_REGISTRY=file:///path/to/packages/registry.json for local testing.\n");
        return 1;
    }

    printf("Available packages:\n\n");

    const char* pos = registry;
    char name[MAX_PKG_NAME];
    char version[64];
    char description[512];
    int count = 0;

    /* Skip the top-level fields before the packages array */
    const char* packages_start = strstr(pos, "\"packages\"");
    if (packages_start) pos = packages_start;

    while ((pos = strstr(pos, "\"name\"")) != NULL) {
        const char* entry_start = pos;
        const char* next;
        next = json_extract_string(entry_start, "name", name, sizeof(name));
        if (!next) { pos++; continue; }
        /* Skip top-level "name" field which is the package name key itself */
        version[0] = '\0';
        description[0] = '\0';
        json_extract_string(entry_start, "version", version, sizeof(version));
        json_extract_string(entry_start, "description", description, sizeof(description));
        if (version[0] != '\0') {
            printf("  %-20s v%-8s %s\n", name, version, description);
            count++;
        }
        pos = next;
    }

    if (count == 0) {
        printf("  (no packages found in registry)\n");
    } else {
        printf("\n%d package(s) available. Install with: ilma get <package-name>\n", count);
    }

    free(registry);
    return 0;
}
