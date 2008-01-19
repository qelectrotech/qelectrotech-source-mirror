%define svnrel 278

Name:        qelectrotech
Summary:     Design electric diagrams
Summary(fr): Réaliser des schémas électriques
Version:     0.1
Release:     0.1.svn%{svnrel}%{?dist}

Group:      Applications/Productivity
License:    GPLv2
Url:        http://qelectrotech.tuxfamily.org/
# svn co svn://svn.tuxfamily.org/svnroot/qet/qet/trunk qelectrotech
# tar cvjf /home/rpmbuild/SOURCES/qelectrotech-svn.tar.bz2 --exclude .svn qelectrotech
Source0:    qelectrotech-svn.tar.bz2

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  qt4-devel
BuildRequires:  desktop-file-utils

%description
QElectroTech is a Qt4 application to design electric diagrams. It uses XML  
files for elements and diagrams, and includes both a diagram editor and an 
element editor.

%description -l fr
QElectroTech est une application Qt4 pour réaliser des schémas électriques.
QET utilise le format XML pour ses éléments et ses schémas et inclut un
éditeur de schémas ainsi qu'un éditeur d'élément.


%prep
%setup -q -n %{name}
sed -i -e s,/usr/local/,%{_prefix}/, %{name}.pro
qmake-qt4 %{name}.pro

cat >%{name}.desktop <<EOF 
[Desktop Entry]
Version=1.0
Encoding=UTF-8
Name=QElectroTech
Type=Application
Exec=qelectrotech
Icon=qelectrotech
Terminal=0
EOF


%build
make %{?_smp_mflags}


%install
rm -fr $RPM_BUILD_ROOT
INSTALL_ROOT=$RPM_BUILD_ROOT make install
rm $RPM_BUILD_ROOT/usr/doc/qelectrotech/LICENSE
#install -d $RPM_BUILD_ROOT%{_bindir}
#install -m 755 %{name} $RPM_BUILD_ROOT%{_bindir}

%{__mkdir_p} $RPM_BUILD_ROOT%{_datadir}/pixmaps
%{__install} -p -D -m 644 ico/qet.png $RPM_BUILD_ROOT%{_datadir}/pixmaps/%{name}.png

desktop-file-install  --vendor "" \
        --dir $RPM_BUILD_ROOT%{_datadir}/applications \
        --add-category Application \
        --add-category X-Fedora \
        --add-category Office \
        %{name}.desktop

%clean
rm -fr $RPM_BUILD_ROOT


%post
update-desktop-database &> /dev/null ||:


%postun
update-desktop-database &> /dev/null ||:


%files
%defattr(-,root,root)
%doc LICENSE CREDIT README
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/pixmaps/%{name}.png
%{_datadir}/%{name}


%changelog
* Sat Jan 19 2008 Remi Collet <RPMS@FamilleCollet.com> - 1.0-0.1.svn278
- add icon

* Mon Jan 07 2008 Remi Collet <RPMS@FamilleCollet.com> - 1.0-0.1.svn266
- first RPM

