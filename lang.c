/* Language base - this contains the language detector function */

#include <windows.h>
#include <wchar.h>
#include <string.h>
#include "lang.h"

enum {
	LANG_EN_US = 0,
	LANG_ZH_TW = 1,
};

int lang_index;
const WCHAR *lang_tbl[][N_ELEMS] =
{
	//ID 0: English
	{
#include "lang_en-us.h"
	},
	//ID 1: Chinese (Traditional)
	{
#include "lang_zh-tw.h"
	},
};

void detectLanguage(int argc, WCHAR **argv) {
	LANGID lang;
	int offs;

	//append "_en" to exe name to force English
	offs = wcslen(argv[0]) - 7;
	if(0 == _wcsicmp(argv[0]+offs, L"_en.exe"))
		lang_index = LANG_EN_US;

	//append "_zh" to exe name to force Chinese
	else if(0 == _wcsicmp(argv[0]+offs, L"_zh.exe"))
		lang_index = LANG_ZH_TW;

	//otherwise, follow the system preferred language settings
	else {
		lang = GetUserDefaultUILanguage();
		if((lang & 0x0ff) == 0x04)
			//xx04 => some form of Chinese; we only have 1 Chinese translation, so use that for all Chinese locales
			lang_index = LANG_ZH_TW;
		else
			lang_index = LANG_EN_US;
	}
}

const WCHAR* TR(int id) {
	return lang_tbl[lang_index][id];
}
