# KUSER_SHARED_DATA Viewer

Plain and simple KUSER_SHARED_DATA structure viewer, that also displays content from SYSTEM_HYPERVISOR_USER_SHARED_DATA and SILO_USER_SHARED_DATA if available.

## Download latest EXE

* [KUSERview.exe for x86-64](https://github.com/tringi/KUSERview/raw/main/Bin/Release/x64/KUSERview.exe)
* [KUSERview.exe for x86-32](https://github.com/tringi/KUSERview/raw/main/Bin/Release/Win32/KUSERview.exe)
* [KUSERview.exe for AArch64](https://github.com/tringi/KUSERview/raw/main/Bin/Release/ARM64/KUSERview.exe)


## References

* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/ns-ntddk-kuser_shared_data
* https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/ntexapi_x/kuser_shared_data/index.htm
* https://ntdoc.m417z.com/kuser_shared_data
* https://ntdoc.m417z.com/silo_user_shared_data
* https://ntdoc.m417z.com/system_hypervisor_user_shared_data

## TODO

* Menu bar? File/Save As/Exit, Edit/Copy/Select All, Help/About/Legend
* Save as text file, not just Ctrl+C
* Ouput to console
* GUI text on colors and my Windows *versions*
* Dark mode?
* Properly do scaling?
