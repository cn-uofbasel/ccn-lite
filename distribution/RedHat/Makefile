workdir:=$(shell pwd) 

all: rpm

rpm:
	mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
	echo '%_topdir ${workdir}/rpmbuild' > ~/.rpmmacros
	rpmbuild -ba ccn-lite.spec

clean:
	rm -rf ~/rpmbuild
