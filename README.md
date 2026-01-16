# Axon_ON

A Flipper Zero application that triggers Axon Body Cameras to start recording by broadcasting BLE commands.

## Features

- **BLE Broadcasting**: Sends recording commands to Axon Body Cameras in range
- **Targeted Commands**: Uses the exact same protocol as the Android AxonCadabra app
- **Clean UI**: Simple interface with Start/Stop controls and navigation
- **About & Credits**: Information screens about the app and its creators

## How It Works

The Flipper Zero broadcasts BLE advertisement packets containing:
- Service UUID: `0xFE6C` (0000FE6C-0000-1000-8000-00805F9B34FB)
- Service Data: Recording command payload
- Target OUI: `00:25:DF` (Axon Body Cameras)

Any Axon Body Camera within BLE range will receive and execute the recording command when the app broadcasts.

## Installation

### Method 1: qFlipper (Recommended)
1. Open qFlipper on your computer
2. Connect your Flipper Zero
3. Go to **File Manager** → **apps** → **Bluetooth**
4. Drag and drop the `axon_on.fap` file into the Bluetooth folder
5. The app will appear in your Flipper's Bluetooth apps menu

### Method 2: Manual SD Card Transfer
1. Safely eject your Flipper Zero's SD card
2. Insert it into your computer
3. Copy `axon_on.fap` to: `SD_Card/apps/Bluetooth/`
4. Safely eject and reinsert into Flipper Zero
5. Reboot your Flipper Zero (hold BACK + LEFT for 3 seconds)

## Usage

1. On your Flipper Zero, go to **Apps** → **Bluetooth** → **Axon On**
2. Press **OK** to start broadcasting the recording command
3. Press **OK** again to stop broadcasting
4. Use **LEFT/RIGHT** arrows to access About and Credits screens

## Building from Source

### Prerequisites
- Flipper Zero firmware development environment
- Access to Flipper Zero firmware repository

### Build Instructions
```bash
cd flipper-zero-firmware
./fbt fap_axon_on
```

The compiled FAP file will be available at:
`build/f7-firmware-C/.extapps/axon_on.fap`

## Technical Details

### BLE Protocol
- **Service UUID**: 0xFE6C
- **Service Data**: 24-byte command payload
- **Advertising Mode**: Undirected broadcasting
- **Target Devices**: OUI 00:25:DF (Axon Body Cameras)

### Compatibility
- Tested with Flipper Zero firmware
- Compatible with Axon Body Camera BLE interface
- Uses standard BLE GAP advertising

## Credits

**Created by Kara**
- Email: kara@netslum.io

This application is based on the work of the [AxonCadabra Android app](https://github.com/WithLoveFromMinneapolis/AxonCadabra). 

## License

See LICENSE file for details.

## Disclaimer

This application is for research and educational purposes. Use responsibly and in accordance with applicable laws and regulations.