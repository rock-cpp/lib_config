# lib_config Documentation

## Bundles
`lib_config` supports Rock's bundle mechanisms. A Bundle is a predefined 
folder structure, where a certain places specific information s supposed 
to be found.

Following the folder structure as expectd from `lib_config` is documented. 
Thereby all folders are give *relative to the root folder of a bundle*:

## `config/orogen`
YAML-like files containing configuration properties for components of a 
certain prototype. The name of a file within the folder has to correspond
with the name of the prototype it contains the configuration properties for

TIP: **Example:**
The file `config/orogen/joint_dispatcher::Task.yml` contains the properties
for the Rock-Task `joint:dispatcher::Task`.

## `data`
Directory used for not further specified data.

TODO: **TODO:**
Clarify if and where in the code this folder is used and evaulate the reason.

## `logs`
The data logger per default outputs its log files to this folder. `lib_config`
provides an API function `Bundle::createLogDirectory()` that creates a new 
log directory in the `logs` folder using the following pattern: 
`logs/%Y%m%d-%H%M`.

If the function is called multiple times within the same minute, the name gets 
extended with a suffix: `logs/%Y%m%d-%H%M.i`, where `i` 

## `logs/current`
When the API function `Bundle::createLogDirectory()` is called (see `logs`),
a symlink called `logs/current` is created that points to the folder that
has just been created. Thus the symlink should always point to the most recently 
created log folder.

## Configuration
## Configuration files
### `config/bundle.yml`
This files YAML contains the configration properties of a bundle in form of a 
map called `bundle`. The following properties are supported:

 * `dependencies`: A list of bundle names containing data that is required for
   the present bundle to work.

TIP: **Example:**
```YAML
# Bundle-specific configuration
bundle:
    # List of bundles we depend on
    dependencies:
        - other_bundle
        - yet_another_bundle

```

If the file is not present, it is assumed, that the bundle depends on no other
bundles.

We have a breadth-first discovery behaviour of dependencies. This means that
first level dependecies are considered more relvant than second level
dependencies. 
TIP: **Example:**
An example: This dependency tree
```
     bundle_1
       |- bundle_2
            |- bundle_4
       |- bundle_3
```
is resolved to this priorization:
```
      1. bundle_1
      2. bundle_2
      3. bundle_3
      4. bundle_4
``` 

## Environment Variables
The following environment Variables are used

### `ROCK_BUNDLE`
Configures the currently selected bundle. `lib_config` allos to specify the 
bundle either by its name (i.e. folder-name of the Bundle visible within 
`ROCK_BUNDLE_PATH`) or absolute path.

TIP: **Example:**
Both examples
```bash
ROCK_BUNDLE_PATH=/home/malte/rock/bundles
ROCK_BUNDLE=my_bundle
```
and
```bash
ROCK_BUNDLE=/home/malte/rock/bundles/my_bundle
```
refer to the same bundle at `/home/malte/rock/bundles/my_bundle` for 
`lib_config`.

### `ROCK_BUNDLE_PATH`
Defines a set of absolute folder-paths which are searched for bundles. Multiple 
paths are separated by thhe `:`-symbol.

TIP: **Example:**
```bash
ROCK_BUNDLE_PATH=/home/malte/rock/bundles:/home/malte/rock/other_bundles
```