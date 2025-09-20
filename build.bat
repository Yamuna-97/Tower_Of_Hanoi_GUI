@echo off
set PATH=D:\mingw64\bin;%PATH%
D:\mingw64\bin\gcc.exe hanoi.c -o hanoi.exe -mms-bitfields ^
-I D:/mingw64/include/gtk-3.0 ^
-I D:/mingw64/include/cairo ^
-I D:/mingw64/include/pango-1.0 ^
-I D:/mingw64/include/harfbuzz ^
-I D:/mingw64/include ^
-I D:/mingw64/include/glib-2.0 ^
-I D:/mingw64/lib/glib-2.0/include ^
-I D:/mingw64/include/pixman-1 ^
-I D:/mingw64/include/freetype2 ^
-I D:/mingw64/include/libpng16 ^
-I D:/mingw64/include/gdk-pixbuf-2.0 ^
-I D:/mingw64/include/atk-1.0 ^
-L D:/mingw64/lib ^
-lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 ^
-lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 ^
-lgobject-2.0 -lglib-2.0
.\hanoi.exe
pause
