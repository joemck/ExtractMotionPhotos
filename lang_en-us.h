//Language file for English -- just the strings themselves; position determines which ID goes with them

/* USAGE */
	L"You can use this program 3 different ways:\r\n"
	L"\r\n"
	L"GUI USE: Just run it. You'll get an file-open dialog where you can open .jpg files. Use ctrl or shift or drag to select more than one.\r\n"
	L"\r\n"
	L"DRAG & DROP USE: Drag one or more motion photos onto the icon for this exe.\r\n"
	L"\r\n"
	L"COMMAND LINE USE: Run this program with one or more motion photo file names as arguments. Remember to use \"quotes\" if there are spaces in the names.\r\n"
	L"\r\n"
	L"Any way you run it, the original files will not be modified. The extracted photo and video will be stored in *_photo.jpg and *_video.mp4 where * is the name of the original file, minus the .jpg extension.\r\n"
	L"\r\n"
	L"Coded by Chupi383. All credit for the method goes to goofwear at the XDA forums. I've just ported their .bat file to plain C and Win32."
,
/* APP_TITLE */
	L"Samsung Motion Photo Batch Extractor"
,
/* FILETYPES */
	L"Motion Photos (*.jpg)\0*.JPG\0All Files\0*.*\0\0"
,
/* OPEN_TITLEBAR */
	L"Open some Samsung Motion Photos"
,
/* SKIP_LONG_PATH */
	L"Skipping too-long path:\r\n%s%s%s"
,
/* ERROR_TITLEBAR */
	L"Error"
,
/* ERROR_TOO_MANY_FILES_NUM */
	L"You've selected too many files. I can only do up to %d at a time."
,
/* ERROR_TOO_MANY_FILES_UNK */
	L"You've selected too many files. Try selecting fewer files and process them in bunches."
,
/* PHOTO_APPEND */
	L"_photo.jpg"
,
/* PHOTO_NO_APPEND */
	L".jpg"
,
/* VIDEO_APPEND */
	L"_video.mp4"
,
/* VIDEO_NO_APPEND */
	L".mp4"
,
/* ORIG_APPEND */
	L"_original.jpg"
,
/* ERROR_OUT_OF_MEMORY */
	L"Can't allocate RAM to read %ld bytes from %s"
,
/* SKIP_READ_ERROR */
	L"Skipping due to read error:\r\n%s"
,
/* SKIP_WRITE_ERROR */
	L"Can't write to %s"
,
/* SUCCESS_MSG */
	L"Finished extracting Motion Photos\r\n    Photos extracted: %d\r\n    Videos extracted: %d"
,
/* SUCCESS_MSG_SKIP */
	L"\r\n%d files were skipped because they weren't Motion Photos"
,
/* SUCCESS_TITLEBAR */
	L"Success"
,
/* FAILURE_TITLEBAR */
	L"Failure"
