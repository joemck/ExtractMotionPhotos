/* Samsung Motion Photo Batch Extractor
 * by Chupi383
 * All credit for the method goes to goofwear at xda.
 * I've merely rewritten his .bat file as a C program and removed dependencies on external GUI programs.
 * The code here looks like more than it is. Most of it is dealing with Win32 and error handling.
 */

#include <stdio.h>
#include <windows.h>
#include <wchar.h>
#include <io.h>
#include <string.h>
#include <strings.h>
#include "lang.h"

//max filenames allowed to be given
#define MAX_FILES 25000

//error messages as dialog, printf, or nothing
int msgLvl = 2;
#define ALERT(msg, title, flags) do { \
	if(msgLvl == 2) MessageBoxW(NULL, (msg), (title), (flags)); \
	else if(msgLvl == 1) wprintf(L"%s\r\n", (msg)); \
} while(0)

//Show usage info
void usage(void) {
	ALERT(TR(USAGE), TR(APP_TITLE), MB_OK);
}

//Show an alert with a short message and the error message for error codes from GetLastError
void ShowWin32Error(LPCWSTR message, DWORD errcode) {
	LPWSTR errstr = NULL;
	WCHAR msgBuf[4096];
	FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,	//dwFlags
			NULL,	//lpSource
			errcode,	//dwMessageId
			0,	//dwLanguageId
			errstr,	//lpBuffer
			0,	//nSize
			NULL	//Arguments
			);
	swprintf(msgBuf, sizeof(msgBuf)/sizeof(WCHAR), L"%s\r\n%s", message, errstr);
	LocalFree(errstr); errstr = NULL;
	ALERT(msgBuf, TR(ERROR_TITLEBAR), MB_OK|MB_ICONERROR);
}

//Show a file-open dialog box and add the selected files to the names array
//returns 1 on success, 0 on failure
int promptForFiles(int *namesCnt, WCHAR names[][MAX_PATH]) {
	OPENFILENAMEW ofn;
	WCHAR filenames[512*1024];

	memset(&ofn, 0, sizeof(ofn));
	memset(&filenames, 0, sizeof(filenames));

	//Windows file-open dialogs are a bit ridiculous...
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = TR(FILETYPES);
	ofn.lpstrFile = filenames;	//dirname, NULL, then filenames in it separated by NULLs, with 2 NULLs at the end
	ofn.nMaxFile = sizeof(filenames);
	ofn.lpstrTitle = TR(OPEN_TITLEBAR);
	ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST;

	if(GetOpenFileNameW(&ofn)) {
		//Success! Parse the output
		int dir_len = wcslen(filenames);
		int i = dir_len + 1;	//skip past the base dir name

		if(filenames[i] == '\0') {
			//only one file selected
			if(dir_len + 1 > MAX_PATH) {
				WCHAR buf[4096];
				swprintf(buf, sizeof(buf)/sizeof(WCHAR), TR(SKIP_LONG_PATH), filenames, L"", L"");
				ALERT(buf, TR(ERROR_TITLEBAR), MB_OK|MB_ICONERROR);
				return 0;
			} else {
				wcsncpy(names[*namesCnt], filenames, MAX_PATH);
				(*namesCnt)++;
			}
		} else {
			//multiple files selected
			//there will be a file with a null name after the end
			while(filenames[i] != L'\0' && *namesCnt < MAX_FILES) {
				int file_len = wcslen(&filenames[i]);
				//sanity check; +2: 1 for added /, 1 for null
				if(dir_len + file_len + 2 > MAX_PATH) {
					WCHAR buf[4096];
					swprintf(buf, sizeof(buf)/sizeof(WCHAR), TR(SKIP_LONG_PATH), filenames, L"\\", &filenames[i]);
					ALERT(buf, TR(ERROR_TITLEBAR), MB_OK|MB_ICONERROR);
				} else {
					wcsncpy(names[*namesCnt], filenames, MAX_PATH);
					wcscat(names[*namesCnt], L"\\");
					wcscat(names[*namesCnt], &filenames[i]);
					(*namesCnt)++;
				}
				//move to next name
				i += file_len + 1;
			}

			if(filenames[i] != L'\0') {
				WCHAR buf[1024];
				swprintf(buf, sizeof(buf)/sizeof(WCHAR), TR(ERROR_TOO_MANY_FILES_NUM), MAX_FILES);
				ALERT(buf, TR(ERROR_TITLEBAR), MB_OK|MB_ICONERROR);
				return 0;
			}
		}
	} else if(CommDlgExtendedError() == FNERR_BUFFERTOOSMALL) {
		ALERT(TR(ERROR_TOO_MANY_FILES_UNK), TR(ERROR_TITLEBAR), MB_OK|MB_ICONERROR);
		return 0;
	}	//otherwise the user has probably cancelled the dialog box
	return 1;
}

