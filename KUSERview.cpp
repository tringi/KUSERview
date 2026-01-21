#include <Windows.h>
#include <CommCtrl.h>

#include <charconv>

#include <cstdint>
#include <cstdio>

#pragma warning (disable:28159)
#pragma warning (disable:6053)
#pragma warning (disable:4996)
#pragma warning (disable:4312)

extern "C" IMAGE_DOS_HEADER __ImageBase;
const wchar_t * szVersionInfo [7] = {};
wchar_t szTmpBuffer [32768];
wchar_t szTmpBuffer2 [32768];

struct WinVer {
    union {
        struct {
            UCHAR major : 4;
            UCHAR minor : 4;
        };
        UCHAR byte;
    };

    bool operator <= (WinVer other) const {
        return this->major < other.major
            || (this->major == other.major && this->minor <= other.minor);
    }
};

WinVer os;

enum Type : UCHAR {
    NoType,
    U8Type,
    U16Type,
    U32Type,
    U64Type,
};
enum Decode : UCHAR {
    NoDecode,
    BoolDecode,
    U8Decode,
    U16Decode,
    U32Decode,
    U64Decode,
    StringDecode,
    SystemTime,
    TickMultiplier,
    NtProductType,
    NativeProcArch,
    AltArch,
    ImageNumber,
    SuiteMask,
    MitigationPol,
    ProcFeatures,
    SharedDataFlags,
    TimeZoneId,
};

