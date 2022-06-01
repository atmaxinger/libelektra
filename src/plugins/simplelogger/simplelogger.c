/**
 * @file
 *
 * @brief Source for simplelogger plugin
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 *
 */

#include "simplelogger.h"

#include <kdbhelper.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


int elektraSimpleloggerOpen (Plugin * handle ELEKTRA_UNUSED, Key * errorKey ELEKTRA_UNUSED)
{
	// plugin initialization logic
	// this function is optional

	return ELEKTRA_PLUGIN_STATUS_SUCCESS;
}

int elektraSimpleloggerClose (Plugin * handle ELEKTRA_UNUSED, Key * errorKey ELEKTRA_UNUSED)
{
	// free all plugin resources and shut it down
	// this function is optional

	return ELEKTRA_PLUGIN_STATUS_SUCCESS;
}

int elektraSimpleloggerGet (Plugin * handle ELEKTRA_UNUSED, KeySet * returned, Key * parentKey)
{
	if (!elektraStrCmp (keyName (parentKey), "system:/elektra/modules/simplelogger"))
	{
		KeySet * contract =
			ksNew (30, keyNew ("system:/elektra/modules/simplelogger", KEY_VALUE, "simplelogger plugin waits for your orders", KEY_END),
			       keyNew ("system:/elektra/modules/simplelogger/exports", KEY_END),
			       keyNew ("system:/elektra/modules/simplelogger/exports/open", KEY_FUNC, elektraSimpleloggerOpen, KEY_END),
			       keyNew ("system:/elektra/modules/simplelogger/exports/close", KEY_FUNC, elektraSimpleloggerClose, KEY_END),
			       keyNew ("system:/elektra/modules/simplelogger/exports/get", KEY_FUNC, elektraSimpleloggerGet, KEY_END),
			       keyNew ("system:/elektra/modules/simplelogger/exports/set", KEY_FUNC, elektraSimpleloggerSet, KEY_END),
			       keyNew ("system:/elektra/modules/simplelogger/exports/commit", KEY_FUNC, elektraSimpleloggerCommit, KEY_END),
			       keyNew ("system:/elektra/modules/simplelogger/exports/error", KEY_FUNC, elektraSimpleloggerError, KEY_END),
			       keyNew ("system:/elektra/modules/simplelogger/exports/checkconf", KEY_FUNC, elektraSimpleloggerCheckConf, KEY_END),
#include ELEKTRA_README
			       keyNew ("system:/elektra/modules/simplelogger/infos/version", KEY_VALUE, PLUGINVERSION, KEY_END), KS_END);
		ksAppend (returned, contract);
		ksDel (contract);

		return ELEKTRA_PLUGIN_STATUS_SUCCESS;
	}
	// get all keys

	return ELEKTRA_PLUGIN_STATUS_NO_UPDATE;
}

int elektraSimpleloggerSet (Plugin * handle ELEKTRA_UNUSED, KeySet * returned ELEKTRA_UNUSED, Key * parentKey ELEKTRA_UNUSED)
{
	// set all keys
	// this function is optional

	return ELEKTRA_PLUGIN_STATUS_NO_UPDATE;
}

int elektraSimpleloggerError (Plugin * handle ELEKTRA_UNUSED, KeySet * returned ELEKTRA_UNUSED, Key * parentKey ELEKTRA_UNUSED)
{
	// handle errors (commit failed)
	// this function is optional

	return ELEKTRA_PLUGIN_STATUS_SUCCESS;
}

static bool is_modified(Key* current, Key* old)
{
	bool modified = false;

	if(keyIsString (current) && keyIsString (old))
	{
		modified = strcmp (keyString (current), keyString(old)) != 0;
	}
	else if (!keyIsString (current) && !keyIsString (old))
	{
		// TODO
		modified = true;
	}
	else
	{
		// Data Type Changed
		modified = true;
	}

	return modified;
}

static void logValue(KeySet* protocol, const char* sessionName, const char* currentTime, const char* keyName, const char* role, const Key* key)
{
	Key* logNewValueKey = keyNew("user:/simplelogger/sessions", KEY_END);
	keyAddBaseName (logNewValueKey, sessionName);
	keyAddBaseName (logNewValueKey, currentTime);
	keyAddBaseName (logNewValueKey, keyName);
	keyAddBaseName (logNewValueKey, role);

	if(keyIsString (key))
	{
		keySetString(logNewValueKey, keyString(key));
	}
	else
	{
		keySetBinary (logNewValueKey, keyValue (key), keyGetValueSize (key));
	}

	ksAppendKey (protocol, logNewValueKey);
}

