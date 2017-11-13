/* Samsung Motion Photo Batch Extractor
 * by Chupi383
 * All credit for the method goes to goofwear at xda.
 * I've merely rewritten his .bat file as a C program and removed dependencies on external GUI programs.
 * The code here looks like more than it is. Most of it is dealing with Win32 and error handling.
 */

#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <strings.h>

//max filenames allowed to be given
#define MAX_FILES 25000

//error messages as dialog, printf, or nothing
int msgLvl = 2;
#define ALERT(msg, title, flags) do { \
	if(msgLvl == 2) MessageBox(NULL, (msg), (title), (flags)); \
	else if(msgLvl == 1) printf("%s\r\n", (msg)); \
} while(0)

//Show usage info
void usage(void) {
	static const char *msg = 
"You can use this program 3 different ways:\r\n"
"\r\n"
"GUI USE: Just run it. You'll get an file-open dialog where you can open .jpg files. Use ctrl or shift or drag to select more than one.\r\n"
"\r\n"
"DRAG & DROP USE: Drag one or more motion photos onto the icon for this exe.\r\n"
"\r\n"
"COMMAND LINE USE: Run this program with one or more motion photo file names as arguments. Remember to use \"quotes\" if there are spaces in the names.\r\n"
"\r\n"
"Any way you run it, the original files will not be modified. The extracted photo and video will be stored in *_photo.jpg and *_video.mp4 where * is the name of the original file, minus the .jpg extension.\r\n"
"\r\n"
"Coded by Chupi383. All credit for the method goes to goofwear at the XDA forums. I've just ported their .bat file to plain C and Win32.";
	ALERT(msg, "Samsung Motion Photo Batch Extractor", MB_OK);
}