static const struct Element {
    USHORT offset;
    USHORT length;
    bool   live;
    Type   type   : 3;
    Decode decode : 5;
    WinVer minver;
    WinVer maxver;

    const char * name;
} structure [] = {
    { 0x0000,   4,  true, U32Type,      U32Decode, {  3,5 }, { 5,1 }, "TickCountLow" },
    { 0x0004,   4, false, U32Type, TickMultiplier, {  3,5 }, { 0,0 }, "TickCountMultiplier" },
    { 0x0008,  12,  true, U32Type,      U64Decode, {  3,5 }, { 5,2 }, "InterruptTime" },
    { 0x0008,  12,  true, U32Type,     SystemTime, {  6,0 }, { 0,0 }, "InterruptTime" },
    { 0x0014,  12,  true, U32Type,     SystemTime, {  3,5 }, { 0,0 }, "SystemTime" },
    { 0x0020,  12,  true, U32Type,     SystemTime, {  3,5 }, { 0,0 }, "TimeZoneBias" },
    { 0x002C,   4, false, U16Type,    ImageNumber, {  3,5 }, { 0,0 }, "ImageNumber" },
    { 0x0030, 260, false,  NoType,   StringDecode, {  3,5 }, { 0,0 }, "NtSystemRoot" },
    { 0x0238,   4, false, U32Type,       NoDecode, {  4,0 }, { 4,0 }, "DosDeviceMap" }, // each bit is DOS disk letter
    { 0x0238,   4, false, U32Type,      U32Decode, {  5,0 }, { 0,0 }, "MaxStackTraceDepth" },
    { 0x023C,   4, false, U32Type,      U32Decode, {  4,0 }, { 0,0 }, "CryptoExponent" },
    { 0x0240,   4,  true, U32Type,     TimeZoneId, {  4,0 }, { 0,0 }, "TimeZoneId" },
    { 0x0244,  20, false,  U8Type,       NoDecode, {  4,0 }, { 4,0 }, "DosDeviceDriveType" },
    { 0x0244,   4, false, U32Type,      U32Decode, {  5,2 }, { 0,0 }, "LargePageMinimum" },
    { 0x0248,   4,  true, U32Type,      U32Decode, {  6,2 }, { 0,0 }, "AitSamplingValue" }, // App Impact Telemetry
    { 0x024C,   4, false, U32Type,       NoDecode, {  6,2 }, { 0,0 }, "AppCompatFlag" },
    { 0x0250,   8,  true, U64Type,      U64Decode, {  6,2 }, { 0,0 }, "RNGSeedVersion" },
    { 0x0258,   4, false, U32Type,      U32Decode, {  6,2 }, { 0,0 }, "GlobalValidationRunLevel" },
    { 0x025C,   4,  true, U32Type,      U32Decode, {  6,2 }, { 0,0 }, "TimeZoneBiasStamp" },
    { 0x0260,   4, false, U32Type,      U32Decode, { 10,0 }, { 0,0 }, "NtBuildNumber" },
    { 0x0264,   4, false, U32Type,  NtProductType, {  4,0 }, { 0,0 }, "NtProductType" },
    { 0x0268,   1, false,  NoType,     BoolDecode, {  4,0 }, { 0,0 }, "ProductTypeIsValid" }, // 0 on install or when updating
    { 0x026A,   2, false, U16Type, NativeProcArch, {  6,2 }, { 0,0 }, "NativeProcessorArchitecture" },
    { 0x026C,   4, false, U32Type,      U32Decode, {  4,0 }, { 0,0 }, "NtMajorVersion" },
    { 0x0270,   4, false, U32Type,      U32Decode, {  4,0 }, { 0,0 }, "NtMinorVersion" },
    { 0x0274,  64, false,  NoType,   ProcFeatures, {  4,0 }, { 0,0 }, "ProcessorFeatures" },
    { 0x02B4,   4, false, U32Type,       NoDecode, {  4,0 }, { 0,0 }, "MmHighestUserAddressDeprecated" },
    { 0x02B8,   4, false, U32Type,       NoDecode, {  4,0 }, { 0,0 }, "MmSystemRangeStartDeprecated" },
    { 0x02BC,   4,  true, U32Type,      U32Decode, {  5,0 }, { 0,0 }, "TimeSlipDebugging" },
    { 0x02C0,   4, false, U32Type,        AltArch, {  5,0 }, { 0,0 }, "AlternativeArchitecture" },
    { 0x02C4,   4, false, U32Type,      U32Decode, { 10,0 }, { 0,0 }, "BootId" },
    { 0x02C8,   8, false, U64Type,     SystemTime, {  5,0 }, { 0,0 }, "SystemExpirationDate" },
    { 0x02D0,   4, false, U32Type,      SuiteMask, {  4,0 }, { 0,0 }, "SuiteMask" },
    { 0x02D4,   1,  true,  NoType,     BoolDecode, {  5,0 }, { 0,0 }, "KdDebuggerEnabled" }, // TODO: bit 0 enabled, bit 1 connected
    { 0x02D5,   1, false,  NoType,  MitigationPol, {  5,1 }, { 0,0 }, "MitigationPolicies" },
    { 0x02D6,   2, false, U16Type,      U16Decode, { 10,7 }, { 0,0 }, "CyclesPerYield" },
    { 0x02D8,   4,  true, U32Type,      U32Decode, {  5,1 }, { 0,0 }, "ActiveConsoleId" }, // TODO: can be -1 for Windows Sandbox
    { 0x02DC,   4,  true, U32Type,      U32Decode, {  5,1 }, { 0,0 }, "DismountCount" },
    { 0x02E0,   4,  true, U32Type,      U32Decode, {  5,1 }, { 0,0 }, "ComPlusPackage" },
    { 0x02E4,   4,  true, U32Type,      U32Decode, {  5,1 }, { 0,0 }, "LastSystemRITEventTickCount" }, // updates every second as long as some users provide input
    { 0x02E8,   4,  true, U32Type,      U32Decode, {  5,1 }, { 0,0 }, "NumberOfPhysicalPages" },
    { 0x02EC,   1, false,  NoType,       NoDecode, {  5,1 }, { 0,0 }, "SafeBootMode" },
    { 0x02ED,   1,  true,  NoType,       NoDecode, {  6,1 }, { 6,1 }, "TscQpcData" },
    { 0x02ED,   1, false,  NoType,       NoDecode, { 10,2 }, { 0,0 }, "VirtualizationFlags" }, // bit 0: vmx/svm enableable, bit 1: locked // Bits are 0 when: Hyper-V is running (regardless of partition); or VMX is off (in bios, or VM VBS-compatibility off). Bits form mask 3 or 1 when: Hyper-V is NOT running, and VMX is on (in bios, or VM VBS-compatibility ON).
    { 0x02F0,   4, false, U32Type,       NoDecode, {  5,1 }, { 5,2 }, "TraceLogging" },
    { 0x02F0,   4,  true, U32Type,SharedDataFlags, {  6,0 }, { 0,0 }, "SharedDataFlags" },
    { 0x02F8,   8, false, U64Type,       NoDecode, {  5,1 }, { 0,0 }, "TestRetInstruction" },
    { 0x0300,   4, false, U32Type,       NoDecode, {  5,1 }, { 6,1 }, "SystemCall" },
    { 0x0304,   4, false, U32Type,       NoDecode, {  5,1 }, { 6,1 }, "SystemCallReturn" },
    { 0x0300,   8, false, U64Type,      U64Decode, {  6,2 }, { 0,0 }, "QpcFrequency" },
    { 0x0308,   4, false, U32Type,       NoDecode, { 10,1 }, { 0,0 }, "SystemCall" }, // 0 - INT 2E, 1 - SYSCALL preferred, 
    { 0x030C,   4, false, U32Type,       NoDecode, { 10,9 }, { 0,0 }, "UserCetAvailableEnvironments" },
    { 0x0310,   8,  true, U64Type,      U64Decode, { 11,3 }, { 0,0 }, "FullNumberOfPhysicalPages" }, 
    { 0x0320,   8,  true, U64Type,      U64Decode, {  5,1 }, { 0,0 }, "TickCount" },
    { 0x0330,   4, false, U32Type,       NoDecode, {  5,1 }, { 0,0 }, "Cookie" },
    { 0x0334,  64, false, U32Type,      U32Decode, {  5,1 }, { 6,1 }, "Wow64SharedInformation" },
    { 0x0338,   8,  true, U64Type,      U64Decode, {  6,0 }, { 0,0 }, "ConsoleSessionForegroundProcessId" },
    { 0x0340,   8,  true, U64Type,      U64Decode, {  6,2 }, { 6,2 }, "TimeUpdateSequence" },
    { 0x0340,   8,  true, U64Type,      U64Decode, {  6,3 }, { 0,0 }, "TimeUpdateLock" },
    { 0x0348,   8,  true, U64Type,      U64Decode, {  6,2 }, { 0,0 }, "BaselineSystemTimeQpc" },
    { 0x0350,   8,  true, U64Type,      U64Decode, {  6,2 }, { 0,0 }, "BaselineInterruptTimeQpc" },
    { 0x0358,   8,  true, U64Type,     SystemTime, {  6,2 }, { 0,0 }, "QpcSystemTimeIncrement" },
    { 0x0360,   8, false, U64Type,     SystemTime, {  6,2 }, { 0,0 }, "QpcInterruptTimeIncrement" },
    { 0x0368,   4,  true, U32Type,      U32Decode, {  6,2 }, { 6,3 }, "QpcSystemTimeIncrement32" },
    { 0x0368,   1,  true,  U8Type,       U8Decode, { 10,0 }, { 0,0 }, "QpcSystemTimeIncrementShift" },
    { 0x0369,   1,  true,  U8Type,       U8Decode, { 10,0 }, { 0,0 }, "QpcInterruptTimeIncrementShift" },
    { 0x036A,   2,  true, U16Type,      U16Decode, { 10,0 }, { 0,0 }, "UnparkedProcessorCount" },
    { 0x036C,   4,  true, U32Type,      U32Decode, {  6,2 }, { 6,3 }, "QpcInterruptTimeIncrement32" },
    { 0x036C,  16, false, U32Type,       NoDecode, { 10,1 }, { 0,0 }, "EnclaveFeatureMask" }, // TODO: decode bits, bit1/bit2: SGX1/SGX2 leaf funcs enabled (and locked in that state), bit8: 1 if LoaderBlock->Extension.IumEnabled (can be 1 in root and non - root partitions; 0 if Hyper - V is off)
    { 0x0370,   1,  true,  U8Type,       U8Decode, {  6,2 }, { 6,3 }, "QpcSystemTimeIncrementShift" },
    { 0x0371,   1,  true,  U8Type,       U8Decode, {  6,2 }, { 6,3 }, "QpcInterruptTimeIncrementShift" },
    { 0x037C,   4,  true, U32Type,      U32Decode, { 10,4 }, { 0,0 }, "TelemetryCoverageRound" },
    { 0x0380,  16, false, U16Type,       NoDecode, {  6,0 }, { 6,0 }, "UserModeGlobalLogger" },
    { 0x0380,  32, false, U16Type,       NoDecode, {  6,1 }, { 0,0 }, "UserModeGlobalLogger" },
    { 0x0390,   8,  true, U32Type,       NoDecode, {  6,0 }, { 6,0 }, "HeapTracingPid" },
    { 0x0398,   8,  true, U32Type,       NoDecode, {  6,0 }, { 6,0 }, "CritSecTracingPid" },
    { 0x03A0,   4,  true, U32Type,       NoDecode, {  6,0 }, { 0,0 }, "ImageFileExecutionOptions" }, // bit 0: app verifier is globally enabled?
    { 0x03A4,   4,  true, U32Type,      U32Decode, {  6,1 }, { 0,0 }, "LangGenerationCount" },
    { 0x03A8,   4,  true, U32Type,      U32Decode, {  6,0 }, { 6,0 }, "ActiveProcessorAffinity" },
    { 0x03B0,   8,  true, U64Type,     SystemTime, {  6,0 }, { 0,0 }, "InterruptTimeBias" }, // ??
    { 0x03B8,   8,  true, U64Type,     SystemTime, {  6,1 }, { 6,2 }, "TscQpcBias" },
    { 0x03B8,   8,  true, U64Type,     SystemTime, {  6,3 }, { 0,0 }, "QpcBias" },
    { 0x03C0,   4,  true, U32Type,      U32Decode, {  6,1 }, { 0,0 }, "ActiveProcessorCount" },
    { 0x03C4,   2,  true, U16Type,      U16Decode, {  6,1 }, { 0,0 }, "ActiveGroupCount" },
    { 0x03C6,   2,  true,  U8Type,       NoDecode, {  6,2 }, { 6,2 }, "TscQpcData" },
    { 0x03C6,   2,  true,  U8Type,       NoDecode, {  6,3 }, { 0,0 }, "QpcData" },
    { 0x03C8,   4,  true, U32Type,      U32Decode, {  6,1 }, { 6,1 }, "AitSamplingValue" },
    { 0x03CC,   4, false, U32Type,       NoDecode, {  6,1 }, { 6,1 }, "AppCompatFlag" },
    { 0x03D0,   8, false, U64Type,       NoDecode, {  6,1 }, { 6,1 }, "SystemDllNativeRelocation" },
    { 0x03D8,   4, false, U32Type,       NoDecode, {  6,1 }, { 6,1 }, "SystemDllWowRelocation" },
    { 0x03C8,   8,  true, U64Type,     SystemTime, {  6,2 }, { 0,0 }, "TimeZoneBiasEffectiveStart" },
    { 0x03D0,   8,  true, U64Type,     SystemTime, {  6,2 }, { 0,0 }, "TimeZoneBiasEffectiveEnd" },
    { 0x03E0, 528,  true, U32Type,       NoDecode, {  6,1 }, { 6,1 }, "XState" },
    { 0x03D8, 840,  true, U32Type,       NoDecode, {  6,2 }, { 0,0 }, "XState" },
    { 0x0710,  12, false,  U8Type,       NoDecode, { 10,0 }, {10,15}, "FeatureConfigurationChangeStamp" },
    { 0x0720,  12, false,  U8Type,       NoDecode, { 11,0 }, { 0,0 }, "FeatureConfigurationChangeStamp" },
    { 0x0730,   8, false, U64Type,       NoDecode, { 11,0 }, { 0,0 }, "UserPointerAuthMask" },
    //{ 0x0738,   8, false, U64Type,       NoDecode, { 11,0 }, { 0,0 }, "XStateArm64" },
};

