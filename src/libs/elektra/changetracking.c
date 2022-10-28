#include <kdbprivate.h>
#include <kdbchangetracking.h>

ChangeTrackingContext * elektraChangeTrackingContextNew (void)
{
	ChangeTrackingContext * context = elektraMalloc (sizeof(ChangeTrackingContext));
	context->addedKeys = ksNew (0, KS_END);
	context->removedKeys = ksNew (0, KS_END);
	context->modifiedKeys = ksNew (0, KS_END);
	context->oldValues = ksNew (0, KS_END);

	return context;
}

void elektraChangeTrackingContextFree(ChangeTrackingContext * context)
{
	ksDel (context->addedKeys);
	ksDel (context->removedKeys);
	ksDel (context->modifiedKeys);
	ksDel (context->oldValues);
	elektraFree (context);
}

/**
 * Determines whether two keys have different value
 *
 * @param new new key
 * @param old old key
 * @return true if @p new and @p old have a different value
 */
static bool keyValueDifferent (Key * new, Key * old)
{
	if (keyIsString (new) != keyIsString (old))
	{
		// different data types -> modified
		return true;
	}

	if (keyIsString (new))
	{
		if (strcmp (keyString (new), keyString (old)) != 0)
		{
			// different value -> modified
			return true;
		}
	}
	else
	{
		if (new->dataSize != old->dataSize)
		{
			// different datasize -> modified
			return true;
		}

		if (memcmp (new->data.v, old->data.v, new->dataSize) != 0)
		{
			// different value -> modified
			return true;
		}
	}

	return false;
}

/**
 * Find the differences between two keysets
 *
 * @param new the new keyset
 * @param old the old keyset
 * @param addedKeys adds keys present in @p new but not in @p old
 * @param removedKeys adds keys present in @p old but not in @p new
 * @param modifiedKeys adds keys present in both @p new and @p old, but with changes in value or in the meta keys
 * @param parentKey parent key - if this parameter is not NULL, only keys below or same are processed.
 */
static void findDifferences (KeySet * new, KeySet * old, KeySet * addedKeys, KeySet * removedKeys, KeySet * modifiedKeys, Key * parentKey)
{
	KeySet * metaAdded = ksNew (0, KS_END);
	KeySet * metaRemoved = ksNew(0, KS_END);
	KeySet * metaModified = ksNew(0, KS_END);

	for (elektraCursor itOld = 0; itOld < ksGetSize (old); itOld++)
	{
		Key * needle = ksAtCursor (old, itOld);

		if (parentKey != NULL && !keyIsBelowOrSame (parentKey, needle))
		{
			continue;
		}

		Key * found = ksLookup (new, needle, 0);

		if(found == NULL)
		{
			// key is present in old key set, but not in new --> removed
			ksAppendKey (removedKeys, needle);
		}
		else
		{
			// key is present in both new and old key sets --> check for modifications

			if (keyValueDifferent (found, needle))
			{
				// The value of the key changed --> modified
				ksAppendKey (modifiedKeys, found);
			}
			else if (keyGetNamespace (found) != KEY_NS_META)
			{
				// Check whether something in the meta keys has changed

				KeySet * oldMeta = keyMeta (needle);
				KeySet * newMeta = keyMeta (found);

				ksClear (metaAdded);
				ksClear (metaRemoved);
				ksClear (metaModified);

				findDifferences (newMeta, oldMeta, metaAdded, metaRemoved, metaModified, NULL);

				if (ksGetSize (addedKeys) > 0 || ksGetSize (removedKeys) > 0 || ksGetSize (modifiedKeys) > 0)
				{
					// there was a change in the meta keys --> modified
					ksAppendKey (modifiedKeys, found);
				}
			}
		}
	}

	for (elektraCursor itNew = 0; itNew < ksGetSize (new); itNew++)
	{
		Key * needle = ksAtCursor (new, itNew);

		if (parentKey != NULL && !keyIsBelowOrSame (parentKey, needle))
		{
			continue;
		}

		Key * found = ksLookup (old, needle, 0);

		if(found == NULL)
		{
			// Key is present in the new keyset but not in the old --> added
			ksAppendKey (addedKeys, needle);
		}
	}

	ksDel (metaAdded);
	ksDel (metaRemoved);
	ksDel (metaModified);
}

void elektraTrackChanges (KDB * handle, KeySet * newKeys, Key * parentKey)
{
	ChangeTrackingContext * context = handle->changeTrackingContext;

	if (context == NULL)
	{
		return;
	}

	ksClear (context->addedKeys);
	ksClear (context->removedKeys);
	ksClear (context->modifiedKeys);

	findDifferences (newKeys, context->oldValues, context->addedKeys, context->removedKeys, context->modifiedKeys, parentKey);
}

void elektraChangeTrackingReset (KDB * handle, const KeySet * keys, Key * parentKey)
{
	ChangeTrackingContext * context = handle->changeTrackingContext;

	if (context == NULL)
	{
		return;
	}

	ksClear (context->addedKeys);
	ksClear (context->removedKeys);
	ksClear (context->modifiedKeys);
	ksDel ( context->oldValues);

	context->oldValues = ksDeepDup (keys);
}

