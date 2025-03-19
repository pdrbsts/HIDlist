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

// Retrieves and prints the manufacturer string using SetupDiGetDeviceRegistryProperty
void PrintDeviceManufacturer(HDEVINFO hDevInfoSet, SP_DEVINFO_DATA& deviceInfoData) {
    TCHAR manufacturer[256];
    DWORD propertyRegDataType;
    if (SetupDiGetDeviceRegistryProperty(
            hDevInfoSet,
            &deviceInfoData,
            SPDRP_MFG,
            &propertyRegDataType,
            (BYTE*)manufacturer,
            sizeof(manufacturer),
            NULL))
    {
        std::wcout << L"  Manufacturer: " << manufacturer << "\n";
    } else {
        std::cout << "  [Error] Could not get manufacturer name: " << GetLastError() << "\n";
    }
}

// Retrieves and prints the friendly name using SetupDiGetDeviceRegistryProperty
void PrintDeviceFriendlyName(HDEVINFO hDevInfoSet, SP_DEVINFO_DATA& deviceInfoData) {
    TCHAR friendlyName[256];
    DWORD propertyRegDataType;
    if (SetupDiGetDeviceRegistryProperty(
            hDevInfoSet,
            &deviceInfoData,
            SPDRP_DEVICEDESC, // Changed from SPDRP_MFG to SPDRP_FRIENDLYNAME
            &propertyRegDataType,
            (BYTE*)friendlyName,
            sizeof(friendlyName),
            NULL))
    {
        std::wcout << L"  Friendly Name: " << friendlyName << "\n"; // Updated output message
    } else {
        std::cout << "  [Error] Could not get friendly name: " << GetLastError() << "\n"; // Updated error message
    }
}

// Enumerates HID devices, checks for USB parent, and prints device details including Product and Serial Number
void ListHIDDevicesWithUSBParents() {
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
            std::cout << "  HID Device #" << i << ": " << deviceInstanceID << "\n";

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

int main() {
    ListHIDDevicesWithUSBParents();
    return 0;
}