//Show a file-open dialog box and add the selected files to the names array
//returns 1 on success, 0 on failure
int promptForFiles(int *namesCnt, char names[][MAX_PATH]) {
	OPENFILENAME ofn;
	char filenames[512*1024];

	memset(&ofn, 0, sizeof(ofn));
	memset(&filenames, 0, sizeof(filenames));

	//Windows file-open dialogs are a bit ridiculous...
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = "Motion Photos (*.jpg)\0*.JPG\0\0";
	ofn.lpstrFile = filenames;	//dirname, NULL, then filenames in it separated by NULLs, with 2 NULLs at the end
	ofn.nMaxFile = sizeof(filenames);
	ofn.lpstrTitle = "Open some Samsung Motion Photos";
	ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST;

	if(GetOpenFileName(&ofn)) {
		//Success! Parse the output
		int dir_len = strlen(filenames);
		int i = dir_len + 1;	//skip past the base dir name

		if(filenames[i] == '\0') {
			//only one file selected
			if(dir_len + 1 > MAX_PATH) {
				char buf[4096];
				snprintf(buf, sizeof(buf), "Skipping too-long path:\r\n%s", filenames);
				ALERT(buf, "Error", MB_OK|MB_ICONERROR);
				return 0;
			} else {
				strcpy(names[*namesCnt], filenames);
				(*namesCnt)++;
			}
		} else {
			//multiple files selected
			//there will be a file with a null name after the end
			while(filenames[i] != '\0' && *namesCnt < MAX_FILES) {
				int file_len = strlen(&filenames[i]);
				//sanity check; +2: 1 for added /, 1 for null
				if(dir_len + file_len + 2 > MAX_PATH) {
					char buf[4096];
					snprintf(buf, sizeof(buf), "Skipping too-long path:\r\n%s\\%s", filenames, &filenames[i]);
					ALERT(buf, "Error", MB_OK|MB_ICONERROR);
				} else {
					strcpy(names[*namesCnt], filenames);
					strcat(names[*namesCnt], "\\");
					strcat(names[*namesCnt], &filenames[i]);
					(*namesCnt)++;
				}
				//move to next name
				i += file_len + 1;
			}

			if(filenames[i] != '\0') {
				char buf[1024];
				snprintf(buf, sizeof(buf), "You've selected too many files. I can only do up to %d at a time.", MAX_FILES);
				ALERT(buf, "Error", MB_OK|MB_ICONERROR);
				return 0;
			}
		}
	} else if(CommDlgExtendedError() == FNERR_BUFFERTOOSMALL) {
		static const char *msg = "You've selected too many files. Try selecting fewer files and process them in bunches.";
		ALERT(msg, "Error", MB_OK|MB_ICONERROR);
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

int main(int argc, char **argv) {
	int fileCnt, filenum, i;
	FILE *infile, *outfile;
	ssize_t bytes, bytes2, split;

	static char names[MAX_FILES][MAX_PATH];
	int namesCnt = 0;
	char namePhoto[MAX_PATH+6], nameVideo[MAX_PATH+6];

	unsigned char *fileData;

	int notMotionPhoto = 0, successPhoto = 0, successVideo = 0;
	int thisPhotoOK, thisVideoOK;

	char msgBuf[4096];

	int deleteOrig = 0, success;

	memset(names, 0, sizeof(names));

	fileCnt=0;
	for(i=1; i<argc; i++) {
		//check if this is a command line option
		if(argv[i][0]=='-' || argv[i][0]=='/') {
			int d=0, q=0, bad=0, j;
			for(j=1; argv[i][j] != '\0' && !bad; j++) {
				int ch = argv[i][j];
				if(ch == 'd' && !d) d=1;
				else if(ch == 'q' && q<2) q++;
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

		strncpy(names[fileCnt], argv[i], MAX_PATH);
		if(names[fileCnt][MAX_PATH-1] == '\0') {
			fileCnt++;
			if(fileCnt > MAX_FILES) {
				snprintf(msgBuf, sizeof(msgBuf), "You've selected too many files. I can only do up to %d at a time.", MAX_FILES);
				ALERT(msgBuf, "Error", MB_OK|MB_ICONERROR);
				return 1;
			}
		} else {
			snprintf(msgBuf, sizeof(msgBuf), "Skipping too-long path:\r\n%s", argv[i]);
			ALERT(msgBuf, "Error", MB_OK|MB_ICONERROR);
			names[fileCnt][MAX_PATH-1] = '\0';	//reset detection so we don't skip all further names
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
		i = strlen(names[filenum]);
		if(names[filenum][i-4]!='.'
				|| names[filenum][i-3]!='j'
				|| names[filenum][i-2]!='p'
				|| names[filenum][i-1]!='g') {
			notMotionPhoto++;
			continue;
		}

		//generate output names
		memset(namePhoto, 0, sizeof(namePhoto));
		memset(nameVideo, 0, sizeof(nameVideo));
		strncpy(namePhoto, names[filenum], i-4);
		strcat(namePhoto, "_photo.jpg");
		strncpy(nameVideo, names[filenum], i-4);
		strcat(nameVideo, "_video.mp4");

		//open and read the input
		infile = fopen(names[filenum], "rb");
		fseek(infile, 0, SEEK_END);
		bytes = ftell(infile);
		if(bytes <= 0) {
			notMotionPhoto++;
			continue;
		}
		fseek(infile, 0, SEEK_SET);
		fileData = malloc(bytes);
		if(!fileData) {
			snprintf(msgBuf, sizeof(msgBuf), "Can't allocate RAM to read %ld bytes from %s", bytes, names[filenum]);
			ALERT(msgBuf, "Error", MB_OK|MB_ICONERROR);
			return 1;
		}
		bytes2 = fread(fileData, 1, bytes, infile);
		fclose(infile);
		if(bytes2 != bytes) {
			snprintf(msgBuf, sizeof(msgBuf), "Skipping due to read error:\r\n%s", names[filenum]);
			ALERT(msgBuf, "Error", MB_OK|MB_ICONERROR);
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
		outfile = fopen(namePhoto, "wb");
		if(outfile) {
			bytes2 = fwrite(fileData, 1, split, outfile);
			fclose(outfile);
			if(bytes2 == split) {
				successPhoto++;
				thisPhotoOK = 1;
			} else {
				snprintf(msgBuf, sizeof(msgBuf), "Can't write to %s", namePhoto);
				ALERT(msgBuf, "Error", MB_OK|MB_ICONERROR);
			}
		}

		//write the video portion (after the split)
		outfile = fopen(nameVideo, "wb");
		if(outfile) {
			bytes2 = fwrite(fileData+split+16, 1, bytes-split-16, outfile);
			fclose(outfile);
			if(bytes2 == bytes-split-16) {
				successVideo++;
				thisVideoOK = 1;
			} else {
				snprintf(msgBuf, sizeof(msgBuf), "Can't write to %s", namePhoto);
				ALERT(msgBuf, "Error", MB_OK|MB_ICONERROR);
			}
		}

		if(deleteOrig && thisPhotoOK && thisVideoOK)
			unlink(names[filenum]);

		free(fileData);
	}

	bytes = snprintf(msgBuf, sizeof(msgBuf), "Finished extracting Motion Photos\r\n    Photos extracted: %d\r\n    Videos extracted: %d", successPhoto, successVideo);
	if(notMotionPhoto > 0)
		snprintf(msgBuf+bytes, sizeof(msgBuf)-bytes, "\r\n%d files were skipped because they weren't Motion Photos", notMotionPhoto);
	if(successPhoto>0 || successVideo>0)
		ALERT(msgBuf, "Success", MB_OK|(notMotionPhoto?MB_ICONWARNING:MB_ICONINFORMATION));
	else
		ALERT(msgBuf, "Failure", MB_OK|MB_ICONERROR);

	return 0;
}
