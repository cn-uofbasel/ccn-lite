Name:		CCN-Lite
Version:	0.3.0	
Release:	1%{?dist}
Summary:	CCN-lite

Group:		Application/Internet
License:	ISC
URL:		ccn-lite.net

#BuildRequires:	
#Requires:	

%description
CCN-Lite is a 

#%prep
#%setup -q
#copy ccn-lite src into dir, or check it out?

#%build
#%configure
#make %{?_smp_mflags}
#make the bin files, 

%install
#%make_install
#use a make install script, which matches the requirements (install from bin...)
mkdir ../BUILDROOT/%name-%version-%release.%_arch/usr
mkdir ../BUILDROOT/%name-%version-%release.%_arch/usr/local
mkdir ../BUILDROOT/%name-%version-%release.%_arch/usr/local/bin
pwd
cp -L ../../../../bin/ccn* ../BUILDROOT/%name-%version-%release.%_arch/usr/local/bin


%files
%doc
/usr/local/bin/ccn*


%changelog

