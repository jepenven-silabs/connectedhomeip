# Matter All-Devices Application — Silabs (EFR32)

This is the Silabs/EFR32 target for the `all-devices-app`. Refer to the
top-level [all-devices-app README](../README.md) for a description of the
Code-Driven paradigm, supported device types, CLI concepts, and the overall
documentation suite under [`../docs/`](../docs/).

Unlike the Linux/POSIX variant, the Silabs image cannot select device types
from `argv` at boot. Instead the topology is chosen in one of two ways:

1. **Runtime (Matter shell)** — flash the default image and use the `devtype`
   shell command to store a single device type in KVS. The image will
   instantiate that device on the next boot.
2. **Build time (no shell required)** — bake a fixed, ordered list of device
   types into the image via GN args. This is the recommended flow for
   product-like builds and enables running multiple device types on
   consecutive endpoints without any Matter-shell interaction.

The build-time selection is driven by two GN args declared in
[all-devices-common/device-factory/enabled_devices.gni](../all-devices-common/device-factory/enabled_devices.gni):

-   `all_devices_enabled_devices` — subset of device types compiled into the
    `DeviceFactory`. Empty (default) compiles all of them in.
-   `all_devices_default_devices` — ordered list of device types instantiated
    at boot, one per endpoint starting at endpoint 1. Empty (default) means
    "fall back to the runtime KVS override / factory default single device".

Every entry in `all_devices_default_devices` must also be present in
`all_devices_enabled_devices` (unless the enabled list is empty). GN
enforces this at `gen` time.

## Build command (humidity + temperature sensor on BRD4187C)

```bash
./scripts/examples/gn_silabs_example.sh \
    examples/all-devices-app/silabs \
    out/all-devices-app \
    BRD4187C \
    'all_devices_enabled_devices=["humidity-sensor","temperature-sensor"]' \
    'all_devices_default_devices=["humidity-sensor","temperature-sensor"]' \
    'all_devices_app_name="humidity-and-temp-sensor"' \
    'chip_build_libshell=false'
```

Result:

-   Output image: `out/all-devices-app/thread/BRD4187C/humidity-and-temp-sensor.out`
-   Endpoint layout on boot:

    ```text
    Endpoint 0 → Root Node
    Endpoint 1 → humidity-sensor
    Endpoint 2 → temperature-sensor
    ```

No Matter shell interaction is needed to select the device types — the
mapping is fixed at build time. Dropping `chip_build_libshell=false` keeps
the `devtype` shell command available but has no effect on the pre-baked
topology (the build-time list takes precedence over any KVS override).

## Building a single device type

Same recipe with a single-element list, e.g. a temperature sensor only:

```bash
./scripts/examples/gn_silabs_example.sh \
    examples/all-devices-app/silabs \
    out/all-devices-app \
    BRD4187C \
    'all_devices_enabled_devices=["temperature-sensor"]' \
    'all_devices_default_devices=["temperature-sensor"]' \
    'all_devices_app_name="temperature-sensor-app"' \
    'chip_build_libshell=false'
```

## Default image (runtime device selection)

Omitting both args produces the full `matter-silabs-all-devices-example.out`
image with every device type registered in the factory and runtime
selection via the shell:

```bash
./scripts/examples/gn_silabs_example.sh \
    examples/all-devices-app/silabs \
    out/all-devices-app \
    BRD4187C
```

Then, from the device shell:

```text
matterCli> devtype list
matterCli> devtype set humidity-sensor
matterCli> reboot
```

## Building as an Intermittently Connected Device (ICD)

Append the `-icd` modifier to the target name (or pass
`chip_enable_icd_server=true chip_openthread_ftd=false` directly to
`gn_silabs_example.sh`) to produce an image that advertises as an ICD:

```bash
./scripts/build/build_examples.py \
    --target efr32-brd4187c-all-devices-icd \
    build
```

When `CHIP_CONFIG_ENABLE_ICD_SERVER=1` at compile time the app additionally:

-   Registers the `ICDManagement` cluster on the root endpoint. The
    `ICDManager` itself lives on `chip::Server` and is initialized
    automatically by the Matter server.
-   Instantiates an extra `power-source` endpoint alongside the selected
    device so commissioners (e.g. Home Assistant) can display a battery
    level (`BatPercentRemaining`) and battery voltage (`BatVoltage`).

The extra `power-source` endpoint is added transparently — if the build-time
device list already contains `"power-source"`, no duplicate endpoint is
added.

Combine `-icd` with the other modifiers just like any other Silabs target,
e.g.:

```bash
./scripts/build/build_examples.py \
    --target efr32-brd4187c-all-devices-humidity-sensor-icd-shell \
    build
```

To switch a device back to a non-ICD build, simply drop the `-icd` modifier
(or set `chip_enable_icd_server=false`).
