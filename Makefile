all:
	mkdir -p build
	cd build && \
	cmake .. && \
	make

install: ./build/ssehub ./conf/config.json.example
	install -m 0755 -o root -g root -s ./build/ssehub /usr/bin/ssehub
	install -m 0644	-o root -g root	-D ./conf/config.json.example /etc/ssehub/config.json.example

clean:
	rm -rf build

docker:
	docker build -t ssehub .