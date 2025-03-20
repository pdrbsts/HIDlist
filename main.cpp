#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <hidsdi.h>      // HID definitions
#include <initguid.h>    // Required for GUID_DEVINTERFACE_HID
#include <iostream>
#include <vector>
#include <string>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

DEFINE_GUID(GUID_DEVINTERFACE_HID, 
    0x4D1E55B2, 0xF16F, 0x11CF, 
    0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30);

// Returns true if the device (by its instance ID) has a USB parent
bool GetUSBParentDevice(const std::string& deviceInstanceID) {
    DEVINST devInst, devInstParent;
    CONFIGRET status;

    status = CM_Locate_DevNodeA(&devInst, (DEVINSTID_A)deviceInstanceID.c_str(), CM_LOCATE_DEVNODE_NORMAL);
    if (status != CR_SUCCESS) {
        std::cout << "  [Error] Could not locate device node.\n";
        return false;
    }
    status = CM_Get_Parent(&devInstParent, devInst, 0);
    if (status != CR_SUCCESS) {
        std::cout << "  [Error] Could not retrieve parent device.\n";
        return false;
    }
    char parentDeviceID[MAX_DEVICE_ID_LEN];
    status = CM_Get_Device_IDA(devInstParent, parentDeviceID, MAX_DEVICE_ID_LEN, 0);
    if (status == CR_SUCCESS) {
        std::string parentIDStr = parentDeviceID;
        if (parentIDStr.rfind("USB", 0) == 0) { // Parent ID begins with "USB"
            std::cout << "  Parent Device: " << parentDeviceID << "\n";
            return true;
        }
    } else {
        std::cout << "  [Error] Could not get parent device ID.\n";
    }
    return false;
}

// Converts a wide string property to UTF-8 for console output
std::string WideToUTF8(const wchar_t* wstr) {
    if (!wstr) return "";
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    std::vector<char> utf8String(size_needed);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8String.data(), size_needed, NULL, NULL);
    
    return std::string(utf8String.data());
}

// Retrieves and prints the manufacturer string using SetupDiGetDeviceRegistryPropertyW
void PrintDeviceManufacturer(HDEVINFO hDevInfoSet, SP_DEVINFO_DATA& deviceInfoData) {
    WCHAR buffer[256] = {0};
    DWORD bufferSize = sizeof(buffer);
    
    if (SetupDiGetDeviceRegistryPropertyW(
            hDevInfoSet,
            &deviceInfoData,
            SPDRP_MFG,
            NULL,
            (PBYTE)buffer,
            bufferSize,
            NULL))
    {
        std::string utf8Str = WideToUTF8(buffer);
        std::cout << "  Manufacturer : " << utf8Str << "\n";
    } else {
        std::cout << "  [Error] Could not get manufacturer name: " << GetLastError() << "\n";
    }
}

// Retrieves and prints the device description using SetupDiGetDeviceRegistryPropertyW
void PrintDeviceFriendlyName(HDEVINFO hDevInfoSet, SP_DEVINFO_DATA& deviceInfoData) {
    WCHAR buffer[256] = {0};
    DWORD bufferSize = sizeof(buffer);
    
    if (SetupDiGetDeviceRegistryPropertyW(
            hDevInfoSet,
            &deviceInfoData,
            SPDRP_DEVICEDESC, // Device description property
            NULL,
            (PBYTE)buffer,
            bufferSize,
            NULL))
    {
        std::string utf8Str = WideToUTF8(buffer);
        std::cout << "  Device Name  : " << utf8Str << "\n";
    } else {
        std::cout << "  [Error] Could not get device description: " << GetLastError() << "\n";
    }
}

// Enumerates HID devices, checks for USB parent, and prints device details
void ListHIDDevicesWithUSBParents() {
    // Set console output to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_HID, NULL, NULL,
                                            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: SetupDiGetClassDevs failed.\n";
        return;
    }

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    DWORD i = 0;
    while (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_HID, i, &deviceInterfaceData)) {
        // Get required buffer size
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            std::cout << "  [Error] Failed to get device interface detail size.\n";
            break;
        }

        std::vector<BYTE> detailDataBuffer(requiredSize);
        PSP_DEVICE_INTERFACE_DETAIL_DATA_A pDeviceInterfaceDetailData = 
            reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_A>(detailDataBuffer.data());
        pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

        SP_DEVINFO_DATA deviceInfoData;
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        if (!SetupDiGetDeviceInterfaceDetailA(hDevInfo, &deviceInterfaceData, 
                                             pDeviceInterfaceDetailData, requiredSize, 
                                             NULL, &deviceInfoData)) {
            std::cout << "  [Error] Failed to get device interface detail.\n";
            i++;
            continue;
        }

        // Retrieve the device instance ID for parent checking
        char deviceInstanceID[MAX_DEVICE_ID_LEN] = {0};
        if (CM_Get_Device_IDA(deviceInfoData.DevInst, deviceInstanceID, MAX_DEVICE_ID_LEN, 0) != CR_SUCCESS) {
            std::cout << "  [Error] Could not retrieve device instance ID.\n";
            i++;
            continue;
        }
        
        std::string instanceIDStr(deviceInstanceID);

        // Check if the parent is a USB device
        if (GetUSBParentDevice(instanceIDStr)) {
            std::cout << "  HID Device   : " << deviceInstanceID << "\n";

            // Print manufacturer info
            PrintDeviceManufacturer(hDevInfo, deviceInfoData);
            
            // Print friendly name
            PrintDeviceFriendlyName(hDevInfo, deviceInfoData);
            std::cout << "--------------------------------------------------\n";
        }
        i++;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
}

// Function to toggle enable/disable state of a HID device by its instance ID
bool DisableHIDDevice(const std::string& deviceInstanceID) {
    DEVINST devInst;
    CONFIGRET status;
    ULONG dnStatus, problemNumber;

    status = CM_Locate_DevNodeA(&devInst, (DEVINSTID_A)deviceInstanceID.c_str(), CM_LOCATE_DEVNODE_NORMAL);
    if (status != CR_SUCCESS) {
        std::cout << "  [Error] Could not locate device node.\n";
        return false;
    }

    status = CM_Get_DevNode_Status(&dnStatus, &problemNumber, devInst, 0);
    if (status != CR_SUCCESS) {
        std::cout << "  [Error] Could not get device node status.\n";
        return false;
    }

    if (dnStatus & DN_DISABLEABLE) { // Check if the device is disableable
        if (dnStatus & DN_STARTED) { // Device is currently enabled, so disable it
            status = CM_Disable_DevNode(devInst, 0);
            if (status == CR_SUCCESS) {
                std::cout << "  Device disabled successfully: " << deviceInstanceID << "\n";
                return true;
            } else {
                std::cout << "  [Error] Could not disable device (Error code: " << status << ").\n";
                return false;
            }
        } else { // Device is currently disabled, so enable it
            status = CM_Enable_DevNode(devInst, 0);
            if (status == CR_SUCCESS) {
                std::cout << "  Device enabled successfully: " << deviceInstanceID << "\n";
                return true;
            } else {
                std::cout << "  [Error] Could not enable device (Error code: " << status << ").\n";
                return false;
            }
        }
    } else {
        std::cout << "  [Error] Device cannot be disabled/enabled programmatically.\n";
        return false;
    }
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string deviceInstanceID = argv[1];
        DisableHIDDevice(deviceInstanceID);
    } else {
        ListHIDDevicesWithUSBParents();
    }
    return 0;
}