static inline HMODULE GetCurrentModuleHandle () {
    return reinterpret_cast <HMODULE> (&__ImageBase);
};

bool InitVersionInfoStrings (HINSTANCE hInstance) {
    if (szVersionInfo [0])
        return true;

    if (HRSRC hRsrc = FindResource (hInstance, MAKEINTRESOURCE (1), RT_VERSION)) {
        if (HGLOBAL hGlobal = LoadResource (hInstance, hRsrc)) {
            auto data = LockResource (hGlobal);
            auto size = SizeofResource (hInstance, hRsrc);

            if (data && (size >= 92)) {
                struct Header {
                    WORD wLength;
                    WORD wValueLength;
                    WORD wType;
                };

                // StringFileInfo
                //  - not searching, leap of faith that the layout is stable

                auto pstrings = static_cast <const unsigned char *> (data) + 76 + reinterpret_cast <const Header *> (data)->wValueLength;
                auto p = reinterpret_cast <const wchar_t *> (pstrings) + 12;
                auto e = p + reinterpret_cast <const Header *> (pstrings)->wLength / 2 - 12;
                auto i = 0u;

                const Header * header = nullptr;
                do {
                    header = reinterpret_cast <const Header *> (p);
                    auto length = header->wLength / 2;

                    if (header->wValueLength) {
                        szVersionInfo [i++] = p + length - header->wValueLength;
                    } else {
                        szVersionInfo [i++] = L"";
                    }

                    p += length;
                    if (length % 2) {
                        ++p;
                    }
                } while ((p < e) && (i < sizeof szVersionInfo / sizeof szVersionInfo [0]) && header->wLength);

                if (i == sizeof szVersionInfo / sizeof szVersionInfo [0])
                    return true;
            }
        }
    }
    return false;
}

