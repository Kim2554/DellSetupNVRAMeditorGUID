#ifndef INTEL_ADVANCED_UI_H
#define INTEL_ADVANCED_UI_H

typedef unsigned char  UINT8;
typedef unsigned short UINT16;
typedef unsigned int   UINT32;
typedef unsigned long long UINT64;
typedef UINT64 UINTN;
typedef UINT16 CHAR16;
typedef UINT64 EFI_STATUS;
typedef void* EFI_HANDLE;

#define EFI_SUCCESS 0ULL
#define EFI_NOT_FOUND 0x800000000000000EULL
#define EFI_BUFFER_TOO_SMALL 0x8000000000000005ULL
#define EFI_NOT_READY 0x8000000000000006ULL
#define EFI_ERROR(s) (((UINT64)(s)) >> 63)

#define EFI_VARIABLE_NON_VOLATILE 0x0000000000000001ULL
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x0000000000000002ULL
#define EFI_VARIABLE_RUNTIME_ACCESS 0x0000000000000004ULL

typedef struct {
    UINT32 Data1; UINT16 Data2; UINT16 Data3; UINT8 Data4[8];
} EFI_GUID;

typedef struct {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

typedef struct {
    UINT16 ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

typedef EFI_STATUS (*EFI_TEXT_OUT_STRING)(void*, const CHAR16*);
typedef EFI_STATUS (*EFI_TEXT_OUT_RESET)(void*, UINT8);
typedef EFI_STATUS (*EFI_TEXT_OUT_TEST)(void*, const CHAR16*);
typedef EFI_STATUS (*EFI_TEXT_OUT_QUERY)(void*, UINTN, UINTN*, UINTN*);
typedef EFI_STATUS (*EFI_TEXT_OUT_MODE)(void*, UINTN);
typedef EFI_STATUS (*EFI_TEXT_OUT_ATTR)(void*, UINTN);
typedef EFI_STATUS (*EFI_TEXT_OUT_CLEAR)(void*);
typedef EFI_STATUS (*EFI_TEXT_OUT_POS)(void*, UINTN, UINTN);
typedef EFI_STATUS (*EFI_TEXT_OUT_CURSOR)(void*, UINT8);

typedef struct {
    EFI_TEXT_OUT_RESET Reset;
    EFI_TEXT_OUT_STRING OutputString;
    EFI_TEXT_OUT_TEST TestString;
    EFI_TEXT_OUT_QUERY QueryMode;
    EFI_TEXT_OUT_MODE SetMode;
    EFI_TEXT_OUT_ATTR SetAttribute;
    EFI_TEXT_OUT_CLEAR ClearScreen;
    EFI_TEXT_OUT_POS SetCursorPosition;
    EFI_TEXT_OUT_CURSOR EnableCursor;
    void *Mode;
} SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef EFI_STATUS (*EFI_TEXT_IN_RESET)(void*, UINT8);
typedef EFI_STATUS (*EFI_TEXT_IN_READ)(void*, EFI_INPUT_KEY*);
typedef struct {
    EFI_TEXT_IN_RESET Reset;
    EFI_TEXT_IN_READ ReadKeyStroke;
    void *WaitForKey;
} SIMPLE_TEXT_INPUT_PROTOCOL;

typedef EFI_STATUS (*EFI_GET_VARIABLE)(
    CHAR16*, EFI_GUID*, UINT32*, UINTN*, void*);
typedef EFI_STATUS (*EFI_GET_NEXT_VARIABLE)(
    UINTN*, CHAR16*, EFI_GUID*);
typedef EFI_STATUS (*EFI_SET_VARIABLE)(
    CHAR16*, EFI_GUID*, UINT32, UINTN, void*);

typedef struct {
    EFI_TABLE_HEADER Hdr;
    void *GetTime; void *SetTime; void *GetWakeupTime; void *SetWakeupTime;
    void *SetVirtualAddressMap; void *ConvertPointer;
    EFI_GET_VARIABLE GetVariable;
    EFI_GET_NEXT_VARIABLE GetNextVariableName;
    EFI_SET_VARIABLE SetVariable;
    void *GetNextHighMonotonicCount; void *ResetSystem;
    void *UpdateCapsule; void *QueryCapsuleCapabilities; void *QueryVariableInfo;
} EFI_RUNTIME_SERVICES;

typedef struct {
    EFI_TABLE_HEADER Hdr;
    CHAR16 *FirmwareVendor;
    UINT32 FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    EFI_RUNTIME_SERVICES *RuntimeServices;
    void *BootServices;
    UINTN NumberOfTableEntries;
    void *ConfigurationTable;
} EFI_SYSTEM_TABLE;

/* Scan codes */
#define SCAN_UP       0x01
#define SCAN_DOWN     0x02
#define SCAN_RIGHT    0x03
#define SCAN_LEFT     0x04
#define SCAN_HOME     0x05
#define SCAN_END      0x06
#define SCAN_DELETE   0x08
#define SCAN_PAGEUP   0x09
#define SCAN_PAGEDOWN 0x0A
#define SCAN_F1       0x0B
#define SCAN_F2       0x0C
#define SCAN_F9       0x13
#define SCAN_F10      0x14
#define SCAN_ESC      0x17 /* many Dell firmwares map Esc as Unicode 0x1B; handle both */

typedef struct {
    UINT32 FormId;
    const char *Name;
    UINT32 FirstQuestion;
    UINT32 QuestionCount;
} IA_FORM;

typedef struct {
    UINT32 Value;
    const char *Label;
} IA_OPTION;

typedef struct {
    UINT32 Type;
    UINT32 StoreId;
    UINT32 Offset;
    UINT32 QuestionId;
    UINT32 Size;
    UINT64 Min;
    UINT64 Max;
    UINT32 OptionBase;
    UINT32 OptionCount;
    UINT64 DefaultValue;
    UINT32 HasDefault;
    const char *Prompt;
} IA_QUESTION;

typedef struct {
    UINT32 Id;
    UINT32 Size;
    EFI_GUID Guid;
    const char *Name;
} IA_STORE;

extern const IA_STORE ia_stores[];
extern const unsigned ia_store_count;
extern const IA_FORM ia_forms[];
extern const unsigned ia_form_count;
extern const IA_OPTION ia_options[];
extern const unsigned ia_option_count;
extern const IA_QUESTION ia_questions[];
extern const unsigned ia_question_count;

#endif
