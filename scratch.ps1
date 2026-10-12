
$content = Get-Content "sources\qetinformation.cpp"
$content = $content -replace "COND_FUNCTION,", "ELMT_CROSS_REF, COND_FUNCTION,"
$content = $content -replace "ELMT_XREF,", "ELMT_XREF, ELMT_CROSS_REF,"
$content = $content -replace "else if \(info == ELMT_XREF\)                          return QObject::tr\(""Rf. croise""\);", "else if (info == ELMT_XREF)                          return QObject::tr(""Rf. croise"");`n`telse if (info == ELMT_CROSS_REF)                     return QObject::tr(""Querverweis (Folienverweis)"");"
Set-Content "sources\qetinformation.cpp" -Value $content

