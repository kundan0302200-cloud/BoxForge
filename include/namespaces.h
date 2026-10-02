#ifndef BOXFORGE_NAMESPACES_H
#define BOXFORGE_NAMESPACES_H
#include "boxforge.h"
int namespace_clone_flags(const ContainerConfig *cfg);
int namespace_configure_userns(pid_t pid, uid_t host_uid, gid_t host_gid);
int namespace_setup(ContainerConfig *cfg);
#endif
