/**
 * @file
 *
 * @brief Private declarations.
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 */

#ifndef KDBPRIVATE_H
#define KDBPRIVATE_H

#include <elektra.h>
#include <elektra/error.h>
#include <kdb.h>
#include <kdbextension.h>
#include <kdbhelper.h>
#include <kdbio.h>
#include <kdbmacros.h>
#include <kdbnotificationinternal.h>
#include <kdbplugin.h>
#include <kdbtypes.h>
#include <kdbchangetracking.h>
#ifdef ELEKTRA_ENABLE_OPTIMIZATIONS
#include <kdbopmphm.h>
#include <kdbopmphmpredictor.h>
#endif

#include <limits.h>

/** The minimal allocation size of a keyset inclusive
	NULL byte. ksGetAlloc() will return one less because
	it says how much can actually be stored.*/
#define KEYSET_SIZE 16

/** Trie optimization */
#define APPROXIMATE_NR_OF_BACKENDS 16

/** The maximum value of unsigned char+1, needed
 *  for iteration over trie children/values:
 *
 *  for (i=0; i<KDB_MAX_UCHAR; ++i)
 * */
#define KDB_MAX_UCHAR (UCHAR_MAX + 1)


/**The maximum of how many characters an integer
  needs as decimal number.*/
#define MAX_LEN_INT 31

/**Backend mounting information.
 *
 * This key directory tells you where each backend is mounted
 * to which mountpoint. */
#define KDB_SYSTEM_ELEKTRA "system:/elektra"

/** All keys below this are used for cache metadata in the global keyset */
#define KDB_CACHE_PREFIX "system:/elektra/cache"


