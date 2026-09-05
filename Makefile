.PHONY: all clean test bios bios-test native native-test

all:
	./build.sh

test:
	python3 -m unittest discover -s tests -v

bios:
	./tools/build_bios.sh

bios-test: bios
	python3 tools/test_bios.py

native:
	./tools/build_native.sh

native-test: native
	python3 tools/test_native.py

clean:
	rm -rf build
