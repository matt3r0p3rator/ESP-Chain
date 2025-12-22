# BadUSB Payloads

Place your DuckyScript payload files in this directory or its subdirectories.

## Structure
- `windows/`: Payloads for Windows targets.
- `linux/`: Payloads for Linux targets.
- `mac/`: Payloads for macOS targets.
- `chromebook/`: Payloads for ChromeOS targets.

## Format
Payloads should be plain text files (usually `.txt` or `.dks`) containing DuckyScript commands.

## Example
```ducky
GUI r
DELAY 500
STRING notepad
ENTER
DELAY 500
STRING Hello World!
```