int elektraSimpleloggerCommit (Plugin * handle ELEKTRA_UNUSED, KeySet * returned, Key * parentKey ELEKTRA_UNUSED)
{
	// Determine current UTC time
	time_t timer = time (NULL);
	struct tm* ptm = gmtime (&timer);
	time_t utcTimer = mktime (ptm);
	char currentTime[64] = "";
	snprintf (currentTime, 64, "%ld", utcTimer);

	// Load required configuration for logging
	Key* kLoggerRoot = keyNew("user:/simplelogger", KEY_END);
	KDB * kdbLogger = kdbOpen (NULL, kLoggerRoot);
	KeySet * ksProtocol = ksNew (0, KS_END);
	kdbGet (kdbLogger, ksProtocol, kLoggerRoot);

	// Determine current session
	Key* kCurrentSession = ksLookupByName (ksProtocol, "/simplelogger/current_session_name", 0);
	if(kCurrentSession == NULL)
	{
		printf("no key for current_session_name --> not logging\n");
		// TODO (atmaxinger): Cleanup
		return ELEKTRA_PLUGIN_STATUS_SUCCESS;
	}

	const char* sessionName = keyString (kCurrentSession);

	// Read all existing keys
	KeySet* ksOldKeys = ksNew (0, KS_END);
	Key *kOldRoot = keyNew (keyName (parentKey), KEY_END);
	KDB * kdbOld = kdbOpen (NULL, kOldRoot);
	kdbGet (kdbOld, ksOldKeys, kOldRoot);
	kdbClose (kdbOld, 0);

	// Handle modified and added keys
	for(Key* k = ksNext (returned); k != NULL; k = ksNext(returned))
	{
		if(keyIsBelowOrSame(k, kLoggerRoot))
		{
			// Ignore logger entries
			continue ;
		}

		Key* existingKey = ksLookupByName (ksOldKeys, keyName (k), 0);

		Key* logKey = keyNew("user:/simplelogger/sessions", KEY_END);
		keyAddBaseName (logKey, sessionName);
		keyAddBaseName (logKey, currentTime);
		keyAddBaseName (logKey, keyName (k));

		bool log = false;
		bool logNewValue = false;
		bool logOldValue = false;

		if(existingKey == NULL)
		{
			printf("Added: %s -> %s\n", keyName(k), keyString (k));

			keySetString (logKey, "created");
			log = true;
			logNewValue = true;
		}
		else if(is_modified(k, existingKey))
		{
			printf("Modified: %s -> %s\n", keyName(k), keyString (k));

			keySetString (logKey, "modified");
			log = true;
			logNewValue = true;
			logOldValue = true;
		}
		else
		{
			log = false;
		}

		if(log)
		{
			ksAppendKey (ksProtocol, logKey);

			if(logNewValue)
			{
				logValue (ksProtocol, sessionName, currentTime, keyName (k), "new", k);
			}

			if(logOldValue)
			{
				logValue (ksProtocol, sessionName, currentTime, keyName (k), "old", existingKey);
			}
		}
		else
		{
			keyDel (logKey);
		}
	}

	// Handle modified and added keys
	ksRewind (ksOldKeys);
	for(Key* ok = ksNext (ksOldKeys); ok != NULL; ok = ksNext(ksOldKeys))
	{
		if(keyIsBelowOrSame(ok, kLoggerRoot))
		{
			// Ignore logger entries
			continue ;
		}

		ksRewind (returned);
		bool found = false;
		for(Key* k = ksNext (returned); k != NULL; k = ksNext(returned))
		{
			if(strcmp (keyName (ok), keyName (k)) == 0)
			{
				found = true;
				break;
			}
		}

		if(!found)
		{
			printf("Deleted: %s\n", keyName(ok));

			Key* logKey = keyNew("user:/simplelogger/sessions", KEY_END);
			keyAddBaseName (logKey, sessionName);
			keyAddBaseName (logKey, currentTime);
			keyAddBaseName (logKey, keyName (ok));
			keySetString (logKey, "deleted");
			ksAppendKey (ksProtocol, logKey);

			logValue (ksProtocol, sessionName, currentTime, keyName (ok), "old", ok);
		}
	}

	ksDel (ksOldKeys);
	keyDel (kOldRoot);

	if(kdbSet (kdbLogger, ksProtocol, kLoggerRoot) == -1)
	{
		fprintf(stderr, "Error during logging\n");
		const Key *err;
		while((err = keyNextMeta (kLoggerRoot)) != NULL)
		{
			fprintf (stderr, "%s\n", keyString (err));
		}
	}

	kdbClose (kdbLogger, 0);
	ksDel (ksProtocol);
	keyDel (kLoggerRoot);

	return ELEKTRA_PLUGIN_STATUS_SUCCESS;
}

int elektraSimpleloggerCheckConf (Key * errorKey ELEKTRA_UNUSED, KeySet * conf ELEKTRA_UNUSED)
{
	// validate plugin configuration
	// this function is optional

	return ELEKTRA_PLUGIN_STATUS_NO_UPDATE;
}

Plugin * ELEKTRA_PLUGIN_EXPORT
{
	// clang-format off
	return elektraPluginExport ("simplelogger",
		ELEKTRA_PLUGIN_OPEN,	&elektraSimpleloggerOpen,
		ELEKTRA_PLUGIN_CLOSE,	&elektraSimpleloggerClose,
		ELEKTRA_PLUGIN_GET,	&elektraSimpleloggerGet,
		ELEKTRA_PLUGIN_SET,	&elektraSimpleloggerSet,
		ELEKTRA_PLUGIN_COMMIT,  &elektraSimpleloggerCommit,
		ELEKTRA_PLUGIN_ERROR,	&elektraSimpleloggerError,
		ELEKTRA_PLUGIN_END);
}