ChangeTrackingContext * elektraChangeTrackingCreateMock (KeySet * newKeys, KeySet * oldKeys, Key * parentKey)
{
	ChangeTrackingContext * context = elektraChangeTrackingContextNew();

	context->oldValues = ksDeepDup (oldKeys);
	findDifferences (newKeys, context->oldValues, context->addedKeys, context->removedKeys, context->modifiedKeys, parentKey);

	return context;
}

void elektraChangeTrackingFreeMock (ChangeTrackingContext * context)
{
	elektraChangeTrackingContextFree (context);
}


/**
 * Check whether change tracking is enabled on this KDB instance
 *
 * @param kdb the KDB instance
 * @return true if change tracking is enabled, false otherwise
 */
bool elektraChangeTrackingIsEnabled (KDB * kdb)
{
	if (kdb == NULL)
	{
		return false;
	}

	return kdb->changeTrackingContext != NULL;
}

/**
 * Gets the current change tracking context from the specified KDB instance.
 * Will only work, if change tracking is enabled on this instance
 *
 * @see elektraChangeTrackingIsEnabled
 *
 * @param kdb
 * @param parentKey
 * @return
 */
ChangeTrackingContext * elektraChangeTrackingGetContext (KDB * kdb, ELEKTRA_UNUSED /* for later user */ Key * parentKey)
{
	if (!elektraChangeTrackingIsEnabled (kdb))
	{
		return NULL;
	}

	return kdb->changeTrackingContext;
}

/**
 *
 * @param context
 * @return
 */
KeySet * elektraChangeTrackingGetAddedKeys (ChangeTrackingContext * context)
{
	return ksDup(context->addedKeys);
}

/**
 *
 * @param context
 * @return
 */
KeySet * elektraChangeTrackingGetRemovedKeys (ChangeTrackingContext * context)
{
	return ksDup(context->removedKeys);
}

/**
 *
 * @param context
 * @return
 */
KeySet * elektraChangeTrackingGetModifiedKeys (ChangeTrackingContext * context)
{
	return ksDup(context->modifiedKeys);
}

bool elektraChangeTrackingValueChanged (ChangeTrackingContext * context, Key * key)
{
	Key * new = ksLookup (context->modifiedKeys, key, 0);

	if (new == NULL)
	{
		return false;
	}

	Key * old = ksLookup (context->oldValues, key, 0);

	return keyValueDifferent (new, old);
}

bool elektraChangeTrackingMetaChanged (ChangeTrackingContext * context, Key * key)
{
	Key * new = ksLookup (context->modifiedKeys, key, 0);

	if (new == NULL)
	{
		return false;
	}

	Key * old = ksLookup (context->oldValues, key, 0);

	// if the value is not different -> meta is different
	return !keyValueDifferent (new, old);
}

KeySet * elektraChangeTrackingGetAddedMetaKeys (ChangeTrackingContext * context, Key * key)
{
	Key * oldKey = elektraChangeTrackingGetOriginalKey (context, key);

	if (oldKey == NULL)
	{
		return NULL;
	}

	KeySet * addedKeys = ksNew (0, KS_END);
	KeySet * removedKeys = ksNew (0, KS_END);
	KeySet * modifiedKeys = ksNew (0, KS_END);

	findDifferences (keyMeta (key), keyMeta (oldKey), addedKeys, removedKeys, modifiedKeys, NULL);

	ksDel (removedKeys);
	ksDel (modifiedKeys);

	return addedKeys;
}

KeySet * elektraChangeTrackingGetRemovedMetaKeys (ChangeTrackingContext * context, Key * key)
{
	Key * oldKey = elektraChangeTrackingGetOriginalKey (context, key);

	if (oldKey == NULL)
	{
		return NULL;
	}

	KeySet * addedKeys = ksNew (0, KS_END);
	KeySet * removedKeys = ksNew (0, KS_END);
	KeySet * modifiedKeys = ksNew (0, KS_END);

	findDifferences (keyMeta (key), keyMeta (oldKey), addedKeys, removedKeys, modifiedKeys, NULL);

	ksDel (addedKeys);
	ksDel (modifiedKeys);

	return removedKeys;
}

KeySet * elektraChangeTrackingGetModifiedMetaKeys (ChangeTrackingContext * context, Key * key)
{
	Key * oldKey = elektraChangeTrackingGetOriginalKey (context, key);

	if (oldKey == NULL)
	{
		return NULL;
	}

	KeySet * addedKeys = ksNew (0, KS_END);
	KeySet * removedKeys = ksNew (0, KS_END);
	KeySet * modifiedKeys = ksNew (0, KS_END);

	findDifferences (keyMeta (key), keyMeta (oldKey), addedKeys, removedKeys, modifiedKeys, NULL);

	ksDel (removedKeys);
	ksDel (addedKeys);

	return modifiedKeys;
}

Key * elektraChangeTrackingGetOriginalKey (ChangeTrackingContext * context, Key * key)
{
	return ksLookup (context->oldValues, key, 0);
}

const Key * elektraChangeTrackingGetOriginalMetaKey (ChangeTrackingContext * context, Key * key, const char * metaName)
{
	Key * oldKey = elektraChangeTrackingGetOriginalKey (context, key);

	if (oldKey == NULL)
	{
		return NULL;
	}

	return keyGetMeta (oldKey, metaName);
}
