
all:
	g++ -dynamiclib -std=c++11 -O2 -I. -lavcodec -lavformat -lavutil -lswresample -lswscale src/*.cpp -o shadron-ffmpeg.dylib

install:
	mkdir -p ~/.config/Shadron/extensions
	cp -f shadron-ffmpeg.dylib ~/.config/Shadron/extensions/shadron-ffmpeg.dylib

clean:
	rm -f shadron-ffmpeg.dylib
