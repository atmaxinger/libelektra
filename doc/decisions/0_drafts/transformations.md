# Transformations

## Problem

Certain plugins, e.g. `rename`, perform extensive transformations on keys and keysets within Elektra.
Those transformations include, but are not limited to:

- Changing the names of the keys inbetween `kdbGet` and `kdbSet` operations
- Changing values back and forth for normalization, e.g. `true` -> `1`, `1` -> `true`
- Adding and removing metadata (e.g. the `spec` plugin)

While these features are useful, they do create feature-interaction problems.
More specifically, problems have been observed in conjunction with the following (overlapping) types of plugins:

- notification plugins
- plugins that do change tracking

The problem, in general, can be described as: Which phase of the KDB should be used for notifications/change tracking?

We differentiate between three types:

- *stored* name/value/metadata: How it is actually stored, i.e. the state returned by and passed to the `storage` plugins.
- *runtime* name/value/metadata: How it is at runtime, i.e. what is returned by `kdbGet` and passed to `kdbSet`.
- *intermediate* name/value/metadata: Any state inbetween the two.

### Observed problems with changing key names

Suppose there is a plugin that changes key names.
It converts to lowercase for the runtime representation, and to uppercase for the stored representation.
The plugin is executed in the post-storage phase for the `get` operation and the pre-storage phase for the `set` operation.
This results in the keys being in UPPERCASE in the configuration files, but they are presented in lowercase to other plugins and applications using the Elektra API.

For example, here is a configuration file with a hypothetical format:

```
/DISPLAY/BRIGHTNESS = 100
``` 

As can be seen, the keys are in UPPERCASE within the configuration file.
In Elektra keys are case-sensitive. 
As operations on keysets such as `ksLookup` operate with *runtime* data after the post-storage phase, `kdb get /DISPLAY/BRIGHTNESS` will fail.
For Elektra, the key `/DISPLAY/BRIGHTNESS` does not exist, as the `rename` plugin transformed this into the lowercase `/display/brightness`.

This leads to problems with the notification plugins.
As notification plugins are executed after the post-storage phase of the `set` operation, they will receive a keyset with the already transformed keys.
In this example, the notification plugins will receive all-UPPERCASE keys, and send out notifications with those all-UPPERCASE keys. 
An application listening to those notifications will not be able to query Elektra for those keys, as for Elektra those UPPERCASE keys do not exist.

Apart from the problems with notifications, the way key name changing plugins work also breaks change tracking in plugins like `dbus`.
This is because key names are read-only when they are contained in a `KeySet`.
In order to change the name of a key, such a plugin has to create a new key with the changed name, and delete the key with the old name.
The `dbus` plugin implements change tracking by checking the 'key needs sync' flag instead of comparing the values.
As new keys by design have the 'key needs sync' flag set, the plugins that implement change tracking via the flag will always erroneously detect transformed keys as changed.

### Observed problems with changing values (normalization)

Suppose we have a plugin that changes the value of a key to the hard-coded value `1` during the storage phase of the `set` operation.
If a plugin does change tracking, this will lead to false positives.

```
user:/limits/openfiles = 1
```

If the user changes the value, e.g. using `kdb set user:/limits/openfiles 23`, plugins will observe the new value `23`.
However, the value-changing plugin will reset that value back to `1`.
So in practice, the configuration has not been changed.
Plugins relying on change tracking plugins (e.g. notification plugins) will however think that it has.

### Observed problems with adding and removing metadata

This is a bit of a mixture between changing key names and changing values.

## Constraints

1.
2.
3.

## Assumptions

1. We want to enable some kind of transformations
2.
3.

## Considered Alternatives

### Enable setting and unsetting of the `keyNeedsSync` flag

Require plugins that rename keys to remove the `keyNeedsSync` flag.
This would require making the 'key sync' APIs public.
This does not solve the problem of changing values.

### Use change tracking to replace the `keyNeedsSync` flag

Eliminate the need of the `keyNeedsSync` flag by utilizing the planned change tracking API.
This does not solve the problem of changing values and false positives.

### Store original names and values in metadata.

For key name transformations require that transformation plugins set a metakey (e.g. `meta:/elektra/runtimename`) with the runtime name before they do any transformations in the `kdbSet` phase.
This _must not_ be done if this metakey already exists, i.e. another plugin already tranformed it beforehand.
This allows notification and change tracking functionality to work determine and work with the runtime name of the key.

A similiar thing was already attempted for values, i.e. `meta:/origvalue`.


## Decision

## Rationale

## Implications

-
-
-

## Related Decisions

- [Hooks](../4_partially_implemented/hooks.md)
- [Change Tracking](../0_drafts/change_tracking.md)
- [Operation Sequences](../0_drafts/operation_sequences.md)
- [Internal Cache](../3_decided/internal_cache.md)

## Notes

- [Issue #404](https://issues.libelektra.org/404) - dbus and rename plugin do not work together
- [Issue #955](https://issues.libelektra.org/955) - dbus: non-UTF8 key names 
- [Issue #3626](https://issues.libelektra.org/3626) - origvalue