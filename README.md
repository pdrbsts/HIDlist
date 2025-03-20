# HIDlist

## Overview

HIDlist is a program designed to list all Human Interface Devices (HID) connected via USB on your computer. This includes devices such as keyboards, mice, touchscreens, and more. The program provides detailed information about each HID device, including its parent device, device ID, manufacturer, and friendly name.

## Installation

To use HIDlist, you will need to clone this repository and build the project. Here are the steps:

1. **Clone the Repository**

   ```
   git clone https://github.com/pdrbsts/HIDlist.git
   ```

2. **Navigate to the Project Directory**

   ```bash
   cd HIDlist
   ```

3. **Build the Program**

   - **Windows (using Visual Studio)**

     Run the build.bat file.



## Usage

Once the program is built, you can run it to list all connected HID devices. Here’s how:

```
HIDlist.exe
```

or simply run the executable if on Windows.

## Sample Output

Below is an example of what the output might look like when running HIDlist:

```
Parent Device: USB\VID_1234&PID_1001\68408CC1C000
HID Device #1: HID\VID_1234&PID_1001\6&5C05E81&0&0000
Manufacturer: Hitachi
Friendly Name: HID compatible device
--------------------------------------------------
Parent Device: USB\VID_0456&PID_1234\5&129312F6&0&1
HID Device #2: HID\VID_0456&PID_1123\6&32DFA592&0&0000
Manufacturer: Microsoft
Friendly Name: HID compatible mouse
--------------------------------------------------
```

