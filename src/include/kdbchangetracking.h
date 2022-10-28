#ifndef KDBCHANGETRACKING_H
#define KDBCHANGETRACKING_H

#include <kdb.h>
#include <kdbtypes.h>

#ifdef __cplusplus
namespace ckdb
{
extern "C" {
#endif

typedef struct _ChangeTrackingContext ChangeTrackingContext;

bool elektraChangeTrackingIsEnabled (KDB * kdb);
ChangeTrackingContext * elektraChangeTrackingGetContext (KDB * kdb, Key * parentKey);

KeySet * elektraChangeTrackingGetAddedKeys (ChangeTrackingContext * context);
KeySet * elektraChangeTrackingGetRemovedKeys (ChangeTrackingContext * context);
KeySet * elektraChangeTrackingGetModifiedKeys (ChangeTrackingContext * context);

bool elektraChangeTrackingValueChanged (ChangeTrackingContext * context, Key * key);
bool elektraChangeTrackingMetaChanged (ChangeTrackingContext * context, Key * key);

KeySet * elektraChangeTrackingGetAddedMetaKeys (ChangeTrackingContext * context, Key * key);
KeySet * elektraChangeTrackingGetRemovedMetaKeys (ChangeTrackingContext * context, Key * key);
KeySet * elektraChangeTrackingGetModifiedMetaKeys (ChangeTrackingContext * context, Key * key);

Key * elektraChangeTrackingGetOriginalKey (ChangeTrackingContext * context, Key * key);
const Key * elektraChangeTrackingGetOriginalMetaKey (ChangeTrackingContext * context, Key * key, const char * metaName);

/**
 * For use in unit tests only. Creates a hard-coded fake instance
 * TODO (atmaxinger): Maybe put in its own header (kdbchangetracking-testing.h)?
 */
ChangeTrackingContext * elektraChangeTrackingCreateMock (KeySet * newKeys, KeySet * oldKeys, Key * parentKey);
void elektraChangeTrackingFreeMock (ChangeTrackingContext * context);

#ifdef __cplusplus
}
}
#endif


#endif // KDBCHANGETRACKING_H
