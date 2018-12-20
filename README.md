# ExtractMotionPhotos
Extract Motion Photos created by Samsung phones to mp4 videos and plain jpg photos

## Usage of Program
This program accepts input files in three different ways:

1. Through a standard Windows file open dialog box. Run ExtractMotionPhotos.exe without any command line arguments and it will let you browse for one or more \*.jpg files. The dialog has multi-select enabled, so you can drag or use shift to select a range of files, ctrl to individually select files, or use ctrl+A to take all files in the directory. If you cancel the dialog without selecting any files, the program will display a help message in a dialog box.

2. Using drag-and-drop. Drag a bunch of motion photos onto the program's icon and it will convert them.

3. On the command line. Simply pass each filename as a separate argument, and it will process all of them. This doubles as a drag-and-drop method, because that's what Windows does when you drop files on an exe's icon.

Result files will be placed in the same directory as the source file they came from. The photo data will be in *${FILENAME}_photo.jpg*, and the video data will be in *${FILENAME}_video.mp4*. Neither is re-encoded, so no quality is lost.

If "-d" or "/d" is passed on the command line before any filenames, the original files will be deleted after successful processing. This works with both command line and GUI operation. To use it with command line drag-and-drop operation, create a shortcut to *ExtractMotionPhotos.exe* and include the -d in the command to run.

If "-r" or "/r" is passed on the command line before any filenames, the original files will be renamed with the "\_original" suffix and extracted files won't have any suffix added. This option can be combined with -d to delete the original and write the extracted photo to the same name. (Note, because of the implementation, -d -r will rename the original first with the \_original suffix, and then delete it. Normally this is insignificant, but if you have another file with the same name and \_original, that file will be lost.)

If "-q" or "/q" is passed on the command line before any filenames, all dialog boxes will be suppressed. This is intended for batch operation. Output will instead of printed to stdout. If you want to suppress this output as well, use "-qq" or "/qq".

The above options may be combined into -dq, -qdr, /rdqq, etc. like in POSIX programs.

The exit code will always be 0 on success, 1 on error, and 2 if the user was prompted for files but cancelled the dialog box.

*Tip:* If you want to use command line with the file dialog or drag-and-drop methods, create a shortcut to the program. Right click the shortcut and choose Properties. In the Shortcut tab, add a space and the command line options you want to the Target box. You can now double-click or drag files onto this shortcut.

## Multilingual version
ExtractMotionPhotos is now bilingual! It supports US English and Traditional Chinese. The Chinese translation is thanks to [AndyLain](http://andylain.blogspot.com). By default, you'll see the app in Chinese if your Windows is set to display its UI in any form of Chinese, and English otherwise. You can override this by putting **\_zh** at the end of the exe file's name to get Chinese, or **\_en** for English. For instance, to use the app in Chinese on an English OS, you might name it **ExtractMotionPhotos\_zh.exe**.

## Description of Motion Photo File Format
Samsung motion photos are simply a complete JPEG image followed by a 16-byte marker and then a complete MP4 video. The marker itself is `MotionPhoto_Data`.

This program simply searches for the first instance of the `MotionPhoto_Data` marker and splits the file around it.

*(Note: This could potentially result in invalid photo and video files if the JPEG half of the original file happens to contain this string. You could construct a valid motion photo that will be split incorrectly, or a plain JPEG that will incorrectly be detected as a motion photo, by putting "MotionPhoto_Data" in the JPEG comment field. It would be more correct to parse the JPEG data and only accept the marker if it appears directly after the end of the image data. However, the program is intended to parse photos taken with a phone, which will never put that string in the comment; and the chances of the string appearing randomly in the JPEG image stream are unbelievably tiny, so this naive implementation should be sufficient.)*

## Credit
This method of splitting motion photos was first published by XDA user [goofwear](http://forum.xda-developers.com/member.php?u=2489239) in [this thread](https://forum.xda-developers.com/android/software/samsung-motion-photo-extractor-t3339997). This project does the same, just in a single exe that doesn't require the user to do things to extract each photo.

## License
This code is published under the [MIT license](https://github.com/joemck/ExtractMotionPhotos/blob/master/LICENSE).
