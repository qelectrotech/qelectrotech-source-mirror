#global svn 854

Name:        qelectrotech

Summary:     An electric diagrams editor
Summary(fr): Un éditeur de schémas électriques
Summary(es): Un editor de esquemas eléctricos
Summary(pt): Um editor de esquemas eléctricos
Summary(ru): Редактор электрических схем
Summary(cs): Editor nákresů elektrických obvodů

# Upstream version is a float so 0.11 < 0.2 < 0.21 < 0.3
# So use %.2f with upstream acknowledgment
# Remember to check Source0 + setup on each update
Version:     0.21
Release:     1%{?dist}

Group:       Applications/Engineering

# Prog is GPLv2 - Symbols/Elements are Creative Commons Attribution
License:    GPLv2+

Url:        http://qelectrotech.org/
%if 0%{?svn}
# svn export svn://svn.tuxfamily.org/svnroot/qet/qet/trunk qelectrotech
# mv qelectrotech qelectrotech-854
# tar cjf tar cjf /home/rpmbuild/SOURCES/qelectrotech-854.tar.bz2 qelectrotech-854
Source0:    qelectrotech-%{svn}.tar.bz2
%else
Source0:    http://download.tuxfamily.org/qet/tags/20100305/qelectrotech-0.21-src.tar.gz
%endif


BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:    desktop-file-utils
Requires(post):   desktop-file-utils
Requires(postun): desktop-file-utils
BuildRequires:    qt4-devel >= 4.4.1
Requires:         qelectrotech-symbols = %{version}-%{release}
Requires:         electronics-menu

%description
QElectroTech is a Qt4 application to design electric diagrams. It uses XML  
files for elements and diagrams, and includes both a diagram editor and an 
element editor.

%description -l es
QElectroTech es una aplicación Qt4 para diseñar esquemas eléctricos.
Utiliza archivos XML para los elementos y esquemas, e incluye un editor 
de esquemas y un editor de elemento.

%description -l fr
QElectroTech est une application Qt4 pour réaliser des schémas électriques.
QET utilise le format XML pour ses éléments et ses schémas et inclut un
éditeur de schémas ainsi qu'un éditeur d'élément.

%description -l ru
QElectroTech - приложение написанное на Qt4 и предназначенное для разработки
электрических схем. Оно использует XML-файлы для элементов и схем, и включает,
как редактор схем, так и редактор элементов.

%description -l pt
QElectroTech é uma aplicação baseada em Qt4 para desenhar esquemas eléctricos.
QET utiliza ficheiros XML para os elementos e para os esquemas e inclui um
editor de esquemas e um editor de elementos.

%description -l cs
QElectroTech je aplikací Qt4 určenou pro návrh nákresů elektrických obvodů.
Pro prvky a nákresy používá soubory XML, a zahrnuje v sobě jak editor nákresů,
tak editor prvků.



%package symbols
Summary:     Elements collection for QElectroTech
Summary(es): Collección de elementos para QElectroTech
Summary(fr): Collection d'élements pour QElectroTech
Summary(pt): Colecção de elementos para QElectroTech
Summary(ru): Коллекция элементов для QElectroTech
Summary(cs): Sbírka prvků pro QElectroTech
Group:       Applications/Productivity
License:     CC-BY
%if 0%{?fedora} >= 11
BuildArch:   noarch
%endif
Requires:    qelectrotech = %{version}-%{release}


%description symbols
Elements collection for QElectroTech.

%description -l es symbols
Collección de elementos para QElectroTech.

%description -l fr symbols
Collection d'élements pour QElectroTech.

%description -l pt symbols
Colecção de elementos para QElectroTech.

%description -l ru symbols
Коллекция элементов для QElectroTech.

%description -l cs symbols
Sbírka prvků pro QElectroTech.

%prep
%if 0%{?svn}
%setup -q -n %{name}-%{svn}
%else
%setup -q -n %{name}-0.21-src
%endif

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

# We only provides UTF-8 files
rm -rf $RPM_BUILD_ROOT/usr/doc/qelectrotech \
       $RPM_BUILD_ROOT%{_mandir}/fr.ISO8859-1 \
       $RPM_BUILD_ROOT%{_mandir}/fr

mv $RPM_BUILD_ROOT%{_mandir}/fr.UTF-8 $RPM_BUILD_ROOT%{_mandir}/fr

desktop-file-install --vendor="" \
   --remove-category=Office \
   --add-category=Electronics \
   --dir=%{buildroot}%{_datadir}/applications/ \
         %{buildroot}%{_datadir}/applications/%{name}.desktop

# QT translation provided by QT.
rm -f $RPM_BUILD_ROOT%{_datadir}/%{name}/lang/qt_*.qm

%if 0%{?fedora} >= 9
%find_lang qet --with-qt
%else
for lg in en es fr pt ru cs
do
  echo "%%lang($lg) %{_datadir}/%{name}/lang/qet_$lg.qm"
done | tee qet.lang
%endif


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
%doc CREDIT LICENSE examples
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
%lang(es) %{_mandir}/es/man1/%{name}.*
%lang(fr) %{_mandir}/fr/man1/%{name}.*
%lang(pt) %{_mandir}/pt/man1/%{name}.*


%files symbols
%defattr(-,root,root,-)
%doc ELEMENTS.LICENSE
%{_datadir}/%{name}/elements


%changelog
* Sat Mar 06 2010 Remi Collet <Fedora@FamilleCollet.com> - 0.21-1
- update to 0.21
- more translations (sumnary and description)

* Sun Feb 14 2010 Remi Collet <Fedora@FamilleCollet.com> - 0.21-0.1.svn854
- update to SVN snapshot

* Sat Jun 27 2009 Remi Collet <Fedora@FamilleCollet.com> - 0.20-1.fc8.remi
- rebuild for Fedora 8

* Sat Jun 27 2009 Remi Collet <Fedora@FamilleCollet.com> - 0.20-1
- update to 0.2 finale

* Sat Jun 20 2009 Remi Collet <Fedora@FamilleCollet.com> - 0.20-0.3.rc2
- rebuild for remi repo 
- add Obsoletes -elements

* Sat Jun 20 2009 Remi Collet <Fedora@FamilleCollet.com> - 0.20-0.2.rc2
- update to RC2

* Thu Jun 18 2009 Remi Collet <Fedora@FamilleCollet.com> - 0.20-0.2.rc1
- changes from review (#505867)
- add multi-lang sumnary (taken from .desktop)
- add multi-lang description (taken from README)
- rename qlectrotech-elements to -symbols
- use electronics-menu

* Sun Jun 14 2009 Remi Collet <Fedora@FamilleCollet.com> - 0.20-0.1.rc1
- initial RPM for fedora

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

