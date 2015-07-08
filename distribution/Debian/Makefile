section:=base
arch:=amd64
version:=0.3.0
installedSize:=$(shell du -ks ccn-lite/usr|cut -f 1)
year:=$(shell date +"%Y")

all: deb

deb:
	mkdir ccn-lite
	mkdir ccn-lite/DEBIAN
	mkdir ccn-lite/usr
	mkdir ccn-lite/usr/local
	mkdir ccn-lite/usr/local/bin
	cp -L ../../bin/ccn-* ccn-lite/usr/local/bin
	cp Metadata ccn-lite/DEBIAN/control
	cp License ccn-lite/DEBIAN/copyright
	sed -i 's/<%= version %>/${version}/g' ccn-lite/DEBIAN/control
	sed -i 's/<%= section %>/${section}/g' ccn-lite/DEBIAN/control
	sed -i 's/<%= arch %>/${arch}/g' ccn-lite/DEBIAN/control
	sed -i 's/<%= installedSize %>/${installedSize}/g' ccn-lite/DEBIAN/control
	sed -i 's/<%= year %>/${year}/g' ccn-lite/DEBIAN/control
	sed -i 's/<%= year %>/${year}/g' ccn-lite/DEBIAN/copyright
	dpkg --build ccn-lite
	#rm -r ./ccn-lite	
	
clean:
	rm -rf ./ccn-lite
	rm -f ./ccn-lite.deb
