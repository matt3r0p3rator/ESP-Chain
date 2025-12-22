# BadUSB Module

The BadUSB module allows the ESP-Chain to emulate a USB keyboard and execute DuckyScript payloads. This can be used for automating tasks or testing security on target machines.

## Features

- **DuckyScript Support**: Parses and executes standard DuckyScript files.
- **SD Card Storage**: Loads payloads from the `payloads/` directory on the SD card.
- **File Browser**: Navigate folders and select scripts directly from the device screen.
- **Startup Delay**: Configurable delay before script execution to ensure the USB driver is loaded on the target.
- **Status Feedback**: On-screen feedback for Armed, Waiting, Running, and Done states.

## Usage

1.  **Prepare Payloads**:
    - Create DuckyScript files (text files) with your desired commands.
    - Save them to the `payloads/` folder on the SD card. You can organize them into subfolders (e.g., `windows`, `linux`).

2.  **Run a Payload**:
    - Open the **BadUSB** module from the main menu.
    - Browse the file list and select a payload file.
    - The screen will show **ARMED**.
    - Connect the ESP-Chain to the target computer via USB.
    - The device will wait for the configured startup delay (default is usually a few seconds). You can double-click the button to skip the delay if the device is already recognized.
    - The script will execute automatically.
    - Once finished, the screen will show **DONE**.

## Supported DuckyScript Commands

The following commands are supported by the parser:

- `REM [comment]`: Comments (ignored).
- `DELAY [ms]`: Pauses execution for the specified milliseconds.
- `DEFAULTDELAY [ms]` or `DEFAULT_DELAY [ms]`: Sets the default delay between commands.
- `STRING [text]`: Types the specified text.
- `GUI` or `WINDOWS`: Press the Windows/Command key. Can be combined with a key (e.g., `GUI r`).
- `SHIFT [key]`: Press Shift + Key.
- `ALT [key]`: Press Alt + Key.
- `CTRL` or `CONTROL [key]`: Press Ctrl + Key.
- `ENTER`: Press the Enter key.
- Standard key names (e.g., `F1`-`F12`, `TAB`, `ESC`, `SPACE`, `UP`, `DOWN`, `LEFT`, `RIGHT`, `DELETE`, `BACKSPACE`, `CAPSLOCK`, `PRINTSCREEN`, `SCROLLLOCK`, `PAUSE`, `INSERT`, `HOME`, `PAGEUP`, `PAGEDOWN`, `END`).

## Example Script

Here is a simple example that opens Notepad on Windows and types a message:

```ducky
REM Open Run dialog
GUI r
DELAY 500
REM Type notepad and press Enter
STRING notepad
DELAY 500
ENTER
DELAY 1000
REM Type a message
STRING Hello from ESP-Chain!
ENTER
```

## Configuration

The startup delay can be configured in the Settings module or by editing `config.json` on the SD card (`badusbStartupDelay`).
