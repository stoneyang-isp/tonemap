tonemap: tonemap.cpp
	g++ -Wall tonemap.cpp -o tonemap `Magick++-config --cppflags --cxxflags --ldflags --libs` `pkg-config --cflags OpenEXR --libs OpenEXR`

test: tonemap ../images/tito.exr
	./tonemap ../images/tito.exr

clean:
	rm -rf tonemap test
