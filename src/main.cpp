#include <cstdint>
#include <windows.h>
#include <minwindef.h>
#include <errhandlingapi.h>
#include <handleapi.h>
#include <iostream>
#include <fileapi.h>
#include <winioctl.h>
#include <winnt.h>
#include <vector>

#include "utils/binary.cpp"

struct FileEntry {
	DWORD RecordLength;
	DWORDLONG FileReferenceNumber;
	DWORDLONG ParentFileReferenceNumber;
	USN usn;
	uint64_t TimeStamp;
	DWORD FileAttributes;
	WORD FileNameLength;
	WORD FileNameOffset;
	std::wstring FileName;
};

struct FileTreeNode {
	FileEntry entry;
	int *parent;
};

class FileEntryDict {
	std::vector<FileEntry> fileEntries;

	public:
		FileEntryDict() {
			this->fileEntries = {};
		}

		FileEntryDict(std::vector<FileEntry> entries) {
			this->fileEntries = entries;
		}

		void print() {
			int size = this->fileEntries.size();
			for (int i = 0; i < size; i++) {
				std::wcout << "File Name: " << this->fileEntries[i].FileName << std::endl;
				std::cout << "File Reference Number: " << this->fileEntries[i].FileReferenceNumber << std::endl;
				std::cout << "Parent File Reference Number: " << this->fileEntries[i].ParentFileReferenceNumber << std::endl;
				std::cout << "USN: " << this->fileEntries[i].usn << std::endl;
				std::cout << "Time Stamp: " << this->fileEntries[i].TimeStamp << std::endl;
				std::cout << "File Attributes: " << this->fileEntries[i].FileAttributes << std::endl;
				std::cout << "--------------------------------------------------" << std::endl;
			}
		}

		void appendEntries(std::vector<FileEntry> entries) {
			for (unsigned long long int i = 0; i < entries.size(); i++) {
				this->fileEntries.push_back(entries[i]);
			}
		}

		void appendEntries(std::vector<FileEntry> entries, const std::vector<std::wstring> &filters) {
			for (unsigned long long int i = 0; i < entries.size(); i++) {
				for (unsigned long long int j = 0; j < filters.size(); j++) {
					if (entries[i].FileName.find(filters[j]) != std::wstring::npos) {
						this->fileEntries.push_back(entries[i]);
						break;
					}
				}
			}
		}

		unsigned long long int getSize() {
			return this->fileEntries.size();
		}
};

std::vector<FileEntry> getFiles(const BYTE *buffer, DWORD bytesReturned) {
	std::vector<FileEntry> entries;

	// All the data in the buffer is just binary, which needs to be parsed
	// std::cout << "Next ref: " << fromBytes<DWORDLONG>(buffer, 0) << std::endl;
	int currByte = 8;

	while (currByte < (int) bytesReturned) {
		int offset = currByte;
		int recordStart = currByte;
		DWORD RecordLength = fromBytes<DWORD>(buffer, offset);
		offset += sizeof(DWORD) + 2 * sizeof(WORD); // 2 * word because major and minor versions
		DWORDLONG FileReferenceNumber = fromBytes<DWORDLONG>(buffer, offset);
		offset += sizeof(DWORDLONG);
		DWORDLONG ParentFileReferenceNumber = fromBytes<DWORDLONG>(buffer, offset);
		offset += sizeof(DWORDLONG);
		USN usn = fromBytes<USN>(buffer, offset);
		offset += sizeof(USN);
		uint64_t TimeStamp = fromBytes<uint64_t>(buffer, offset);
		offset += sizeof(LARGE_INTEGER) + sizeof(DWORD) + sizeof(DWORD) + sizeof(DWORD); // skip reason, source info and security id
		DWORD FileAttributes = fromBytes<DWORD>(buffer, offset);
		offset += sizeof(DWORD);
		WORD FileNameLength = fromBytes<WORD>(buffer, offset);
		offset += sizeof(WORD);
		WORD FileNameOffset = fromBytes<WORD>(buffer, offset);
		offset += sizeof(WORD);

		const BYTE* nameData = buffer + currByte + FileNameOffset;
		const WCHAR* name = reinterpret_cast<const WCHAR*>(nameData);
		std::wstring FileName(name, FileNameLength / sizeof(WCHAR));

		currByte = recordStart + RecordLength;

		FileEntry entry = {
			RecordLength,
			FileReferenceNumber,
			ParentFileReferenceNumber,
			usn,
			TimeStamp,
			FileAttributes,
			FileNameLength,
			FileNameOffset,
			FileName
		};

		entries.push_back(std::move(entry));
	}
	return entries;
}

int main() {
	// creating a handle to read the partition
	HANDLE h = CreateFileW(
		L"\\\\.\\C:", // normalizes to "\\.\C:" string, which basically points to C drive
		GENERIC_READ, // desired access type in DWORD
		FILE_SHARE_READ | FILE_SHARE_WRITE, // desired share mode in DWORD
		nullptr, // security attributes
		OPEN_EXISTING, // action to perform on a file (or device)
		0, // extra flags and attributes
		nullptr // template handle
	);

	FileEntryDict dict = FileEntryDict();

	// MFT and its params
	MFT_ENUM_DATA mft{}; // creating an MFT variable
	mft.StartFileReferenceNumber = 0; // start from beginning
	mft.LowUsn = 0;
	mft.HighUsn = MAXLONGLONG; // give all you can find

	int lastError = 0;

	while (!lastError) {
		// buffer to store the MFT data
		BYTE buffer[1024 * 1024];
		DWORD bytesReturned; // number of bytes returned

		// bool success = DeviceIoControl(
		DeviceIoControl(
		    h, // device handle
		    FSCTL_ENUM_USN_DATA, // io control code, FSCTL_ENUM_USN_DATA -> enumerate NTFS file records
		    &mft, // place to store the data
		    sizeof(mft), // size of the data
		    buffer, // buffer data location
		    sizeof(buffer), // buffer size
		    &bytesReturned, // number of bytes returned
		    nullptr // overlap, not needed
		);

		mft.StartFileReferenceNumber = fromBytes<uint64_t>(buffer, 0);
		lastError = GetLastError();

		if (lastError) {
			if (lastError == ERROR_HANDLE_EOF) {
				std::cout << "EOF reached. Stopping iteration." << std::endl;
			}
			else {
				std::cout << "Last Error: " << lastError << std::endl; // prints error number
			}
			break;
		}

		// std::cout << "Bytes: " << bytesReturned << std::endl;
		// std::cout << "Next Start: " << mft.StartFileReferenceNumber << std::endl;

		std::vector<FileEntry> entries = getFiles(buffer, bytesReturned);

		// std::vector<std::wstring> filters = {L"."};
		// dict.appendEntries(entries, filters);
		dict.appendEntries(entries);
	}

	std::cout << "Total Size: " << dict.getSize() << std::endl;

	CloseHandle(h);

	return 0;
}
