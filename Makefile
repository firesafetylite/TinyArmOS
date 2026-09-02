.PHONY: all clean test

all:
	./build.sh

test:
	python3 -m unittest discover -s tests -v

clean:
	rm -rf build
