deb:
	mkdir ccn-lite
	mkdir ccn-lite/DEBIAN
	mkdir ccn-lite/usr
	mkdir ccn-lite/usr/local
	mkdir ccn-lite/usr/local/bin
	cp -L ../../bin/ccn-* ccn-lite/usr/local/bin
	cp Metadata ccn-lite/DEBIAN/control
	dpkg --build ccn-lite
	rm -r ./ccn-lite	
	
clean:
	rm -rf ./ccn-lite
	rm -f ./ccn-lite.deb
