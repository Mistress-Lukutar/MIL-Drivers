# MIL Drivers

Hardware abstraction layer (HAL) drivers for **Milandr** microcontrollers.
All modules use the `MIL_` prefix and are written in portable C99 with
CMSIS-style register access.

## Supported Platforms

| Platform | MCU | Status |
|----------|-----|--------|
| `MDR1986BE9x` | Milandr MDR1986BE91 / BE92 (ARM Cortex-M3) | ✅ Active |

The directory layout is platform-first, so support for additional MCU
families can be added without restructuring:

```
Drivers/
└── MDR1986BE9x/
    ├── MIL_ADC/
    ├── MIL_EBC/
    ├── MIL_GPIO/
    ├── MIL_I2C/
    ├── MIL_SPI/
    ├── MIL_TIMx/
    ├── MIL_Time/
    ├── MIL_Uart/
    └── MIL_eeprom/
```

## Modules

| Module | Version | Description |
|--------|---------|-------------|
| `MIL_ADC` | v1.0.0 | ADC configuration and conversion control |
| `MIL_EBC` | v1.0.0 | External Bus Controller setup |
| `MIL_GPIO` | v1.0.1 | GPIO init/read/write/toggle with analog mode support |
| `MIL_I2C` | v1.2.1 | I2C master, blocking and interrupt-driven, selectable pinout (PC0/PC1 or PE14/PE15) |
| `MIL_SPI` | v1.0.0 | SPI master |
| `MIL_TIMx` | v1.0.0 | General-purpose timer configuration |
| `MIL_Time` | v1.1.1 | SysTick-based timebase: `millis`, `micros`, blocking delays, configurable core clock |
| `MIL_Uart` | v2.1.1 | UART1/UART2 driver, half- and full-duplex, blocking and interrupt-driven TX/RX with ring buffers |
| `MIL_eeprom` | v1.2.3 | Internal Flash/EEPROM read/write/erase with timing-safe programming sequences |

Each module is self-contained in its own directory as a `.c` / `.h` pair.
Module versions are tracked per-module in the Doxygen file header
(`@version` / `@date`), not by a single global library version.

## Using the drivers in a project

Projects do **not** keep their own copies of these drivers. Instead, each
project pins a version of this repository in a `drivers.lock` file and runs
the sync tool [`tools/update_drivers.py`](tools/update_drivers.py).

**Bootstrap** (one-liner, no clone needed):

```powershell
irm https://raw.githubusercontent.com/Mistress-Lukutar/MIL-Drivers/main/tools/update_drivers.py | python - init --repo Mistress-Lukutar/MIL-Drivers --ref v2026.07.30
```

**Migrate** an existing project with old in-tree `Driver/` copies:

```bash
python tools/update_drivers.py migrate --repo Mistress-Lukutar/MIL-Drivers --ref v2026.07.30
```

The sync tool downloads this repository at the pinned ref, copies the
requested modules into the project's `Drivers/` directory, registers them
in a Keil uVision project (`.uvprojx`), self-updates, and records file
hashes in `drivers.lock`.

## Code style

All code is formatted with **clang-format** using the `.clang-format`
configuration in this repository (WebKit-based, 2-space indent,
150-column limit). Run before committing:

```bash
find Drivers -name "*.c" -o -name "*.h" | xargs clang-format -i
```

Naming conventions:

| Entity | Convention | Example |
|--------|-----------|---------|
| Public functions | `MIL_<Module>_PascalCase` | `MIL_GPIO_WritePin()` |
| Static (private) functions | `_camelCase` | `_waitTransmission()` |
| Variables | `snake_case` | `rx_head` |
| Constants / macros | `UPPER_CASE` | `UART_TX_BUFFER_SIZE` |
| Types | `MIL_<Module>_PascalCase_t` | `MIL_I2C_Handle_t` |

## Documentation

Public APIs are documented with Doxygen comments. Every file carries a
header block with `@file`, `@brief`, `@author`, `@date`, `@version` —
keep `@date` and `@version` in sync with the actual change.

## Versioning

- Each module is versioned independently (semver, `vMAJOR.MINOR.PATCH`).
- Breaking API change → **MAJOR**; new backward-compatible feature →
  **MINOR**; bugfix → **PATCH**.
- Repository tags (e.g. `v2026.07.30`) mark tested snapshots of the whole
  collection and are what projects pin in `drivers.lock`.

## License

TBD
