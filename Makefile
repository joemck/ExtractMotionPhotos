#Makefile for ExtractMotionPhotos - multilanguage version

#The program itself is Windows only since it makes use of Win32 API calls for
#things like common dialogs. Since it only does simple things with Win32 API,
#it should work perfectly on Linux or Mac using any version of Wine. Dragging
#files onto the exe won't, but command line use and running it to get a file
#open dialog should work fine. To compile the exe on Linux, use a mingw32
#cross-compiler package.

#these settings work on mingw32-gcc and should work for cygwin
#they will need to be adjusted if you have mingw32 installed on Linux to cross-compile Windows programs
#Other makefile adjustments will likely be needed to compile with Visual Studio, Watcom, etc.
CC = gcc
WINDRES = windres
LD = gcc
RM = rm -f

CFLAGS += -Os
LDFLAGS += -lcomdlg32 -mwindows

TARGET = ExtractMotionPhotos.exe
OBJ = main.o res.o
HEADERS = 
RES_O = res.o
RES_RC = res.rc
RES_DEPENDS = icon_multires.ico

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(LD) -o $@ $+ $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@

$(RES_O): $(RES_RC) $(RES_DEPENDS)
	$(WINDRES) $< $@

clean:
	$(RM) $(TARGET) $(OBJ)