//This initializes the data structures for findMagic(). It's only called for the first
//file processed in a batch since the magic bytes searched for are always the same.
void initSearch(const unsigned char *magic, int magiclen, char *d1, char *d2) {
	int i, j, lastPrefix = magiclen-1, suffixLen;

	memset(d1, magiclen, 256);
	for(i=0; i<magiclen-1; i++)
		d1[magic[i]] = magiclen-1-i;

	for(i=magiclen-1; i>=0; i--) {
		suffixLen = magiclen-i-1;
		for(j=0; j<suffixLen; j++)
			if(magic[j] != magic[i+1+j])
				break;
		if(j==suffixLen)
			lastPrefix = i+1;
		d2[i] = lastPrefix + (magiclen-1 - i);
	}

	for(i=0; i<magiclen-1; i++) {
		suffixLen = 0;
		while((magic[i-suffixLen] == magic[magiclen-1-suffixLen]) && (suffixLen<i))
			suffixLen++;

		if(magic[i-suffixLen] != magic[magiclen-1 - suffixLen])
			d2[magiclen-1 - suffixLen] = magiclen-1 - i + suffixLen;
	}
}

//Boyer-Moore string search -- this is considerably faster than a plain linear search
SSIZE_T findMagic(unsigned char *buf, SSIZE_T size) {
	const unsigned char magic[] = {
		0x4D, 0x6F, 0x74, 0x69, 0x6F, 0x6E, 0x50, 0x68,
		0x6F, 0x74, 0x6F, 0x5F, 0x44, 0x61, 0x74, 0x61 };
	static char d1[256];
	static char d2[sizeof(magic)];
	static BOOL inited = FALSE;
	SSIZE_T i, j;

	if(!inited) {
		initSearch(magic, sizeof(magic), d1, d2);
		inited = TRUE;
	}

	i = sizeof(magic)-1;
	while(i < size) {
		j = sizeof(magic)-1;
		while(j >= 0 && (buf[i] == magic[j])) {
			--i; --j;
		}
		if(j < 0)
			return i+1;
		i += max(d1[buf[i]], d2[j]);
	}
	return -1;
}

