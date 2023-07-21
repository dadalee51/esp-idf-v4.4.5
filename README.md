<<<<<<< HEAD
# robotics_idf
forking the espidf repo for class
=======
# Using Espressif IoT Development Framework with the ESP32

# Prerequisites

# Configuring your project

`make menuconfig`

# Compiling your project

`make app`

# Flashing the Bootloader

ESP32 has a bootloader in ROM which runs after reset, but ESP-IDF also uses a second stage software bootloader. The ROM bootloader loads the software bootloader, which then loads the firmware app of the ESP32. The software bootloader must be flashed to offset 0x5000 in the flash.

To build the software bootloader, navigate to your project's top-level directory and run:

``` shell
make bootloader
```

If you've configured the serial port details in `make menuconfig`, then

``` shell
make bootloader-flash
```

... will automatically run esptool.py to flash the image. Otherwise, you can customise the `esptool.py` command that is printed out as part of `make bootloader`.

You only need to flash the ESP32 bootloader once.

# The Partition Table

Once you've compiled your project, the "build" directory will contain a binary file with a name like "my_app.bin". This is an ESP32 image binary that can be loaded by the bootloader.

A single ESP32's flash can contain multiple apps, as well as many different kinds of data (calibration data, filesystems, parameter storage, etc). For this reason a partition table is flashed to offset 0x4000 in the flash.

Each entry in the partition table has a name (label), type (app, data, or something else), subtype and the offset in flash where the partition is loaded.

The simplest way to use the partition table is to `make menuconfig` and choose one of the simple predefined partition tables:

* "Single factory app, no OTA"
* "Factory app, two OTA definitions"

In both cases the factory app is flashed at offset 0x10000. If you `make partition_table` then it will print a summary of the partition table.

Here is the summary printed for the "Single factory app, no OTA" configuration:

```
# Espressif ESP32 Partition Table
# Name,  Type, SubType, Offset,  Size
factory, app,  factory, 0x10000, 1M
rfdata,  data, rf,     0x110000, 256K
wifidata,data, wifi,   0x150000, 256K
```

* At a 0x10000 (64KB) offset in the flash is the app labelled "factory". The bootloader will run this app by default.
* There are also two data regions defined in the partition table for storing RF & Wifi calibration data.

Here is the summary printed for the "Factory app, two OTA definitions" configuration:

```
# Espressif ESP32 Partition Table
# Name,  Type, SubType, Offset,  Size
factory, app,  factory,  0x10000, 1M
ota_0,   app,  ota_0,   0x110000, 1M
ota_1,   app,  ota_1,   0x210000, 1M
rfdata,  data, rf,      0x310000, 256K
wifidata,data, wifi,    0x350000, 256K
otadata, data, ota,     0x390000, 256K
```

* There are now three app partition definitions.
* The type of all three are set as "app", but the subtype varies between the factory app at 0x10000 and the next two "OTA" apps.
* There is also a new "ota data" slot, which holds the data for OTA updates. The bootloader consults this data in order to know which app to execute. If "ota data" is empty, it will execute the factory app.


## Creating Custom Tables

If you choose "Custom partition table CSV" in menuconfig then you can also enter the name of a CSV file (in the project directory) to use for your partition table. The CSV file can describe any number of definitions for the table you need.

The CSV format is the same format as printed in the summaries shown above. However, not all fields are required in the CSV. For example, here is the "input" CSV for the OTA partition table:

```
# Name,   Type, SubType, Offset,   Size
factory,  app,  factory, 0x10000,  1M
ota_0,    app,  ota_0,   ,         1M
ota_1,    app,  ota_1,   ,         1M
rfdata,   data, rf,      ,         256K
wifidata, data, wifi,    ,         256K
otadata,  data, ota,     ,         256K
```

* Whitespace between fields is ignored, and so is any line starting with # (comments).
* Each non-comment line in the CSV file is a partition definition.
* Only the offset for the first partition is supplied. The gen_esp32part.py tool fills in each remaining offset to start after the preceding partition.

### Name field

Name field can be any meaningful name. It is not significant to the ESP32. Names longer than 16 characters will be truncated.

### Type field

Type field can be specified as app (0) or data (1). Or it can be a number 0-254 (or as hex 0x00-0xFE). Types 0x00-0x3F are reserved for Espressif. If your application needs to store data, please add a custom partition type in the range 0x40-0xFE.

The bootloader ignores any types other than 0 & 1.

### Subtype

When type is "app", the subtype field can be specified as factory (0), ota_0 (0x10) ... ota_15 (0x1F) and test (0x20). Or it can be any number 0-255 (0x00-0xFF). The bootloader will execute the factory app unless there it sees a partition of type data/ota, in which case it reads this partition to determine which OTA image to boot

When type is "data", the subtype field can be specified as ota (0), rf (1), wifi (2). Or it can be a number 0x00-0xFF. The bootloader ignores all data subtypes except for ota. Other "data" subtypes are reserved for Espressif use. To create custom data partition subtypes then use a custom type value, and choose any subtype 0x00-0xFF.

### Offset & Size

Only the first offset field is required (we recommend using 0x10000). Partitions with blank offsets will start after the previous partition.

App partitions have to be at offsets aligned to 0x10000 (64K). If you leave the offset field blank, the tool will automatically align the partition. If you specify an unaligned offset for an app partition, the tool will return an error.

Sizes and offsets can be specified as decimal numbers, hex numbers with the prefix 0x, or size multipliers M or K (1024 and 1024*1024 bytes).

## Generating Binary Partition Table

The partition table which is flashed to the ESP32 is in a binary format, not CSV. The tool bin/gen_esp32part.py is used to convert between CSV and binary formats.

If you configure the partition table CSV name in `make menuconfig` and then `make partition_table`, this conversion is done for you.

To convert CSV to Binary manually:

``` shell
python bin/gen_esp32part.py --verify input_partitions.csv binary_partitions.bin
```

To convert binary format back to CSV:

``` shell
python bin/gen_esp32part.py --verify binary_partitions.bin input_partitions.csv
```

To display the contents of a binary partition table on stdout (this is how the summaries displayed when running `make partition_table` are generated:

``` shell
python bin/gen_esp32part.py binary_partitions.bin
```

`gen_esp32part.py` takes one optional argument, `--verify`, which will also verify the partition table during conversion (checking for overlapping partitions, unaligned partitions, etc.)

# Flashing the partition table

The command `make partition_table-flash` will flash the partition table with esptool.py. However a manual flashing command is printed as part of `make partition_table`.
>>>>>>> 9e0c80688 (Fix gen_esp32part.py locations & documentation (thanks @wujiangang\!))