void AppendTmp (const wchar_t * string) {
    if (szTmpBuffer [0]) {
        wcscat (szTmpBuffer, L", ");
    }
    wcscat (szTmpBuffer, string);
}

LRESULT ListViewCtrl_SetItemText (HWND hWnd, int idCtrl, LV_ITEM & item, int column) {
    item.iSubItem = column;
    item.cchTextMax = 32768;
    item.pszText = szTmpBuffer2;

    if (!SendDlgItemMessage (hWnd, 1, LVM_GETITEMTEXT, item.iItem, (LPARAM) &item) || (wcscmp (szTmpBuffer, szTmpBuffer2) != 0)) {
        item.pszText = szTmpBuffer;
        return SendDlgItemMessage (hWnd, 1, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
    } else
        return 0;
}

HFONT hFixedFont = NULL;

LRESULT CALLBACK WindowProcedure (_In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            if (auto cs = reinterpret_cast <CREATESTRUCT *> (lParam)) {

                auto style = WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL
                           | LVS_REPORT | LVS_SHAREIMAGELISTS | LVS_NOSORTHEADER;
                auto extra = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;

                if (auto h = CreateWindowEx (0, WC_LISTVIEW, L"", style,
                                             0, 0, 0, 0, hWnd, (HMENU) 1, cs->hInstance, NULL)) {
                    ListView_SetExtendedListViewStyle (h, extra);

                    LOGFONT lf;
                    if (GetObject ((HGDIOBJ) SendMessage (h, WM_GETFONT, 0, 0), sizeof lf, &lf)) {
                        if (os.major >= 6) {
                            wcscpy (lf.lfFaceName, L"Consolas");
                        } else {
                            wcscpy (lf.lfFaceName, L"Courier New");
                        }
                        hFixedFont = CreateFontIndirect (&lf);
                    }
                    if (!hFixedFont) {
                        hFixedFont = (HFONT) GetStockObject (SYSTEM_FIXED_FONT);
                    }

                    LVCOLUMN column {};
                    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                    column.pszText = szTmpBuffer;

                    while (LoadString (NULL, column.iSubItem + 1, szTmpBuffer, 32768)) {
                        column.cx = 128;
                        ListView_InsertColumn (h, column.iSubItem++, &column);
                    }

                    LVITEM item {};
                    item.mask = LVIF_TEXT;
                    item.iItem = 0x7FFF'FFFF;
                    item.pszText = szTmpBuffer;

                    for (const auto & element : structure) {
                        _snwprintf (szTmpBuffer, 32768, L"0x7FFE%04X", element.offset);
                        auto index = ListView_InsertItem (h, &item);
                        if (index != -1) {
                            
                            LVITEMA itemA;
                            itemA.iSubItem = 1;
                            itemA.pszText = const_cast <char *> (element.name);
                            SendMessage (h, LVM_SETITEMTEXTA, index, (LPARAM) &itemA);

                            if (element.maxver.byte == element.minver.byte) {
                                _snwprintf (szTmpBuffer, 32768, L"%u.%u only", // TODO: rsrc
                                            element.minver.major, element.minver.minor);
                            } else
                            if (element.maxver.byte) {
                                _snwprintf (szTmpBuffer, 32768, L"%u.%u - %u.%u",
                                            element.minver.major, element.minver.minor,
                                            element.maxver.major, element.maxver.minor);
                            } else {
                                _snwprintf (szTmpBuffer, 32768, L"%u.%u",
                                            element.minver.major, element.minver.minor);
                            }
                            
                            ListView_SetItemText (h, index, 2, szTmpBuffer);
                        }
                    }

                    WindowProcedure (hWnd, WM_TIMER, 0, 0);

                    ListView_SetColumnWidth (h, 0, LVSCW_AUTOSIZE);
                    ListView_SetColumnWidth (h, 1, LVSCW_AUTOSIZE);
                    ListView_SetColumnWidth (h, 2, LVSCW_AUTOSIZE);
                    ListView_SetColumnWidth (h, 3, LVSCW_AUTOSIZE);
                    if (ListView_SetColumnWidth (h, 4, LVSCW_AUTOSIZE)) {
                        ListView_SetColumnWidth (h, 4, 5 * ListView_GetColumnWidth (h, 4) / 4); // because of different font?
                    }

                    WindowProcedure (hWnd, WM_TIMER, 1, 0);

                    ListView_SetColumnWidth (h, 5, LVSCW_AUTOSIZE);

                    SetTimer (hWnd, 2, 40, NULL);
                    SetFocus (h);

                    RECT r;
                    if (GetWindowRect (hWnd, &r)) {
                        MoveWindow (hWnd, r.left, r.top, 3 * (r.right - r.left) / 4, r.bottom - r.top, TRUE);
                    }
                    
                } else
                    return -1;
            }
            break;

        case WM_ACTIVATE:
            if (wParam) {
                SetFocus (GetDlgItem (hWnd, 1));
            }
            break;

        case WM_WINDOWPOSCHANGED:
            if (auto position = reinterpret_cast <const WINDOWPOS *> (lParam)) {
                if (!(position->flags & SWP_NOSIZE) || (position->flags & (SWP_SHOWWINDOW | SWP_FRAMECHANGED))) {
                    RECT rClient;
                    if (GetClientRect (hWnd, &rClient)) {
                        MoveWindow (GetDlgItem (hWnd, 1), 0, 0, rClient.right, rClient.bottom, TRUE);
                    }
                }
            }
            break;

        case WM_TIMER:
            switch (wParam) {
                case 0: // first work, skip long data to autosize column width
                case 1: // second work, render remaining (unchanging) items
                case 2: // normal work, refresh only live items
                    LV_ITEM item;
                    item.iItem = 0;
                    item.pszText = szTmpBuffer;

                    SendDlgItemMessage (hWnd, 1, WM_SETREDRAW, FALSE, 0);

                    for (const auto & element : structure) {
                        bool display = false;

                        switch (wParam) {
                            case 0: display = (element.length <= 12u); break;
                            case 1: display = (element.length > 12u); break;
                            case 2: display = element.live; break;
                        }

                        if (display) {
                            szTmpBuffer [0] = L'\0';
                            for (auto i = 0u; i != element.length; ++i) {
                                _snwprintf (szTmpBuffer + 3 * i, 4, L"%02X ", reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [i]);
                            }

                            ListViewCtrl_SetItemText (hWnd, 1, item, 3);

                            szTmpBuffer [0] = L'\0';
                            switch (element.type) {
                                case U8Type:
                                    for (auto i = 0u; i != element.length / 1; ++i) {
                                        _snwprintf (szTmpBuffer + 5 * i, 6, L"0x%02X ", reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [i]);
                                    }
                                    break;
                                case U16Type:
                                    for (auto i = 0u; i != element.length / 2; ++i) {
                                        _snwprintf (szTmpBuffer + 7 * i, 8, L"0x%04X ", reinterpret_cast <const std::uint16_t *> (0x7FFE0000u + element.offset) [i]);
                                    }
                                    break;
                                case U32Type:
                                    for (auto i = 0u; i != element.length / 4; ++i) {
                                        _snwprintf (szTmpBuffer + 11 * i, 12, L"0x%08X ", reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset) [i]);
                                    }
                                    break;
                                case U64Type:
                                    for (auto i = 0u; i != element.length / 8; ++i) {
                                        _snwprintf (szTmpBuffer + 19 * i, 20, L"0x%016llX ", reinterpret_cast <const std::uint64_t *> (0x7FFE0000u + element.offset) [i]);
                                    }
                                    break;
                            }

                            ListViewCtrl_SetItemText (hWnd, 1, item, 4);

                            szTmpBuffer [0] = L'\0';
                            switch (element.decode) {
                                case BoolDecode: {
                                    bool result = false;
                                    for (auto i = 0u; i != element.length; ++i) {
                                        if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [i] != 0) {
                                            result = true;
                                            break;
                                        }
                                    }
                                    _snwprintf (szTmpBuffer, 32768, result ? L"True" : L"False");
                                } break;
                                
                                case U8Decode:
                                    _snwprintf (szTmpBuffer, 32768, L"%u", *reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset));
                                    break;
                                case U16Decode:
                                    _snwprintf (szTmpBuffer, 32768, L"%u", *reinterpret_cast <const std::uint16_t *> (0x7FFE0000u + element.offset));
                                    break;
                                case U32Decode:
                                    _snwprintf (szTmpBuffer, 32768, L"%u", *reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset));
                                    break;
                                case U64Decode:
                                    _snwprintf (szTmpBuffer, 32768, L"%llu", *reinterpret_cast <const std::uint64_t *> (0x7FFE0000u + element.offset));
                                    break;
                                case StringDecode:
                                    _snwprintf (szTmpBuffer, 32768, L"%s", reinterpret_cast <const wchar_t *> (0x7FFE0000u + element.offset));
                                    break;
                                case TickMultiplier: {
                                    auto x = (unsigned int) ((*reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset) * 1000uLL) >> 0x18);
                                    _snwprintf (szTmpBuffer, 32768, L"%u.%u", x / 1000, x % 1000);
                                } break;

                                case SystemTime:
                                    if (auto value = *reinterpret_cast <const std::int64_t *> (0x7FFE0000u + element.offset)) {
                                        SYSTEMTIME st;
                                        if (FileTimeToSystemTime (reinterpret_cast <const FILETIME *> (0x7FFE0000u + element.offset), &st) && (st.wYear > 1900)) {
                                            _snwprintf (szTmpBuffer, 32768, L"%04u-%02u-%02u %02u:%02u:%02u.%03u",
                                                        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
                                        } else
                                        if (value != 0x8000'0000'0000'0000) {
                                            bool negative = value < 0;
                                            if (negative) {
                                                value = -value;
                                            }
                                            value /= 10'000;
                                            auto ms = value % 1000;
                                            value /= 1000;
                                            auto s = value % 60;
                                            value /= 60;
                                            auto m = value % 60;
                                            value /= 60;
                                            auto h = value % 24;
                                            value /= 24;

                                            if (value) {
                                                _snwprintf (szTmpBuffer, 32768, L"%c %lldd %02llu:%02llu:%02llu.%03llu",
                                                            negative ? L'–' : L'+',
                                                            value, h, m, s, ms);
                                            } else {
                                                _snwprintf (szTmpBuffer, 32768, L"%c %02llu:%02llu:%02llu.%03llu",
                                                            negative ? L'–' : L'+',
                                                            h, m, s, ms);
                                            }
                                        }
                                    } else {
                                        _snwprintf (szTmpBuffer, 32768, L"0");
                                    }
                                    break;

                                case TimeZoneId:
                                    switch (auto value = *reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset)) {
                                        case 0: _snwprintf (szTmpBuffer, 32768, L"Not using DST"); break;
                                        case 1: _snwprintf (szTmpBuffer, 32768, L"Standard (winter)"); break;
                                        case 2: _snwprintf (szTmpBuffer, 32768, L"Daylight Savings Time (summer)"); break;

                                        default:
                                            _snwprintf (szTmpBuffer, 32768, L"Unknown");
                                    }
                                    break;

                                case NtProductType:
                                    switch (auto value = *reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset)) {
                                        case 1: _snwprintf (szTmpBuffer, 32768, L"Workstation"); break;
                                        case 2: _snwprintf (szTmpBuffer, 32768, L"Domain Controller"); break;
                                        case 3: _snwprintf (szTmpBuffer, 32768, L"Server"); break;

                                        default:
                                            _snwprintf (szTmpBuffer, 32768, L"Unknown");
                                    }
                                    break;

                                case NativeProcArch:
                                    switch (auto value = *reinterpret_cast <const std::uint16_t *> (0x7FFE0000u + element.offset)) {
                                        case PROCESSOR_ARCHITECTURE_INTEL: _snwprintf (szTmpBuffer, 32768, L"x86-32"); break;
                                        case PROCESSOR_ARCHITECTURE_ARM:   _snwprintf (szTmpBuffer, 32768, L"ARM32"); break;
                                        case PROCESSOR_ARCHITECTURE_IA64:  _snwprintf (szTmpBuffer, 32768, L"Itanium"); break;
                                        case PROCESSOR_ARCHITECTURE_MSIL:  _snwprintf (szTmpBuffer, 32768, L"MSIL"); break;
                                        case PROCESSOR_ARCHITECTURE_AMD64: _snwprintf (szTmpBuffer, 32768, L"AMD64/x86-64"); break;
                                        case PROCESSOR_ARCHITECTURE_ARM64:  _snwprintf (szTmpBuffer, 32768, L"ARM64"); break;

                                        default:
                                            _snwprintf (szTmpBuffer, 32768, L"Unknown");
                                    }
                                    break;

                                case AltArch:
                                    switch (auto value = *reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset)) {
                                        case 0: _snwprintf (szTmpBuffer, 32768, L""); break;
                                        case 1: _snwprintf (szTmpBuffer, 32768, L"NEC98x86"); break;

                                        default:
                                            _snwprintf (szTmpBuffer, 32768, L"Unknown");
                                    }
                                    break;

                                case ImageNumber: {
                                    auto p = reinterpret_cast <const std::uint16_t *> (0x7FFE0000u + element.offset);
                                    for (auto i = 0u; i != element.length / 2; ++i) {
                                        if (i) {
                                            if (p [i] != p [i - 1]) {
                                                wcscat (szTmpBuffer, L", ");
                                            } else
                                                continue;
                                        }

                                        switch (p [i]) {
                                            case IMAGE_FILE_MACHINE_I386: _snwprintf (szTmpBuffer2, 32768, L"i386/x86-32"); break;
                                            case IMAGE_FILE_MACHINE_ARMNT: _snwprintf (szTmpBuffer2, 32768, L"ARM32 Thumb-2"); break;
                                            case IMAGE_FILE_MACHINE_IA64: _snwprintf (szTmpBuffer2, 32768, L"IA64/Itanium"); break;
                                            case IMAGE_FILE_MACHINE_AMD64: _snwprintf (szTmpBuffer2, 32768, L"AMD64/x86-64"); break;
                                            case IMAGE_FILE_MACHINE_ARM64: _snwprintf (szTmpBuffer2, 32768, L"ARM64"); break;

                                            default:
                                                _snwprintf (szTmpBuffer2, 32768, L"Unknown");
                                        }
                                        wcscat (szTmpBuffer, szTmpBuffer2);
                                    }
                                } break;

                                case SuiteMask:
                                    if (auto value = *reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset)) {
                                        if (value & VER_SUITE_SMALLBUSINESS) AppendTmp (L"SBS");
                                        if (value & VER_SUITE_ENTERPRISE) AppendTmp (L"Enterprise");
                                        if (value & VER_SUITE_BACKOFFICE) AppendTmp (L"Back Office");
                                        if (value & VER_SUITE_COMMUNICATIONS) AppendTmp (L"Communications");
                                        if (value & VER_SUITE_TERMINAL) AppendTmp (L"Terminal Services");
                                        if (value & VER_SUITE_SMALLBUSINESS_RESTRICTED) AppendTmp (L"SBS Restricted");
                                        if (value & VER_SUITE_EMBEDDEDNT) AppendTmp (L"Embedded");
                                        if (value & VER_SUITE_DATACENTER) AppendTmp (L"DC");
                                        if (value & VER_SUITE_SINGLEUSERTS) AppendTmp (L"Single User");
                                        if (value & VER_SUITE_PERSONAL) AppendTmp (L"Home");
                                        if (value & VER_SUITE_BLADE) AppendTmp (L"Web Edition");
                                        if (value & VER_SUITE_EMBEDDED_RESTRICTED) AppendTmp (L"Embedded Restricted");
                                        if (value & VER_SUITE_SECURITY_APPLIANCE) AppendTmp (L"Security Appliance");
                                        if (value & VER_SUITE_STORAGE_SERVER) AppendTmp (L"Storage Server");
                                        if (value & VER_SUITE_COMPUTE_SERVER) AppendTmp (L"Compute Cluster Edition");
                                        if (value & VER_SUITE_WH_SERVER) AppendTmp (L"Home Server");
                                        if (value & 0x0001'0000) AppendTmp (L"Phone");
                                        if (value & VER_SUITE_MULTIUSERTS) AppendTmp (L"Multi User");
                                    }
                                    break;

                                case MitigationPol:
                                    if (auto value = *reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset)) {
                                        switch (value & 0x03) {
                                            case 0: AppendTmp (L"NX OFF"); break;
                                            case 1: AppendTmp (L"NX ON"); break;
                                            case 2: AppendTmp (L"NX Opt-In"); break;
                                            case 3: AppendTmp (L"NX Opt-Out"); break;
                                        }
                                        switch ((value >> 2) & 0x03) {
                                            case 0: AppendTmp (L"SEH Validation ON"); break;
                                            case 1: AppendTmp (L"SEH Validation OFF"); break;
                                            case 2: AppendTmp (L"SEH Validation Telemetry"); break;
                                            case 3: AppendTmp (L"SEH Validation Defer"); break;
                                        }
                                        switch ((value >> 4) & 0x03) {
                                            case 0: AppendTmp (L"All DLL paths allowed"); break;
                                            case 1: AppendTmp (L"WebDav DLL paths disallowed"); break;
                                            case 2: AppendTmp (L"All Remote Device DLL paths disallowed"); break;
                                            case 3: AppendTmp (L"All Current Working Directory DLL paths disallowed"); break;
                                        }
                                    }
                                    break;

                                case ProcFeatures:
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_FLOATING_POINT_PRECISION_ERRATA        ]) AppendTmp (L"FP errata");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_FLOATING_POINT_EMULATED                ]) AppendTmp (L"FP emulated");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_COMPARE_EXCHANGE_DOUBLE                ]) AppendTmp (L"CMPXCHG8B");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_MMX_INSTRUCTIONS_AVAILABLE             ]) AppendTmp (L"MMX");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_XMMI_INSTRUCTIONS_AVAILABLE            ]) AppendTmp (L"SSE");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_3DNOW_INSTRUCTIONS_AVAILABLE           ]) AppendTmp (L"3Dnow!");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_RDTSC_INSTRUCTION_AVAILABLE            ]) AppendTmp (L"RDTSC");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_PAE_ENABLED                            ]) AppendTmp (L"PAE");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_XMMI64_INSTRUCTIONS_AVAILABLE          ]) AppendTmp (L"SSE2");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_SSE_DAZ_MODE_AVAILABLE                 ]) AppendTmp (L"SSE DAZ");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_NX_ENABLED                             ]) AppendTmp (L"NX");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_SSE3_INSTRUCTIONS_AVAILABLE            ]) AppendTmp (L"SSE3");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_COMPARE_EXCHANGE128                    ]) AppendTmp (L"CMPXCHG16B");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_COMPARE64_EXCHANGE128                  ]) AppendTmp (L"CMP8XCHG16");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_XSAVE_ENABLED                          ]) AppendTmp (L"XSAVE");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_VFP_32_REGISTERS_AVAILABLE         ]) AppendTmp (L"VFP");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_NEON_INSTRUCTIONS_AVAILABLE        ]) AppendTmp (L"NEON");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_SECOND_LEVEL_ADDRESS_TRANSLATION       ]) AppendTmp (L"SLAT");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_VIRT_FIRMWARE_ENABLED                  ]) AppendTmp (L"Virtualization");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_RDWRFSGSBASE_AVAILABLE                 ]) AppendTmp (L"RDWRFSGSBASE");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_FASTFAIL_AVAILABLE                     ]) AppendTmp (L"Fast Fail");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_DIVIDE_INSTRUCTION_AVAILABLE       ]) AppendTmp (L"Divide");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_64BIT_LOADSTORE_ATOMIC             ]) AppendTmp (L"64b Load/Store");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_EXTERNAL_CACHE_AVAILABLE           ]) AppendTmp (L"External Cache");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_FMAC_INSTRUCTIONS_AVAILABLE        ]) AppendTmp (L"FMAC");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_RDRAND_INSTRUCTION_AVAILABLE           ]) AppendTmp (L"RDRAND");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_V8_INSTRUCTIONS_AVAILABLE          ]) AppendTmp (L"ARM64");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE   ]) AppendTmp (L"Crypto");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE    ]) AppendTmp (L"CRC32");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_RDTSCP_INSTRUCTION_AVAILABLE           ]) AppendTmp (L"RDTSCP");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_RDPID_INSTRUCTION_AVAILABLE            ]) AppendTmp (L"RDPID");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE  ]) AppendTmp (L"Atomics");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_MONITORX_INSTRUCTION_AVAILABLE         ]) AppendTmp (L"MONITORX");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_SSSE3_INSTRUCTIONS_AVAILABLE           ]) AppendTmp (L"SSSE3");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_SSE4_1_INSTRUCTIONS_AVAILABLE          ]) AppendTmp (L"SSE4.1");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_SSE4_2_INSTRUCTIONS_AVAILABLE          ]) AppendTmp (L"SSE4.2");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_AVX_INSTRUCTIONS_AVAILABLE             ]) AppendTmp (L"AVX");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_AVX2_INSTRUCTIONS_AVAILABLE            ]) AppendTmp (L"AVX2");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_AVX512F_INSTRUCTIONS_AVAILABLE         ]) AppendTmp (L"AVX512F");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ERMS_AVAILABLE                         ]) AppendTmp (L"ERMS");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_V82_DP_INSTRUCTIONS_AVAILABLE      ]) AppendTmp (L"DP");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_V83_JSCVT_INSTRUCTIONS_AVAILABLE   ]) AppendTmp (L"JSCVT");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_V83_LRCPC_INSTRUCTIONS_AVAILABLE   ]) AppendTmp (L"LRCPC");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_SVE_INSTRUCTIONS_AVAILABLE         ]) AppendTmp (L"SVE");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE        ]) AppendTmp (L"SVE2");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_SVE2_1_INSTRUCTIONS_AVAILABLE      ]) AppendTmp (L"SVE2.1");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_SVE_AES_INSTRUCTIONS_AVAILABLE     ]) AppendTmp (L"AES");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_SVE_PMULL128_INSTRUCTIONS_AVAILABLE]) AppendTmp (L"PMULL128");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_SVE_BITPERM_INSTRUCTIONS_AVAILABLE ]) AppendTmp (L"BITPERM");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_SVE_BF16_INSTRUCTIONS_AVAILABLE    ]) AppendTmp (L"BF16");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_SVE_EBF16_INSTRUCTIONS_AVAILABLE   ]) AppendTmp (L"EBF16");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_SVE_B16B16_INSTRUCTIONS_AVAILABLE  ]) AppendTmp (L"B16B16");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_SVE_SHA3_INSTRUCTIONS_AVAILABLE    ]) AppendTmp (L"SHA3");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_SVE_SM4_INSTRUCTIONS_AVAILABLE     ]) AppendTmp (L"SM4");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_SVE_I8MM_INSTRUCTIONS_AVAILABLE    ]) AppendTmp (L"I8MM");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_SVE_F32MM_INSTRUCTIONS_AVAILABLE   ]) AppendTmp (L"F32MM");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_SVE_F64MM_INSTRUCTIONS_AVAILABLE   ]) AppendTmp (L"F64MM");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_BMI2_INSTRUCTIONS_AVAILABLE            ]) AppendTmp (L"BMI2");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_MOVDIR64B_INSTRUCTION_AVAILABLE        ]) AppendTmp (L"MOVDIR64B");
                                    if (reinterpret_cast <const std::uint8_t *> (0x7FFE0000u + element.offset) [PF_ARM_LSE2_AVAILABLE                     ]) AppendTmp (L"LSE2");
                                    break;

                                case SharedDataFlags:
                                    if (*reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset) & 0x0000'0001) AppendTmp (L"Error Port");
                                    if (*reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset) & 0x0000'0002) AppendTmp (L"Elevation");
                                    if (*reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset) & 0x0000'0004) AppendTmp (L"Virtualization");
                                    if (*reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset) & 0x0000'0008) AppendTmp (L"Installer Detect");
                                    if (*reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset) & 0x0000'0010) AppendTmp (L"LKG");
                                    if (*reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset) & 0x0000'0020) AppendTmp (L"Dynamic Processor");
                                    if (*reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset) & 0x0000'0040) AppendTmp (L"Console Broker");
                                    if (*reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset) & 0x0000'0080) AppendTmp (L"Secure Boot");
                                    if (*reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset) & 0x0000'0100) AppendTmp (L"Multi Session SKU");
                                    if (*reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset) & 0x0000'0200) AppendTmp (L"Multi Users In Session SKU");
                                    if (*reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset) & 0x0000'0400) AppendTmp (L"State Separation");
                                    if (*reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset) & 0x0000'0800) AppendTmp (L"Split Token");
                                    if (*reinterpret_cast <const std::uint32_t *> (0x7FFE0000u + element.offset) & 0x0000'1000) AppendTmp (L"Shadow Admin");
                                    break;
                            }

                            ListViewCtrl_SetItemText (hWnd, 1, item, 5);
                        }
                        ++item.iItem;
                    }
                    SendDlgItemMessage (hWnd, 1, WM_SETREDRAW, TRUE, 0);
                    break;
            }
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR) lParam)->code) {
                case NM_CUSTOMDRAW:
                    LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW) lParam;

                    auto index = lplvcd->nmcd.dwItemSpec;
                    switch (lplvcd->nmcd.dwDrawStage) {

                        case CDDS_PREPAINT:
                            return CDRF_NOTIFYITEMDRAW;

                        case CDDS_ITEMPREPAINT:
                            if (structure [index].minver <= os && (os <= structure [index].maxver || structure [index].maxver.byte == 0)) {
                                lplvcd->clrText = GetSysColor (COLOR_WINDOWTEXT);
                            } else {
                                lplvcd->clrText = 0xAAAAAA;
                            }
                            return CDRF_NOTIFYSUBITEMDRAW;

                        case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
                            static HGDIOBJ hPrevFont = NULL;
                            if (lplvcd->iSubItem == 3) {
                                hPrevFont = SelectObject (lplvcd->nmcd.hdc, hFixedFont);
                                return CDRF_NEWFONT;
                            }
                            if (lplvcd->iSubItem == 5) {
                                SelectObject (lplvcd->nmcd.hdc, hPrevFont);
                                return CDRF_NEWFONT;
                            }
                    }
                    break;
            }
            break;

        case WM_CLOSE:
            PostQuitMessage ((int) wParam);
            break;
        case WM_ENDSESSION:
            PostQuitMessage (ERROR_SHUTDOWN_IN_PROGRESS);
            break;
    }
    return DefWindowProc (hWnd, message, wParam, lParam);
}

