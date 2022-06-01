/**
 * @file
 *
 * @brief Header for simplelogger plugin
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 *
 */

#ifndef ELEKTRA_PLUGIN_SIMPLELOGGER_H
#define ELEKTRA_PLUGIN_SIMPLELOGGER_H

#include <kdbplugin.h>


int elektraSimpleloggerOpen (Plugin * handle, Key * errorKey);
int elektraSimpleloggerClose (Plugin * handle, Key * errorKey);
int elektraSimpleloggerGet (Plugin * handle, KeySet * ks, Key * parentKey);
int elektraSimpleloggerSet (Plugin * handle, KeySet * ks, Key * parentKey);
int elektraSimpleloggerError (Plugin * handle, KeySet * ks, Key * parentKey);
int elektraSimpleloggerCommit (Plugin * handle, KeySet * ks, Key * parentKey);
int elektraSimpleloggerCheckConf (Key * errorKey, KeySet * conf);

Plugin * ELEKTRA_PLUGIN_EXPORT;

#endif