#ifdef __cplusplus
namespace ckdb
{
extern "C" {
#endif


/* These define the type for pointers to all the kdb functions */
typedef int (*kdbOpenPtr) (Plugin *, Key * errorKey);
typedef int (*kdbClosePtr) (Plugin *, Key * errorKey);

typedef int (*kdbInitPtr) (Plugin * handle, KeySet * definition, Key * parentKey);
typedef int (*kdbGetPtr) (Plugin * handle, KeySet * returned, Key * parentKey);
typedef int (*kdbSetPtr) (Plugin * handle, KeySet * returned, Key * parentKey);
typedef int (*kdbErrorPtr) (Plugin * handle, KeySet * returned, Key * parentKey);
typedef int (*kdbCommitPtr) (Plugin * handle, KeySet * returned, Key * parentKey);

typedef int (*kdbHookGoptsGetPtr) (Plugin * handle, KeySet * returned, Key * parentKey);

typedef int (*kdbHookSpecCopyPtr) (Plugin * handle, KeySet * returned, Key * parentKey, bool isKdbGet);
typedef int (*kdbHookSpecRemovePtr) (Plugin * handle, KeySet * returned, Key * parentKey);

typedef int (*kdbHookSendNotificationGetPtr) (Plugin * handle, KeySet * returned, Key * parentKey);
typedef int (*kdbHookSendNotificationSetPtr) (Plugin * handle, KeySet * returned, Key * parentKey);

typedef Plugin * (*OpenMapper) (const char *, const char *, KeySet *);
typedef int (*CloseMapper) (Plugin *);


/*****************
 * Key Flags
 *****************/

enum
{
	KEY_EMPTY_NAME = 1 << 22
};

// clang-format off

/**
 * Key Flags.
 *
 * Store a synchronizer state so that the Elektra knows if something
 * has changed or not.
 *
 * @ingroup backend
 */
typedef enum {
	KEY_FLAG_SYNC = 1,	  /*!<
			Key need sync.
			If name, value or metadata
			are changed this flag will be set, so that the backend will sync
			the key to database.*/
	KEY_FLAG_RO_NAME = 1 << 1,	/*!<
			 Read only flag for name.
			 Key name is read only and not allowed
			 to be changed. All attempts to change the name
			 will lead to an error.
			 Needed for metakeys and keys that are in a data
			 structure that depends on name ordering.*/
	KEY_FLAG_RO_VALUE = 1 << 2, /*!<
			 Read only flag for value.
			 Key value is read only and not allowed
			 to be changed. All attempts to change the value
			 will lead to an error.
			 Needed for metakeys*/
	KEY_FLAG_RO_META = 1 << 3,	/*!<
			 Read only flag for meta.
			 Key meta is read only and not allowed
			 to be changed. All attempts to change the value
			 will lead to an error.
			 Needed for metakeys.*/
	KEY_FLAG_MMAP_STRUCT = 1 << 4,	/*!<
			 Key struct lies inside a mmap region.
			 This flag is set for Keys inside a mapped region.
			 It prevents erroneous free() calls on these keys. */
	KEY_FLAG_MMAP_KEY = 1 << 5,	/*!<
			 Key name lies inside a mmap region.
			 This flag is set once a Key name has been moved to a mapped region,
			 and is removed if the name moves out of the mapped region.
			 It prevents erroneous free() calls on these keys. */
	KEY_FLAG_MMAP_DATA = 1 << 6	/*!<
			 Key value lies inside a mmap region.
			 This flag is set once a Key value has been moved to a mapped region,
			 and is removed if the value moves out of the mapped region.
			 It prevents erroneous free() calls on these keys. */
} keyflag_t;


/**
 * Advanced KS Flags.
 *
 * Store a synchronizer state so that the Elektra knows if something
 * has changed or not.
 *
 * @ingroup backend
 */
typedef enum {
	KS_FLAG_SYNC = 1 /*!<
		 KeySet need sync.
		 If keys were popped from the Keyset
		 this flag will be set, so that the backend will sync
		 the keys to database.*/
#ifdef ELEKTRA_ENABLE_OPTIMIZATIONS
	,KS_FLAG_NAME_CHANGE = 1 << 1 /*!<
		 The OPMPHM needs to be rebuild.
		 Every Key add, Key removal or Key name change operation
		 sets this flag.*/
#endif
	,KS_FLAG_MMAP_STRUCT = 1 << 2	/*!<
		 KeySet struct lies inside a mmap region.
		 This flag is set for KeySets inside a mapped region.
		 It prevents erroneous free() calls on these KeySets. */
	,KS_FLAG_MMAP_ARRAY = 1 << 3	/*!<
		 Array of the KeySet lies inside a mmap region.
		 This flag is set for KeySets where the array is in a mapped region,
		 and is removed if the array is moved out from the mapped region.
		 It prevents erroneous free() calls on these arrays. */
} ksflag_t;


/**
 * The private Key struct.
 *
 * Its internal private attributes should not be accessed directly by regular
 * programs. Use the @ref key "Key access methods" instead.
 * Only a backend writer needs to have access to the private attributes of the
 * Key object which is defined as:
 * @code
typedef struct _Key Key;
 * @endcode
 *
 * @ingroup backend
 */
struct _Key
{
	/**
	 * The value, which is a NULL terminated string or binary.
	 * @see keyString(), keyBinary(),
	 * @see keyGetString(), keyGetBinary(),
	 * @see keySetString(), keySetBinary()
	 */
	union {
		char * c;
		void * v;
	} data;

	/**
	 * Size of the value, in bytes, including ending NULL.
	 * @see keyGetValueSize()
	 */
	size_t dataSize;

	/**
	 * The canonical (escaped) name of the key.
	 * @see keyGetName(), keySetName()
	 */
	char * key;

	/**
	 * Size of the name, in bytes, including ending NULL.
	 * @see keyGetName(), keyGetNameSize(), keySetName()
	 */
	size_t keySize;

	/**
	 * The unescaped name of the key.
	 * Note: This is NOT a standard null-terminated string.
	 * @see keyGetName(), keySetName()
	 */
	char * ukey;

	/**
	 * Size of the unescaped key name in bytes, including all NULL.
	 * @see keyBaseName(), keyUnescapedName()
	 */
	size_t keyUSize;

	/**
	 * All the key's meta information.
	 */
	KeySet * meta;

	/**
	 * Some control and internal flags.
	 */
	keyflag_t flags;

	/**
	 * Reference counter
	 */
	uint16_t refs;

	/**
	 * Reserved for future use
	 */
	uint16_t reserved;
};


/**
 * The private KeySet structure.
 *
 * Its internal private attributes should not be accessed directly by regular
 * programs. Use the @ref keyset "KeySet access methods" instead.
 * Only a backend writer needs to have access to the private attributes of the
 * KeySet object which is defined as:
 * @code
typedef struct _KeySet KeySet;
 * @endcode
 *
 * @ingroup backend
 */
struct _KeySet
{
	struct _Key ** array; /**<Array which holds the keys */

	size_t size;  /**< Number of keys contained in the KeySet */
	size_t alloc; /**< Allocated size of array */

	struct _Key * cursor; /**< Internal cursor */
	size_t current;		  /**< Current position of cursor */

	/**
	 * Some control and internal flags.
	 */
	ksflag_t flags;

	uint16_t refs; /**< Reference counter */

	uint16_t reserved; /**< Reserved for future use */

#ifdef ELEKTRA_ENABLE_OPTIMIZATIONS
	/**
	 * The Order Preserving Minimal Perfect Hash Map.
	 */
	Opmphm * opmphm;
	/**
	 * The Order Preserving Minimal Perfect Hash Map Predictor.
	 */
	OpmphmPredictor * opmphmPredictor;
#endif
};

typedef struct _SendNotificationHook
{
	struct _Plugin * plugin;
	struct _SendNotificationHook * next;

	/**
	 * Optional, may be NULL
	 */
	kdbHookSendNotificationGetPtr get;

	/**
	 * Optional, may be NULL
	 */
	kdbHookSendNotificationSetPtr set;
} SendNotificationHook;

/**
 * The access point to the key database.
 *
 * The structure which holds all information about loaded backends.
 *
 * Its internal private attributes should not be accessed directly.
 *
 * See kdb mount tool to mount new backends.
 *
 * KDB object is defined as:
 * @code
typedef struct _KDB KDB;
 * @endcode
 *
 * @see kdbOpen() and kdbClose() for external use
 * @ingroup backend
 */

struct _KDB
{
	KeySet * modules; /*!< A list of all modules loaded at the moment.*/

	KeySet * global; /*!< This keyset can be used by plugins to pass data through
			the KDB and communicate with other plugins. Plugins shall clean
			up their parts of the global keyset, which they do not need any more.*/

	KeySet * backends;

	struct
	{
		struct
		{
			struct _Plugin * plugin;
			kdbHookGoptsGetPtr get;
		} gopts;

		struct
		{
			struct _Plugin * plugin;
			kdbHookSpecCopyPtr copy;
			kdbHookSpecRemovePtr remove;
		} spec;

		struct _SendNotificationHook * sendNotification;
	} hooks;

	ChangeTrackingContext * changeTrackingContext;
};

/**
 * Holds all information related to a plugin.
 *
 * Since Elektra 0.8 a Backend consists of many plugins.
 *
 * A plugin should be reusable and only implement a single concern.
 * Plugins which are supplied with Elektra are located below src/plugins.
 * It is no problem that plugins are developed external too.
 *
 * @ingroup backend
 */
struct _Plugin
{
	KeySet * config; /*!< This keyset contains configuration for the plugin.
	 Direct below system:/ there is the configuration supplied for the backend.
	 Direct below user:/ there is the configuration supplied just for the
	 plugin, which should be of course preferred to the backend configuration.
	 The keys inside contain information like /path which path should be used
	 to write configuration to or /host to which host packets should be send.
	 @see elektraPluginGetConfig() */

	kdbOpenPtr kdbOpen;   /*!< The pointer to kdbOpen_template() of the backend. */
	kdbClosePtr kdbClose; /*!< The pointer to kdbClose_template() of the backend. */

	kdbInitPtr kdbInit;	  /*!< The pointer to kdbInit_template() of the backend. */
	kdbGetPtr kdbGet;	  /*!< The pointer to kdbGet_template() of the backend. */
	kdbSetPtr kdbSet;	  /*!< The pointer to kdbSet_template() of the backend. */
	kdbErrorPtr kdbError; /*!< The pointer to kdbError_template() of the backend. */
	kdbCommitPtr kdbCommit; /*!< The pointer to kdbCommit_template() of the backend. */

	const char * name; /*!< The name of the module responsible for that plugin. */

	size_t refcounter; /*!< This refcounter shows how often the plugin
	   is used.  Not shared plugins have 1 in it */

	void * data; /*!< This handle can be used for a plugin to store
	 any data its want to. */

	KeySet * global; /*!< This keyset can be used by plugins to pass data through
			the KDB and communicate with other plugins. Plugins shall clean
			up their parts of the global keyset, which they do not need any more.*/

	KeySet * modules; /*!< A list of all currently loaded modules.*/
};

/**
 * Holds all information to a specific change tracking
 */
struct _ChangeTrackingContext
{
	KeySet * addedKeys;
	KeySet * modifiedKeys;
	KeySet * removedKeys;

	/**
	 * Stores the old keys for the keys in modified keys
	 */
	KeySet * oldValues;
};

void elektraChangeTrackingReset (KDB * handle, const KeySet * keys, Key * parentKey);
void elektraTrackChanges (KDB * handle, KeySet * newKeys, Key * parentKey);
ChangeTrackingContext * elektraChangeTrackingContextNew (void);
void elektraChangeTrackingContextFree(ChangeTrackingContext * context);


/**
 * Holds all data for one backend.
 * 
 * This struct is used for the key values in @ref _KDB.backends
 * 
 * @ingroup backend
 */
typedef struct _BackendData
{
	struct _Plugin * backend; /*!< the backend plugin for this backend */
	struct _KeySet * keys; /*!< holds the keys for this backend, assigned by backendsDivide() */
	struct _KeySet * plugins; /*!< Holds all the plugins of this backend.
	 The key names are all `system:/<ref>` where `<ref>` is the same as in
	 `system:/elektra/mountpoints/<mp>/plugins/<ref>` */
	struct _KeySet * definition; /*!< Holds all the mountpoint definition of this backend.
	 This is a copy of `system:/elektra/mountpoints/<mp>/defintion` moved to `system:/` */
	size_t getSize; /*!< the size of @ref _BackendData.keys at the end of kdbGet()
	 More precisely this is set by backendsMerge() to the size of @ref _BackendData.keys */
	bool initialized; /*!< whether or not the init function of this backend has been called */
	bool keyNeedsSync; /*!< whether or not any key in this backend needs a sync (keyNeedSync())
	 More precisely this is set by backendsDivide() to indicate whether it encountered a key that needs sync */
} BackendData;

// clang-format on

/***************************************
 *
 * Not exported functions, for internal use only
 *
 **************************************/

/* Backends handling */
Key * backendsFindParent (KeySet * backends, const Key * key);
KeySet * backendsForParentKey (KeySet * backends, Key * parentKey);
bool backendsDivide (KeySet * backends, const KeySet * ks);
void backendsMerge (KeySet * backends, KeySet * ks);

/* Mountpoint parsing */
// visible for testing
KeySet * elektraMountpointsParse (KeySet * elektraKs, KeySet * modules, KeySet * global, Key * errorKey);

/* Plugin handling */
Plugin * elektraPluginOpen (const char * backendname, KeySet * modules, KeySet * config, Key * errorKey);
int elektraPluginClose (Plugin * handle, Key * errorKey);
size_t elektraPluginGetFunction (Plugin * plugin, const char * name);

/* Hooks handling */
int initHooks (KDB * kdb, const KeySet * config, KeySet * modules, const KeySet * contract, Key * errorKey);
void freeHooks (KDB * kdb, Key * errorKey);
Plugin * elektraFindInternalNotificationPlugin (KDB * kdb);


/*Private helper for key*/
ssize_t keySetRaw (Key * key, const void * newBinary, size_t dataSize);
void keyInit (Key * key);
int keyClearSync (Key * key);
int keyReplacePrefix (Key * key, const Key * oldPrefix, const Key * newPrefix);

/*Private helper for keyset*/
int ksInit (KeySet * ks);
int ksClose (KeySet * ks);

int ksResize (KeySet * ks, size_t size);
size_t ksGetAlloc (const KeySet * ks);
KeySet * ksDeepDup (const KeySet * source);

Key * elektraKsPopAtCursor (KeySet * ks, elektraCursor pos);

KeySet * ksRenameKeys (KeySet * config, const char * name);

ssize_t ksRename (KeySet * ks, const Key * root, const Key * newRoot);

elektraCursor ksFindHierarchy (const KeySet * ks, const Key * root, elektraCursor * end);
KeySet * ksBelow (const KeySet * ks, const Key * root);

/*Used for internal memcpy/memmove*/
ssize_t elektraMemcpy (Key ** array1, Key ** array2, size_t size);
ssize_t elektraMemmove (Key ** array1, Key ** array2, size_t size);

/*Internally used for array handling*/
int elektraReadArrayNumber (const char * baseName, kdb_long_long_t * oldIndex);

/* Conveniences Methods for Making Tests */

int keyIsSpec (const Key * key);
int keyIsProc (const Key * key);
int keyIsDir (const Key * key);
int keyIsSystem (const Key * key);
int keyIsUser (const Key * key);

elektraNamespace elektraReadNamespace (const char * namespaceStr, size_t len);

bool elektraKeyNameValidate (const char * name, bool isComplete);
void elektraKeyNameCanonicalize (const char * name, char ** canonicalName, size_t * canonicalSizePtr, size_t offset, size_t * usizePtr);
void elektraKeyNameUnescape (const char * name, char * unescapedName);
size_t elektraKeyNameEscapePart (const char * part, char ** escapedPart);

// TODO (kodebaach) [Q]: make public?
int elektraIsArrayPart (const char * namePart);

/** Test a bit. @see set_bit(), clear_bit() */
#define test_bit(var, bit) (((unsigned long long) (var)) & ((unsigned long long) (bit)))
/** Set a bit. @see clear_bit() */
#define set_bit(var, bit) ((var) |= ((unsigned long long) (bit)))
/** Clear a bit. @see set_bit() */
#define clear_bit(var, bit) ((var) &= ~((unsigned long long) (bit)))

#ifdef __cplusplus
}
}

