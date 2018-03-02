Name:           divvyd
Version:        0.28.2
Release:        1%{?dist}
Summary:        Divvy peer-to-peer network daemon

Group:          Applications/Internet
License:        ISC
URL:            https://github.com/xdv/divvyd

# curl -L -o SOURCES/divvyd-release.zip https://github.com/xdv/divvyd/archive/release.zip
Source0:        divvyd-release.zip
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  gcc-c++ scons openssl-devel protobuf-devel
Requires:       protobuf openssl


%description
Divvyd is the server component of the Divvy network.


%prep
%setup -n divvyd-release


%build
# Assume boost is manually installed
export RIPPLED_BOOST_HOME=/usr/local/boost_1_55_0
scons -j `grep -c processor /proc/cpuinfo` build/divvyd


%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/%{name}
cp LICENSE %{buildroot}/usr/share/%{name}/
mkdir -p %{buildroot}/usr/bin
cp build/divvyd %{buildroot}/usr/bin/divvyd
mkdir -p %{buildroot}/etc/%{name}
cp doc/divvyd-example.cfg %{buildroot}/etc/%{name}/divvyd.cfg
mkdir -p %{buildroot}/var/lib/%{name}/db
mkdir -p %{buildroot}/var/log/%{name}


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root,-)
/usr/bin/divvyd
/usr/share/divvyd/LICENSE
/etc/divvyd/divvyd-example.cfg
