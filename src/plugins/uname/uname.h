/**
 * @file
 *
 * @brief
 *
 * @copyright BSD License (see doc/COPYING or http://www.libelektra.org)
 */

#ifndef UNAME_H
#define UNAME_H

#include <kdbplugin.h>
#include <kdbextension.h>

int elektraUnameGet(Plugin *handle, KeySet *returned, Key *parentKey);
int elektraUnameSet(Plugin *handle, KeySet *ks, Key *parentKey);

#endif
