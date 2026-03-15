#ifndef ILMA_PKG_H
#define ILMA_PKG_H

int ilma_pkg_install(const char* package_name);
int ilma_pkg_list_installed(void);
int ilma_pkg_list_available(void);

#endif