#define KDB ckdb::KDB
#define Key ckdb::Key
#define KeySet ckdb::KeySet
extern "C" {
#endif

struct _Elektra
{
	KDB * kdb;
	Key * parentKey;
	KeySet * config;
	KeySet * defaults;
	Key * lookupKey;
	ElektraErrorHandler fatalErrorHandler;
	char * resolvedReference;
	size_t parentKeyLength;
};

struct _ElektraError
{
	char * code;
	char * codeFromKey;
	char * description;
	char * module;
	char * file;
	kdb_long_t line;
	kdb_long_t warningCount;
	kdb_long_t warningAlloc;
	struct _ElektraError ** warnings;
	Key * errorKey;
};

/* high-level API */
void elektraSaveKey (Elektra * elektra, Key * key, ElektraError ** error);
void elektraSetLookupKey (Elektra * elektra, const char * name);
void elektraSetArrayLookupKey (Elektra * elektra, const char * name, kdb_long_long_t index);

ElektraError * elektraErrorCreate (const char * code, const char * description, const char * module, const char * file, kdb_long_t line);
void elektraErrorAddWarning (ElektraError * error, ElektraError * warning);
ElektraError * elektraErrorFromKey (Key * key);

ElektraError * elektraErrorKeyNotFound (const char * keyname);
ElektraError * elektraErrorWrongType (const char * keyname, KDBType expectedType, KDBType actualType);
ElektraError * elektraErrorNullError (const char * function);
ElektraError * elektraErrorEnsureFailed (const char * reason);

#ifdef __cplusplus
}
#undef Key
#undef KeySet
#undef KDB
#endif

#endif /* KDBPRIVATE_H */
