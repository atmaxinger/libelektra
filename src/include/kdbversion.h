/***************************************************************************
          version.c  -  A plugin which returns version information
                             -------------------
 *  begin                : Wed 23 Jul, 2010
 *  copyright            : (C) 2010 by Markus Raab
 *  email                : elektra@markus-raab.org
 ***************************************************************************/

@DISCLAMER@

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the BSD License (revised).                      *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <kdb.h>

static inline KeySet *elektraVersionSet ()
{
	return ksNew (50, keyNew ("system/elektra/version",
			KEY_VALUE, "Below are version information of the Elektra Library you are currently using", KEY_END),
		keyNew ("system/elektra/version/constants", KEY_END),
		keyNew ("system/elektra/version/constants/KDB_VERSION",
			KEY_VALUE, "@KDB_VERSION@", KEY_END),
		keyNew ("system/elektra/version/constants/KDB_VERSION_MAJOR",
			KEY_VALUE, "@KDB_VERSION_MAJOR@", KEY_END),
		keyNew ("system/elektra/version/constants/KDB_VERSION_MINOR",
			KEY_VALUE, "@KDB_VERSION_MINOR@", KEY_END),
		keyNew ("system/elektra/version/constants/KDB_VERSION_MICRO",
			KEY_VALUE, "@KDB_VERSION_MICRO@", KEY_END),
		keyNew ("system/elektra/version/constants/KDB_VERSION_POSTFIX",
			KEY_VALUE, "@KDB_VERSION_POSTFIX@", KEY_END),
		keyNew ("system/elektra/version/constants/KDB_VERSION_MAJORMINOR",
			KEY_VALUE, "@KDB_VERSION_MAJORMINOR@", KEY_END),
		keyNew ("system/elektra/version/constants/SO_VERSION",
			KEY_VALUE, "@SO_VERSION@", KEY_END),
		keyNew ("system/elektra/version/infos",
			KEY_VALUE, "All information you want to know", KEY_END),
		keyNew ("system/elektra/version/infos/author",
			KEY_VALUE, "Markus Raab <elektra@markus-raab.org>", KEY_END),
		keyNew ("system/elektra/version/infos/licence",
			KEY_VALUE, "BSD", KEY_END),
		keyNew ("system/elektra/version/infos/description",
			KEY_VALUE, "Information of your Elektra Installation", KEY_END),
		keyNew ("system/elektra/version/infos/version",
			KEY_VALUE, PLUGINVERSION, KEY_END),
		KS_END);
}

