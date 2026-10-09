|Offset	|Field	|Size| Datatype|
|--|--|--|--|
|0x00	|RecordLength	|4 bytes|DWORD|
|0x04	|MajorVersion	|2 bytes|WORD|
|0x06	|MinorVersion	|2 bytes|WORD|
|0x08	|FileReferenceNumber	|8 bytes|DWORDLONG|
|0x10	|ParentFileReferenceNumber	|8 bytes|DWORDLONG|
|0x18	|Usn	|8 bytes|USN|
|0x20	|TimeStamp	|8 bytes|LARGE_INTEGER|
|0x28	|Reason	|4 bytes|DWORD|
|0x2C	|SourceInfo	|4 bytes|DWORD|
|0x30	|SecurityId	|4 bytes|DWORD|
|0x34	|FileAttributes	|4 bytes|DWORD|
|0x38	|FileNameLength	|2 bytes|WORD|
|0x3A	|FileNameOffset	|2 bytes|WORD|
|0x3C	|FileName begins in the usual V2 layout	|Variable|WCHAR[]|
