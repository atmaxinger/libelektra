- infos = Information about the simplelogger plugin is in keys below
- infos/author = Author Name <elektra@libelektra.org>
- infos/licence = BSD
- infos/needs =
- infos/provides = simplelogger
- infos/recommends =
- infos/placements = precommit
- infos/status = maintained nodep global nodoc
- infos/metadata =
- infos/description = one-line description of simplelogger

## Introduction

Copy this simplelogger if you want to start a new
plugin written in C.

## Installation

See [installation](/doc/INSTALL.md).
The package is called `libelektra5-experimental`.

## Usage

You can use `scripts/copy-simplelogger`
to automatically rename everything to your
plugin name:

```bash
cd src/plugins
../../scripts/copy-simplelogger yourplugin
```

Then update the README.md of your newly created plugin:

- enter your full name+email in `infos/author`
- make sure `status`, `placements`, and other clauses conform to
  descriptions in `doc/CONTRACT.ini`
- update the one-line description above
- add your plugin in `src/plugins/README.md`
- and rewrite the rest of this `README.md` to give a great
  explanation of what your plugin does

## Dependencies

None.

## Examples

```sh
# Backup-and-Restore: user:/tests/simplelogger

kdb set user:/tests/simplelogger/key value
#> Create a new key user:/tests/simplelogger/key with string "value"

kdb get /tests/simplelogger/key
#> value
```

## Limitations

None.
