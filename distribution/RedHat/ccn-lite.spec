Name:		CCN-Lite
Version:	00.1	
Release:	1%{?dist}
Summary:	CCN-lite is a lightweigt implementation....

Group:		Application/Internet	
License:	MIT
URL:		ccn-lite.net

#BuildRequires:	
#Requires:	

%description


#%prep
#%setup -q


#%build
#%configure
#make %{?_smp_mflags}


%install
#%make_install
mkdir ../BUILDROOT/CCN-Lite-00.1-1.fc22.x86_64/usr
mkdir ../BUILDROOT/CCN-Lite-00.1-1.fc22.x86_64/usr/local
mkdir ../BUILDROOT/CCN-Lite-00.1-1.fc22.x86_64/usr/local/bin
cp -L $CCNL_HOME/bin/ccn* ../BUILDROOT/CCN-Lite-00.1-1.fc22.x86_64/usr/local/bin

%files
%doc
/usr/local/bin/ccn*


%changelog

