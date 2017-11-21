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
ssize_t findMagic(unsigned char *buf, ssize_t size) {
	const unsigned char magic[] = {
		0x4D, 0x6F, 0x74, 0x69, 0x6F, 0x6E, 0x50, 0x68,
		0x6F, 0x74, 0x6F, 0x5F, 0x44, 0x61, 0x74, 0x61 };
	static char d1[256];
	static char d2[sizeof(magic)];
	static BOOL inited = FALSE;
	ssize_t i, j;

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
	FILE *infile, *outfile;
	ssize_t bytes, bytes2, split;

	static WCHAR names[MAX_FILES][MAX_PATH];
	int namesCnt = 0;
	WCHAR namePhoto[MAX_PATH+6], nameVideo[MAX_PATH+6];

	unsigned char *fileData;

	int notMotionPhoto = 0, successPhoto = 0, successVideo = 0;
	int thisPhotoOK, thisVideoOK;

	WCHAR msgBuf[4096];

	int deleteOrig = 0, success;

	detectLanguage(argc, argv);

	memset(names, 0, sizeof(names));

	fileCnt=0;
	for(i=1; i<argc; i++) {
		//check if this is a command line option
		if(argv[i][0]==L'-' || argv[i][0]==L'/') {
			int d=0, q=0, bad=0, j;
			for(j=1; argv[i][j] != L'\0' && !bad; j++) {
				int ch = argv[i][j];
				if(ch == L'd' && !d) d=1;
				else if(ch == L'q' && q<2) q++;
				else bad=1;
			}
			if(!bad && (d || q)) {
				//this was a valid command line arg, we should save its result
				if(d) deleteOrig = 1;
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
				|| names[filenum][i-3]!=L'j'
				|| names[filenum][i-2]!=L'p'
				|| names[filenum][i-1]!=L'g') {
			notMotionPhoto++;
			continue;
		}

		//generate output names
		memset(namePhoto, 0, sizeof(namePhoto));
		memset(nameVideo, 0, sizeof(nameVideo));
		wcsncpy(namePhoto, names[filenum], i-4);
		wcscat(namePhoto, TR(PHOTO_APPEND));
		wcsncpy(nameVideo, names[filenum], i-4);
		wcscat(nameVideo, TR(VIDEO_APPEND));

		//open and read the input
		infile = _wfopen(names[filenum], L"rb");
		fseek(infile, 0, SEEK_END);
		bytes = ftell(infile);
		if(bytes <= 0) {
			notMotionPhoto++;
			continue;
		}
		fseek(infile, 0, SEEK_SET);
		fileData = malloc(bytes);
		if(!fileData) {
			swprintf(msgBuf, sizeof(msgBuf)/sizeof(WCHAR), TR(ERROR_OUT_OF_MEMORY), bytes, names[filenum]);
			ALERT(msgBuf, TR(ERROR_TITLEBAR), MB_OK|MB_ICONERROR);
			return 1;
		}
		bytes2 = fread(fileData, 1, bytes, infile);
		fclose(infile);
		if(bytes2 != bytes) {
			swprintf(msgBuf, sizeof(msgBuf)/sizeof(WCHAR), TR(SKIP_READ_ERROR), names[filenum]);
			ALERT(msgBuf, TR(ERROR_TITLEBAR), MB_OK|MB_ICONERROR);
			notMotionPhoto++;
			continue;
		}

		//find the split
		split = findMagic(fileData, bytes);

		//if we didn't find the magic in the file, it's not a motion photo
		if(split < 0) {
			notMotionPhoto++;
			continue;
		}

		thisPhotoOK = thisVideoOK = 0;

		//write the photo portion (before split)
		outfile = _wfopen(namePhoto, L"wb");
		if(outfile) {
			bytes2 = fwrite(fileData, 1, split, outfile);
			fclose(outfile);
			if(bytes2 == split) {
				successPhoto++;
				thisPhotoOK = 1;
			} else {
				swprintf(msgBuf, sizeof(msgBuf)/sizeof(WCHAR), TR(SKIP_WRITE_ERROR), namePhoto);
				ALERT(msgBuf, TR(ERROR_TITLEBAR), MB_OK|MB_ICONERROR);
			}
		}

		//write the video portion (after the split)
		outfile = _wfopen(nameVideo, L"wb");
		if(outfile) {
			bytes2 = fwrite(fileData+split+16, 1, bytes-split-16, outfile);
			fclose(outfile);
			if(bytes2 == bytes-split-16) {
				successVideo++;
				thisVideoOK = 1;
			} else {
				swprintf(msgBuf, sizeof(msgBuf)/sizeof(WCHAR), TR(SKIP_WRITE_ERROR), namePhoto);
				ALERT(msgBuf, TR(ERROR_TITLEBAR), MB_OK|MB_ICONERROR);
			}
		}

		if(deleteOrig && thisPhotoOK && thisVideoOK)
			_wunlink(names[filenum]);

		free(fileData);
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
