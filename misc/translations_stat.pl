#!/usr/bin/perl
# Copyright 2006-2012 Xavier Guerrin
# This file is part of QElectroTech.
# 
# QElectroTech is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
# 
# QElectroTech is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.

# This script analyzes a QElectroTech Subversion working directory in order to
# output statistics on the translation progress. The output is a table
# conforming to the Dokuwiki syntax, as configured on qelectrotech.org/wiki.
# Typical usage: misc/translations_stat.pl

use strict;
use utf8;
use File::Find;
binmode(STDOUT, ":utf8");

# Configuration
our $working_directory = '.';

my $ok = '[ok]';
my $no = '[no]';
my $todo = 'TODO';

our $default_language = 'fr';
our @misc_desktop_files = qw(misc/qelectrotech.desktop misc/x-qet-element.desktop misc/x-qet-project.desktop misc/x-qet-titleblock.desktop);
our @misc_xml_files     = qw(misc/qelectrotech.xml     misc/x-qet-element.xml     misc/x-qet-project.xml     misc/x-qet-titleblock.xml);
our @readme_files       = qw(CREDIT README INSTALL ELEMENTS.LICENSE packaging/linux/fedora/README.elements);
our @ordered_languages  = qw(fr en es ru pt cs pl ca ro de it ar sl hr el);
our %languages = (
	'fr' => {
		'name'               => 'Français',
		'french_name'        => 'Français',
		'translator_name'    => 'Laurent Trinques',
		'translator_mail_ml' => 'scorpio@qelectrotech.org',
		'qt'                 => 1
	},
	'en' => {
		'name'               => 'English',
		'french_name'        => 'Anglais',
		'translator_name'    => 'Laurent Trinques',
		'translator_mail_ml' => 'scorpio@qelectrotech.org',
		'qt'                 => 1,
		'unix_manual'        => 1,
		'desktop_progress_summary' => $ok,
		'xml_progress_summary' => $ok
	},
	'es' => {
		'name'               => 'Español',
		'french_name'        => 'Espagnol',
		'translator_name'    => 'Alfredo Carreto',
		'translator_mail_ml' => 'electronicos_mx@yahoo.com.mx',
		'qt'                 => 1
	},
	'ru' => {
		'name'               => 'Russe',
		'french_name'        => 'Русский',
		'translator_name'    => 'Yuriy Litkevich',
		'translator_mail_ml' => 'lit-uriy@yandex.ru',
		'qt'                 => 1
	},
	'pt' => {
		'name'               => 'Portugais',
		'french_name'        => 'Português',
		'translator_name'    => 'Jose Carlos Martins',
		'translator_mail_ml' => 'jose@qelectrotech.org',
		'qt'                 => 1
	},
	'cs' => {
		'name'               => 'Czech',
		'french_name'        => 'Tchèque',
		'translator_name'    => 'Pavel Fric',
		'translator_mail_ml' => 'pavelfric@seznam.cz',
		'qt'                 => 1
	},
	'pl' => {
		'name'               => 'Polska',
		'french_name'        => 'Polonais',
		'translator_name'    => 'Pawel Smiech',
		'translator_mail_ml' => 'pawel32640@gmail.com',
		'qt'                 => 1
	},
	'ca' => {
		'name'               => 'Català',
		'french_name'        => 'Catalan',
		'translator_name'    => 'Youssef Ouamalkran & Eduard Amoros',
		'translator_mail_ml' => 'youssefsan@gmail.com',
		'qt'                 => 0
	},
	'ro' => {
		'name'               => 'Română',
		'french_name'        => 'Roumain',
		'translator_name'    => 'Gabi Mandoc',
		'translator_mail_ml' => 'gabriel.mandoc@gic.ro',
		'qt'                 => 0
	},
	'de' => {
		'name'               => 'Deutsch',
		'french_name'        => 'Allemand',
		'translator_name'    => 'Markus Budde & Jonas Stein &  Noah Braden',
		'translator_mail_ml' => 'markus.budde@msn.com',
		'qt'                 => 1
	},
	'it' => {
		'name'               => 'Italiano',
		'french_name'        => 'Italiano',
		'translator_name'    => 'Alessandro Conti & Silvio Brera',
		'translator_mail_ml' => 'dr.slump@alexconti.it',
		'qt'                 => 1
	},
	'ar' => {
		'name'               => 'Arabic',
		'french_name'        => 'Arabe',
		'translator_name'    => 'Mohamed Souabni',
		'translator_mail_ml' => 'souabnimohamed@yahoo.fr',
		'qt'                 => 1
	},
	'sl' => {
		'name'               => 'Slovenian',
		'french_name'        => 'Slovene',
		'translator_name'    => 'Uroš Platiše',
		'translator_mail_ml' => 'uros.platise@energycon.eu',
		'qt'                 => 1
	},
	'hr' => {
		'name'               => 'Croatian',
		'french_name'        => 'Croate',
		'translator_name'    => 'Antun Marakovic',
		'translator_mail_ml' => 'antun.marakovic@lolaribar.hr',
		'qt'                 => 1
	},
	'el' => {
		'name'               => 'Greek',
		'french_name'        => 'Grec',
		'translator_name'    => 'Nikos Papadopoylos &  Yannis Gyftomitros',
		'translator_mail_ml' => '231036448@freemail.gr',
		'qt'                 => 1
	}
	'nl' => {
		'name'               => 'Netherlands',
		'french_name'        => 'Pays-Bas',
		'translator_name'    => 'Shooter',
		'translator_mail_ml' => 'shooter@home.nl',
		'qt'                 => 1
	}
);