int wmain(int argc, WCHAR **argv) {
	int fileCnt, filenum, i;
	HANDLE infile, outfile;
	LARGE_INTEGER filesize;
	SSIZE_T bytes, bytes2, split;
	DWORD bytesDone;
	BOOL success;

	static WCHAR names[MAX_FILES][MAX_PATH];
	int namesCnt = 0;
	WCHAR namePhoto[MAX_PATH+32], nameVideo[MAX_PATH+32], nameOrig[MAX_PATH+32];

	unsigned char *fileData;

	int notMotionPhoto = 0, successPhoto = 0, successVideo = 0;
	int thisPhotoOK, thisVideoOK;

	WCHAR msgBuf[4096];

	int deleteOrig = 0, renameOrig = 0;

	FILETIME creationTime, lastWriteTime;	//we now preserve file timestamps
	BOOL haveFileTime;	//we store the result of GetFileTime here, so if there's a problem we can skip touching new files instead of touching with bad values

	DWORD err;

	detectLanguage(argc, argv);

	memset(names, 0, sizeof(names));

	fileCnt=0;
	for(i=1; i<argc; i++) {
		//check if this is a command line option
		//if an argument starts with - or / and contains only 'd' (max 1), 'q' (max 2) and 'r' (max 1) characters, it's options
		// -d: Delete the original file when done.
		// -q: Quiet. Puts messages on the terminal instead of dialogs. A second q shuts up terminal messages too.
		// -r: Rename original to *_original.jpg (or delete it if combined with -d), and do not append _photo and _video to extracted.
		if(argv[i][0]==L'-' || argv[i][0]==L'/') {
			int d=0, q=0, r=0, bad=0, j;
			for(j=1; argv[i][j] != L'\0' && !bad; j++) {
				int ch = argv[i][j];
				if(ch == L'd' && !d) d=1;
				else if(ch == L'r' && !r) r=1;
				else if(ch == L'q' && q<2) q++;
				else bad=1;
			}
			if(!bad && (d || r || q)) {
				//this was a valid command line arg, we should save its result
				if(d) deleteOrig = 1;
				if(r) renameOrig = 1;
				if(q) msgLvl -= q;
				if(msgLvl < 0) msgLvl = 0;
				continue;	//don't store this as a filename, it contained only up to 1 d and 2 q's
			}	//else, drop through and store this filename, it just happened to start with a -
		}

		wcsncpy(names[fileCnt], argv[i], MAX_PATH);
		if(names[fileCnt][MAX_PATH-1] == L'\0') {
			fileCnt++;
			if(fileCnt > MAX_FILES) {
				swprintf(msgBuf, sizeof(msgBuf)/sizeof(WCHAR), TR(ERROR_TOO_MANY_FILES_NUM), MAX_FILES);
				ALERT(msgBuf, TR(ERROR_TITLEBAR), MB_OK|MB_ICONERROR);
				return 1;
			}
		} else {
			swprintf(msgBuf, sizeof(msgBuf)/sizeof(WCHAR), TR(SKIP_LONG_PATH), argv[i], L"", L"");
			ALERT(msgBuf, TR(ERROR_TITLEBAR), MB_OK|MB_ICONERROR);
			names[fileCnt][MAX_PATH-1] = L'\0';	//reset detection so we don't skip all further names
		}
	}

	if(fileCnt == 0)
		if(!promptForFiles(&fileCnt, names))
			return 1;

	if(fileCnt == 0) {
		usage();
		return 2;
	}

	for(filenum = 0; filenum < fileCnt; filenum++) {
		//validate name -- only accept *.jpg
		i = wcslen(names[filenum]);
		if(names[filenum][i-4]!=L'.'
				|| ( names[filenum][i-3]!=L'j' && names[filenum][i-3]!=L'J' )
				|| ( names[filenum][i-2]!=L'p' && names[filenum][i-2]!=L'P' )
				|| ( names[filenum][i-1]!=L'g' && names[filenum][i-1]!=L'G' )) {
			notMotionPhoto++;
			continue;
		}

		//generate output names
		memset(namePhoto, 0, sizeof(namePhoto));
		wcsncpy(namePhoto, names[filenum], i-4);
		wcscat(namePhoto, TR(renameOrig ? PHOTO_NO_APPEND : PHOTO_APPEND));

		memset(nameVideo, 0, sizeof(nameVideo));
		wcsncpy(nameVideo, names[filenum], i-4);
		wcscat(nameVideo, TR(renameOrig ? VIDEO_NO_APPEND : VIDEO_APPEND));

		if(renameOrig) {	//this string is only used if we are renaming the original
			memset(nameOrig, 0, sizeof(nameOrig));
			wcsncpy(nameOrig, names[filenum], i-4);
			wcscat(nameOrig, TR(ORIG_APPEND));
		}

		//open and read the input
		//now using the win32 way for better control
		infile = CreateFileW(
				names[filenum],	//lpFileName
				GENERIC_READ,	//dwDesiredAccess
				FILE_SHARE_READ,	//dwShareMode
				NULL,	//lpSecurityAttributes
				OPEN_EXISTING,	//dwCreationDisposition
				FILE_FLAG_SEQUENTIAL_SCAN,	//dwFlagsAndAttributes
				NULL	//hTemplateFile
				);
		if(infile == INVALID_HANDLE_VALUE) {
			ShowWin32Error(names[filenum], GetLastError());
			notMotionPhoto++;
			continue;
		}

		//store its file times
		haveFileTime = GetFileTime(infile, &creationTime, NULL, &lastWriteTime);

		//get the size of the file
		if(!GetFileSizeEx(infile, &filesize)) {
			ShowWin32Error(names[filenum], GetLastError());
			CloseHandle(infile);
			notMotionPhoto++;
			continue;
		}

		bytes = filesize.QuadPart;

		//I sincerely hope nobody has managed to take a motion photo bigger than 4 GB. Either way, this program does not support large files so we skip anything that big.
		if(bytes > UINT_MAX) {
			CloseHandle(infile);
			notMotionPhoto++;
			continue;
		}

		//allocate space for the entire file in memory
		fileData = LocalAlloc(0, bytes);
		if(!fileData) {
			err = GetLastError();
			CloseHandle(infile);
			swprintf(msgBuf, sizeof(msgBuf)/sizeof(WCHAR), TR(ERROR_OUT_OF_MEMORY), bytes, names[filenum]);
			ShowWin32Error(msgBuf, err);
			return 1;
		}

		//read the file
		success = ReadFile(infile, fileData, (DWORD)bytes, &bytesDone, NULL);
		err = GetLastError();
		CloseHandle(infile);
		if(!success || bytesDone != bytes) {
			swprintf(msgBuf, sizeof(msgBuf)/sizeof(WCHAR), TR(SKIP_READ_ERROR), names[filenum]);
			ShowWin32Error(msgBuf, err);
			notMotionPhoto++;
			continue;
		}

		//find the split
		split = findMagic(fileData, bytes);

		//if we didn't find the magic in the file, it's not a motion photo
		if(split < 0) {
			LocalFree(fileData);
			notMotionPhoto++;
			continue;
		}

		//rename the original so we don't overwrite it
		if(renameOrig)
			MoveFileW(names[filenum], nameOrig);

		thisPhotoOK = thisVideoOK = 0;

		//write the photo portion (before split)
		outfile = CreateFileW(namePhoto, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if(outfile == INVALID_HANDLE_VALUE) {
			err = GetLastError();
			swprintf(msgBuf, sizeof(msgBuf)/sizeof(WCHAR), TR(SKIP_WRITE_ERROR), namePhoto);
			ShowWin32Error(msgBuf, err);
		} else {
			success = WriteFile(outfile, fileData, (DWORD)split, &bytesDone, NULL);
			err = GetLastError();
			if(haveFileTime)
				SetFileTime(outfile, &creationTime, NULL, &lastWriteTime);
			CloseHandle(outfile);
			if(success && bytesDone == (DWORD)split) {
				successPhoto++;
				thisPhotoOK = 1;
			} else {
				swprintf(msgBuf, sizeof(msgBuf)/sizeof(WCHAR), TR(SKIP_WRITE_ERROR), namePhoto);
				ShowWin32Error(msgBuf, err);
			}
		}

		//write the video portion (after the split)
		outfile = CreateFileW(nameVideo, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if(outfile == INVALID_HANDLE_VALUE) {
			err = GetLastError();
			swprintf(msgBuf, sizeof(msgBuf)/sizeof(WCHAR), TR(SKIP_WRITE_ERROR), nameVideo);
			ShowWin32Error(msgBuf, err);
		} else {
			success = WriteFile(outfile, fileData+split+16, (DWORD)(bytes-split-16), &bytesDone, NULL);
			err = GetLastError();
			if(haveFileTime)
				SetFileTime(outfile, &creationTime, NULL, &lastWriteTime);
			CloseHandle(outfile);
			if(success && bytesDone == (DWORD)(bytes-split-16)) {
				successVideo++;
				thisVideoOK = 1;
			} else {
				swprintf(msgBuf, sizeof(msgBuf)/sizeof(WCHAR), TR(SKIP_WRITE_ERROR), namePhoto);
				ShowWin32Error(msgBuf, err);
			}
		}

		LocalFree(fileData);

		if(deleteOrig) {
			if(thisPhotoOK && thisVideoOK) {	//everything good, delete the original file
				if(renameOrig)	//original was renamed, now delete it under the new name
					DeleteFileW(nameOrig);
				else	//original was not renamed, delete it under the old name
					DeleteFileW(names[filenum]);
			} else {	//everything NOT good, put things back how they were
				DeleteFileW(namePhoto);
				DeleteFileW(nameVideo);
				if(renameOrig)	//we renamed the original, undo this
					MoveFileW(nameOrig, names[filenum]);
			}
		}
	}

	bytes = swprintf(msgBuf, sizeof(msgBuf)/sizeof(WCHAR), TR(SUCCESS_MSG), successPhoto, successVideo);
	if(notMotionPhoto > 0)
		swprintf(msgBuf+bytes, sizeof(msgBuf)/sizeof(WCHAR)-bytes, TR(SUCCESS_MSG_SKIP), notMotionPhoto);
	if(successPhoto>0 || successVideo>0)
		ALERT(msgBuf, TR(SUCCESS_TITLEBAR), MB_OK|(notMotionPhoto?MB_ICONWARNING:MB_ICONINFORMATION));
	else
		ALERT(msgBuf, TR(FAILURE_TITLEBAR), MB_OK|MB_ICONERROR);

	return 0;
}
