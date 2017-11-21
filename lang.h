/* Language base - this defines language structures and then includes a special
 * file for each language.
 */

#ifndef __LANG_H_
#define __LANG_H_

enum lang_elems {
	USAGE,
	APP_TITLE,
	FILETYPES,
	OPEN_TITLEBAR,
	SKIP_LONG_PATH,
	ERROR_TITLEBAR,
	ERROR_TOO_MANY_FILES_NUM,
	ERROR_TOO_MANY_FILES_UNK,
	PHOTO_APPEND,
	VIDEO_APPEND,
	ERROR_OUT_OF_MEMORY,
	SKIP_READ_ERROR,
	SKIP_WRITE_ERROR,
	SUCCESS_MSG,
	SUCCESS_MSG_SKIP,
	SUCCESS_TITLEBAR,
	FAILURE_TITLEBAR,
	N_ELEMS
};

extern int lang_index;
extern const WCHAR *lang_tbl[2][N_ELEMS];

void detectLanguage(int argc, WCHAR **argv);
const WCHAR* TR(int id);

#endif /* __LANG_H_ */