my $head_pattern = '^ %-32s ^ %-22s ^ %-8s ^ %-16s ^ %-16s ^ %-23s ^ %-20s ^ %-11s ^ %-7s ^ %-7s ^ %-7s ^ %-16s ^ %-38s ^'."\n";
my $line_pattern = $head_pattern;
$line_pattern =~ s/\^/\|/g;

# Functions
sub get_pattern_count_in_file {
	my $count = 0, my %params = @_;
	return -3 if (!defined($params{'pattern'}));
	return -2 if (!defined($params{'file'}) || ! -f $params{'file'});
	return -1 if (!open(my $fh, '<', $params{'file'}));
	while (<$fh>) {
		++ $count if (/$params{'pattern'}/);
		last if (defined($params{'limit'}) && $count == $params{'limit'});
	}
	close($fh);
	return $count;
}

sub analyze_element_file {
	return if (($File::Find::name =~ m/\.svn/) || (! -f $_));
	return if ($_ !~ m/(^qet_directory|\.elmt)$/);
	
	# One more element, count it
	our $elements_count;
	++ $elements_count;
	
	# Reports translation stat into the %languages hash
	my $file = $_;
	our %languages;
	for my $lang_key (our @ordered_languages) {
		my $translations_count = get_pattern_count_in_file('file' => $file, 'pattern' => sprintf('<name\s+lang="%s"\s*>', $lang_key));
		if ($translations_count == 1) {
			++ $languages{$lang_key}{'translated_elements_count'};
		} elsif ($translations_count > 1) {
			print STDERR "Multiple translation in $File::Find::name\n";
		}
	}
}

# Action
chdir($working_directory) or die(sprintf('could not chdir to %s', $working_directory));

# Store the total number of elements
our $elements_count = 0;

# Analyze the elements collection
finddepth(\&analyze_element_file, 'elements');