#if NDEBUG
void EntryPoint () {
#else
int APIENTRY wWinMain (_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int) {
#endif

    auto version = GetVersion ();
    os.major = LOBYTE (LOWORD (version));
    os.minor = HIBYTE (LOWORD (version));

    if (os.major == 10) {
        auto build = *reinterpret_cast <const std::uint32_t *> (0x7FFE0260u);
        if (build >= 22000) {
            os.major = 11;
            if (build >= 22621) os.minor = 1;
            if (build >= 22631) os.minor = 2;
            if (build >= 26100) os.minor = 3;
            if (build >= 26200) os.minor = 4;
        } else {
            if (build >= 10586) os.minor = 1;
            if (build >= 14393) os.minor = 2;
            if (build >= 15063) os.minor = 3;
            if (build >= 16299) os.minor = 4;
            if (build >= 17134) os.minor = 5;
            if (build >= 17763) os.minor = 6;
            if (build >= 18362) os.minor = 7;
            if (build >= 18363) os.minor = 8;
            if (build >= 19041) os.minor = 9;
            if (build >= 19042) os.minor = 10;
            if (build >= 19043) os.minor = 11;
            if (build >= 19044) os.minor = 12;
            if (build >= 19045) os.minor = 13;
            if (build >= 20348) os.minor = 14;
            if (build >= 20349) os.minor = 15;
        }
    }

    const INITCOMMONCONTROLSEX classes = {
        sizeof (INITCOMMONCONTROLSEX),
        ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES
    };

    InitCommonControls ();
    if (InitCommonControlsEx (&classes)) {
        if (InitVersionInfoStrings (GetCurrentModuleHandle ())) {

            const WNDCLASSEX wndclass = {
                sizeof (WNDCLASSEX), CS_HREDRAW | CS_VREDRAW,
                WindowProcedure, 0, 0, GetCurrentModuleHandle (),
                NULL, NULL, NULL, NULL, szVersionInfo [0], NULL
            };

            if (auto atom = RegisterClassEx (&wndclass)) {
                _snwprintf (szTmpBuffer, 32768, L"%s %s - %s // NT %u.%u",
                            szVersionInfo [5], szVersionInfo [6], szVersionInfo [2], os.major, os.minor);

                if (auto hWnd = CreateWindow ((LPCTSTR) (std::intptr_t) atom, szTmpBuffer,
                                              WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                              CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                              HWND_DESKTOP, NULL, GetCurrentModuleHandle (), NULL)) {
                    SetLastError (0);

                    MSG message {};
                    while (GetMessage (&message, hWnd, 0, 0) > 0) {
                        if (!IsDialogMessage (hWnd, &message)) {
                            TranslateMessage (&message);
                            DispatchMessage (&message);
                        }
                    }

#if NDEBUG
                    ExitProcess (message.wParam);
#else
                    return (int) message.wParam;
#endif
                }
            }
        }
    }

#if NDEBUG
    ExitProcess (GetLastError ());
#else
    return (int) GetLastError ();
#endif
}
