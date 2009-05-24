Name:        qelectrotech
Summary:     Design electric diagrams
Summary(fr): Réaliser des schémas électriques

# Upstream version is a float so 0.11 < 0.2 < 0.21 < 0.3
# So use %.2f with upstream acknowledgment
Version:     0.20
Release:     0.2.beta%{?dist}

Group:      Applications/Productivity
License:    GPLv2
Url:        http://qelectrotech.tuxfamily.org/
# svn export -r 374 svn://svn.tuxfamily.org/svnroot/qet/qet/trunk qelectrotech-svn374
# tar cvjf /home/rpmbuild/SOURCES/qelectrotech-svn374.tar.bz2 --exclude .svn qelectrotech-svn374
#Source0:    qelectrotech-svn374.tar.bz2
#Source0:    http://download.tuxfamily.org/qet/tags/20080831/qelectrotech-0.11-src.tar.gz
Source0:    http://download.tuxfamily.org/qet/tags/20090523/qelectrotech-0.2b-src.tar.gz

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:    desktop-file-utils
Requires(post):   desktop-file-utils
Requires(postun): desktop-file-utils
%if 0%{?fedora} > 8
BuildRequires:    qt-devel >= 4.4.1
%else
BuildRequires:    qt4-devel >= 4.4.1
%endif

%description
QElectroTech is a Qt4 application to design electric diagrams. It uses XML  
files for elements and diagrams, and includes both a diagram editor and an 
element editor.

%description -l fr
QElectroTech est une application Qt4 pour réaliser des schémas électriques.
QET utilise le format XML pour ses éléments et ses schémas et inclut un
éditeur de schémas ainsi qu'un éditeur d'élément.


%prep
#%setup -q -n %{name}-%{version}
#%setup -q -n %{name}-svn374
%setup -q -n %{name}-0.2b-src

sed -e s,/usr/local/,%{_prefix}/, \
    -e /QET_MAN_PATH/s,'man/','share/man', \
    -e /QET_MIME/s,../,, \
    -i %{name}.pro

qmake-qt4 %{name}.pro


%build
make %{?_smp_mflags}


%install
rm -fr $RPM_BUILD_ROOT
INSTALL_ROOT=$RPM_BUILD_ROOT make install

rm -rf $RPM_BUILD_ROOT/usr/doc/qelectrotech \
       $RPM_BUILD_ROOT%{_mandir}/fr.ISO8859-1 \
       $RPM_BUILD_ROOT%{_mandir}/fr

mv $RPM_BUILD_ROOT%{_mandir}/fr.UTF-8 $RPM_BUILD_ROOT%{_mandir}/fr


%clean
rm -fr $RPM_BUILD_ROOT


%post
update-desktop-database &> /dev/null || :
touch --no-create %{_datadir}/icons/hicolor
if [ -x %{_bindir}/gtk-update-icon-cache ]; then
  %{_bindir}/gtk-update-icon-cache --quiet %{_datadir}/icons/hicolor || :
fi

%postun
update-desktop-database &> /dev/null || :
touch --no-create %{_datadir}/icons/hicolor
if [ -x %{_bindir}/gtk-update-icon-cache ]; then
  %{_bindir}/gtk-update-icon-cache --quiet %{_datadir}/icons/hicolor || :
fi


%files
%defattr(-,root,root)
%doc CREDIT LICENSE README examples ELEMENTS.LICENSE
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/mime/application/x-qet-*.xml
%{_datadir}/mime/packages/%{name}.xml
%{_datadir}/mimelnk/application/x-qet-*.desktop
%{_datadir}/icons/hicolor/*/*/*.png
%{_datadir}/%{name}
%{_mandir}/man1/%{name}.*
%lang(fr) %{_mandir}/fr/man1/%{name}.*


%changelog
* Sun May 24 2009 Remi Collet <RPMS@FamilleCollet.com> - 0.20-0.2.beta
- update to 0.2b

* Sun Apr 12 2009 Remi Collet <RPMS@FamilleCollet.com> - 0.20-0.1.alpha
- update to 0.2alpha

* Sat Sep 06 2008 Remi Collet <RPMS@FamilleCollet.com> - 0.11-1
- update to 0.11

* Fri Aug 15 2008 Remi Collet <RPMS@FamilleCollet.com> - 0.1-3.svn374
- update to 1.0 finale
- new snapshot (pre 0.11)
- require qt >= 4.4.1

* Tue May 20 2008 Remi Collet <RPMS@FamilleCollet.com> - 0.1-2
- Fedora 9 build
- add gcc43.patch 
- add bug12.patch

* Sun Mar  2 2008 Remi Collet <RPMS@FamilleCollet.com> - 0.1-1
- update to 0.1 finale

* Sat Jan 19 2008 Remi Collet <RPMS@FamilleCollet.com> - 0.1-0.1.svn278
- add icon

* Mon Jan 07 2008 Remi Collet <RPMS@FamilleCollet.com> - 0.1-0.1.svn266
- first RPM

