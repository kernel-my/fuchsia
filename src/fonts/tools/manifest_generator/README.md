# Font Manifest Generator

The font manifest generator creates a `.font_manifest.json` file for use by the font server.

To see usage instructions, run
```shell
./out/default/host_x64/font_manifest_generator --help
```

## Inputs

### 1. `*.font_catalog.json`

Contains a human-written listing of font families, assets, and typefaces. See
[schema](../schemas/font_catalog.schema.json).

One `.font_catalog.json` file exists for each CIPD font repo.

These files are expected to reside in the Fuchsia source tree.

### 2. `*.font_pkgs.json`

Contains a listing of font asset names (e.g. `"Roboto-Bold.ttf"`, safe package
name suffixes (e.g. `"roboto-bold-ttf"`), and the relative path of the asset
within the [font directory](#4_font-directory).

One `.font_pkgs.json` file exists for each CIPD font repo. It is generated by
a recipe and checked out as a prebuilt, along with font files.

### 3. `*.font_sets.json`

Contains a listing of `base` and `universe` font asset names, indicating which
font packages are part of the OTA image and which are ephemeral packages.

This file determines which font assets from the catalogs actually end up in the
generated manifest.

This file is generated by the GN `font_set` template. <sup>[TODO(8892)][1]</sup>
There should be one `.font_sets.json` file per product target.

[1]: https://bugs.fuchsia.dev/p/fuchsia/issues/detail?id=8892

### 4. Font directory

This is usually a directory in `//prebuilt/third_party`, and it is where the
CIPD checkout places all of the font asset files and the `*.font_pkgs.json`
files.

The manifest generator must read the font files to collect the set of code
points for each typeface.

## Testing

1. Make sure your `fx set` arguments include `--with //bundles:tests`.
1. Execute `fx build`
1. Execute:
  ```shell
  fx run-host-tests font_manifest_generator_bin_test \
  font_manifest_generator_integration_tests
  ```