Name:        qelectrotech
Summary:     Design electric diagrams
Summary(fr): Réaliser des schémas électriques

# Upstream version is a float so 0.11 < 0.2 < 0.21 < 0.3
# So use %.2f with upstream acknowledgment
Version:     0.20
Release:     0.3.rc1%{?dist}

Group:      Applications/Productivity
# Prog is GPLv2 - Element are Creative Commons Attribution
License:    GPLv2+

Url:        http://qelectrotech.org/
# svn export -r 374 svn://svn.tuxfamily.org/svnroot/qet/qet/trunk qelectrotech-svn374
# tar cvjf /home/rpmbuild/SOURCES/qelectrotech-svn374.tar.bz2 --exclude .svn qelectrotech-svn374
#Source0:    qelectrotech-svn374.tar.bz2
Source0:    http://download.tuxfamily.org/qet/tags/20090613/qelectrotech-0.2rc1-src.tar.gz

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:    desktop-file-utils
Requires(post):   desktop-file-utils
Requires(postun): desktop-file-utils
%if 0%{?fedora} >= 9
BuildRequires:    qt-devel >= 4.4.1
%else
BuildRequires:    qt4-devel >= 4.4.1
%endif
Requires:         qelectrotech-elements = %{version}-%{release}

%description
QElectroTech is a Qt4 application to design electric diagrams. It uses XML  
files for elements and diagrams, and includes both a diagram editor and an 
element editor.

%description -l fr
QElectroTech est une application Qt4 pour réaliser des schémas électriques.
QET utilise le format XML pour ses éléments et ses schémas et inclut un
éditeur de schémas ainsi qu'un éditeur d'élément.


%package elements
Summary:     Elements collection for QElectroTech
Summary(fr): Collection d'élements pour QElectroTech
Group:       Applications/Productivity
License:     CC-BY
%if 0%{?fedora} >= 11
BuildArch:   noarch
%endif


%description elements
Elements collection for QElectroTech.

%description -l fr elements
Collection d'élements pour QElectroTech.


%prep
#%setup -q -n %{name}-%{version}
#%setup -q -n %{name}-svn374
%setup -q -n %{name}-0.2rc1-src

sed -e s,/usr/local/,%{_prefix}/, \
    -e /QET_MAN_PATH/s,'man/','share/man', \
    -e /QET_MIME/s,../,, \
    -i %{name}.pro

qmake-qt4 %{name}.pro


%build
make %{?_smp_mflags}


%install
rm -fr $RPM_BUILD_ROOT qet.lang
INSTALL_ROOT=$RPM_BUILD_ROOT make install

rm -rf $RPM_BUILD_ROOT/usr/doc/qelectrotech \
       $RPM_BUILD_ROOT%{_mandir}/fr.ISO8859-1 \
       $RPM_BUILD_ROOT%{_mandir}/fr

mv $RPM_BUILD_ROOT%{_mandir}/fr.UTF-8 $RPM_BUILD_ROOT%{_mandir}/fr

desktop-file-install --vendor="" \
   --dir=%{buildroot}%{_datadir}/applications/ \
         %{buildroot}%{_datadir}/applications/%{name}.desktop

rm -f $RPM_BUILD_ROOT%{_datadir}/%{name}/lang/qt_*.qm
%find_lang qet --with-qt


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


%files -f qet.lang
%defattr(-,root,root,-)
%doc CREDIT LICENSE README examples
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/mime/application/x-qet-*.xml
%{_datadir}/mime/packages/%{name}.xml
%{_datadir}/mimelnk/application/x-qet-*.desktop
%{_datadir}/icons/hicolor/*/*/*.png
%dir %{_datadir}/%{name}
%dir %{_datadir}/%{name}/lang
%exclude %{_datadir}/%{name}/examples
%{_mandir}/man1/%{name}.*
%lang(fr) %{_mandir}/fr/man1/%{name}.*


%files elements
%defattr(-,root,root,-)
%doc ELEMENTS.LICENSE
%{_datadir}/%{name}/elements


%changelog
* Sat Jun 13 2009 Remi Collet <RPMS@FamilleCollet.com> - 0.20-0.3.rc1
- update to 0.2rc1
- split elements in sub-package
- fix license, URL, lang

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

