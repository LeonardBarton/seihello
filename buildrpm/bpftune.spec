# BPF/tracing tools for auto-tuning SPEC file

%define name        bpftune
%define release     1%{?dist}
%define version     0.1

License:        LGPL/BSD Dual License
Name:           %{name}
Summary:        BPF/tracing tools for auto-tuning Linux
Group:          Development/Tools
Requires:       libbpf >= 0.6
Requires:       libnl3
BuildRequires:  libbpf-devel >= 0.6
BuildRequires:  libnl3-devel
#BuildRequires:  clang >= 11
#BuildRequires:  clang-libs >= 11
#BuildRequires:  llvm >= 11
#BuildRequires:  llvm-libs >= 11
Version:        %{version}
Release:        %{release}
Source:         bpftune-%{version}.tar.bz2
Prefix:         %{_prefix}

%description
Service consisting of daemon (bpftune) and plugins which
support auto-tuning of Linux via BPF observability.

%prep
%setup -q -n bpftune-%{version}

%build
make

%install
rm -Rf %{buildroot}
%make_install

%files
%defattr(-,root,root)
%{_sbindir}/bpftune
%{_unitdir}/bpftune.service
%{_libdir}/libbpftune.so
%{_libdir}/libbpftune.so.0.1.1
%{_libdir}/bpftune/*
%{_mandir}/*/*

%license LICENSE

%changelog
* Mon May 30 2022 Alan Maguire <alan.maguire@oracle.com> - 0.1-1
  - Initial packaging support