# Complete the "languages" hash with statistics
for my $lang_key (@ordered_languages) {
	my $lang = $languages{$lang_key};
	
	# strings statistics
	my $ts_file = sprintf('lang/qet_%s.ts', $lang_key);
	my $a = $lang->{'translated_strings_count'} = get_pattern_count_in_file('file' => $ts_file, 'pattern' => '<translation>');
	my $b = $lang->{'strings_count'}            = get_pattern_count_in_file('file' => $ts_file, 'pattern' => '<translation');
	my $c = $lang->{'strings_progress'}         = $b ? ($a / $b * 100) : 0;
	if ($lang->{'default'} || ($b && $a == $b)) {
		$lang->{'strings_progress_summary'} = $ok;
	} else {
		$lang->{'strings_progress_summary'} = sprintf('%i/%i (%.1f%%)', $a, $b, $c);
	}
	
	# elements statistics
	my $d = our $elements_count;
	my $e = $lang->{'translated_elements_count'};
	my $f = $lang->{'elements_progress'} = $d ? ($e / $d * 100) : 0;
	if ($d && $d == $e) {
		$lang->{'elements_progress_summary'} = $ok;
	} else {
		$lang->{'elements_progress_summary'} = sprintf('%i/%i (%.1f%%)', $e, $d, $f);
	}
	
	# misc files statistics
	if (!defined($lang->{'desktop_progress_summary'})) {
		my $desktop_count = 0;
		for my $desktop_file (@misc_desktop_files) {
			++ $desktop_count if (get_pattern_count_in_file('file' => $desktop_file, 'pattern' => sprintf('Comment\[%s.*\]', $lang_key), 'limit' => 1) == 1);
		}
		if (@misc_desktop_files && @misc_desktop_files == $desktop_count) {
			$lang->{'desktop_progress_summary'} = $ok;
		} else {
			$lang->{'desktop_progress_summary'} = sprintf('%i/%i', $desktop_count, scalar(@misc_desktop_files));
		}
	}
	
	# misc files statistics
	if (!defined($lang->{'xml_progress_summary'})) {
		my $xml_count = 0;
		for my $xml_file (@misc_xml_files) {
			++ $xml_count if (get_pattern_count_in_file('file' => $xml_file, 'pattern' => sprintf('xml:lang="%s.*"', $lang_key), 'limit' => 1) == 1);
		}
		if (@misc_xml_files && @misc_xml_files == $xml_count) {
			$lang->{'xml_progress_summary'} = $ok;
		} else {
			$lang->{'xml_progress_summary'} = sprintf('%i/%i', $xml_count, scalar(@misc_xml_files));
		}
	}
	
	# Unix manual
	if (!defined($lang->{'unix_manual'})) {
		$lang->{'unix_manual'} = 0;
		if (opendir(my $man_dh, 'man/files')) {
			for my $dir (grep { /^($lang_key(?:\..+)?)$/ } readdir($man_dh)) {
				if (-f 'man/files/'.$dir.'/man1/qelectrotech.1') {
					$lang->{'unix_manual'} = 1;
					last;
				}
			}
			closedir($man_dh);
		}
	}
	
	# CREDIT README INSTALL ELEMENTS.LICENSE files statistics
	for my $file (@readme_files) {
		if (!defined($lang->{$file})) {
			my $header_count = get_pattern_count_in_file(
				'file' => $file,
				'pattern' => sprintf('^\[%s\]$', $lang_key),
				'count' => 1
			);
			$lang->{$file} = ($header_count == 1) ? 1 : 0;
		}
	}
}

# Render a Dokuwiki table displaying the computed statistics
printf($head_pattern, 'Langue', 'Traducteur', 'Qt', 'Application', 'Collection', 'Fichiers misc/*.desktop', 'Fichiers misc/*.xml', 'Manuel Unix', @readme_files);
for my $lang_key (@ordered_languages) {
	my $lang = $languages{$lang_key};
	my @readme_values = ();
	for my $file (@readme_files) {
		push(@readme_values, $lang->{$file} ? $ok : $todo);
	}
	printf(
		$line_pattern,
		sprintf('\'\'[%s]\'\' %s (%s)', $lang_key, $lang->{'name'}, $lang->{'french_name'}),
		$lang->{'translator_name'},
		$lang->{'qt'} ? $ok : $no,
		$lang->{'strings_progress_summary'},
		$lang->{'elements_progress_summary'},
		$lang->{'desktop_progress_summary'},
		$lang->{'xml_progress_summary'},
		$lang->{'unix_manual'} ? $ok : $todo,
		@readme_values
	);
}

# Add date to output
print "\n";
my @date = localtime(time());
printf('Dernière date de mise à jour : %02d/%02d/%04d %02d:%02d'."\n", $date[3], $date[4] + 1, 1900 + $date[5], $date[2], $date[1]);
