# Migration NSIS 2.x → 3.x — QElectroTech

## Fichiers modifiés

| Fichier | Changement |
|---|---|
| `QET64.nsi` | Migration complète (voir détails ci-dessous) |
| `lang_extra_fr.nsh` | Réencodé ISO-8859-1 → UTF-8 |
| `lang_extra.nsh` | Inchangé (déjà UTF-8) |

---

## Détail des changements dans `QET64.nsi`

### 1. `SetCompressor` déplacé en tête de fichier
**NSIS 3 exige** que `SetCompressor` apparaisse avant toute `Section` ou `Function`.  
Avant il était après les `!include` et `!define` — cela fonctionnait en NSIS 2 mais
génère un avertissement/erreur en NSIS 3.

```nsis
; ✅ NSIS 3 — en tout premier
SetCompressor /FINAL /SOLID lzma
```

### 2. `XPStyle on` supprimé → remplacé par `ManifestSupportedOS`
`XPStyle on` est **retiré en NSIS 3**. Il était utilisé pour activer les styles visuels
Windows XP/Vista. NSIS 3 gère cela via le manifeste de l'exécutable :

```nsis
; ✅ NSIS 3
ManifestSupportedOS all
ManifestDPIAware true
```

### 3. `Var /GLOBAL` déclarées au niveau global
En NSIS 2, des `Var /GLOBAL` à l'intérieur d'une `Section` compilaient sans erreur.
En NSIS 3, les variables doivent être déclarées **au niveau du script** (avant toute section).

```nsis
; ✅ NSIS 3 — déclaration globale
Var final_qet_exe
Var final_project_ico
Var final_element_ico
Var final_titleblock_ico
```

### 4. `SetRegView 64` ajouté dans `.onInit`, section cachée, et `un.onInit`
En NSIS 3, sur un système 64 bits, **la redirection de registre WOW6432Node est active
par défaut**. Sans `SetRegView 64`, les clés HKLM atterrissent dans
`HKLM\SOFTWARE\Wow6432Node\` au lieu de `HKLM\SOFTWARE\`.

Ajouté à trois endroits :
- `Function .onInit` → pour lire la clé d'installation existante
- Section cachée (registry/shortcuts) → avant les `WriteRegStr`
- `Section "Uninstall"` → pour supprimer les bonnes clés
- `Function un.onInit` → cohérence avec l'installation

### 5. Icônes MUI mises à jour
Les icônes `XPUI-install.ico` / `XPUI-uninstall.ico` sont remplacées par les nouvelles
icônes NSIS 3 :

```nsis
; ✅ NSIS 3
!define MUI_ICON   "${NSISDIR}\Contrib\Graphics\Icons\nsis3-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\nsis3-uninstall.ico"
```
> Les anciennes icônes XPUI sont encore présentes pour compatibilité ascendante, mais
> les nouvelles sont recommandées.

### 6. `${IfNot} ${RunningX64}` — syntaxe clarifiée
Remplacement du bloc `${If}/${Else}/${EndIf}` moins lisible par `${IfNot}` plus concis :

```nsis
; ✅ NSIS 3 — idiome recommandé
${IfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONSTOP $(wrongArch)
    Abort
${EndIf}
```

### 7. URLs HTTP → HTTPS
Les URLs `http://qelectrotech.org/` ont été mises à jour en `https://qelectrotech.org/`
(cosmétique, sans impact sur la compilation).

### 8. `lang_extra_fr.nsh` — Réencodage ISO-8859-1 → UTF-8
NSIS 3 est **Unicode natif** : tous les fichiers `.nsh` doivent être encodés en UTF-8
(avec ou sans BOM). Le fichier `lang_extra_fr.nsh` était en ISO-8859-1 — il a été
converti en UTF-8.

> **Commande utilisée :** `iconv -f ISO-8859-1 -t UTF-8 lang_extra_fr.nsh`

---

## Ce qui N'A PAS changé (déjà compatible NSIS 3)

- `!include "MUI2.nsh"` — inchangé ✅
- `!define MUI_LANGDLL_ALLLANGUAGES` — toujours supporté ✅
- `!insertmacro MUI_RESERVEFILE_LANGDLL` — inchangé ✅
- `FileFunc.nsh` / macro `Locate` — inchangé ✅
- `RequestExecutionLevel admin` — inchangé ✅
- `InstallDir "$PROGRAMFILES64\..."` — inchangé ✅
- Structure Sections / SubSections — inchangée ✅
- `!insertmacro MUI_FUNCTION_DESCRIPTION_*` — inchangé ✅
- Toutes les `LangString` — inchangées ✅
- `Dutch_Belgium` via `Contrib/` — inchangé ✅

---

## Checklist avant compilation avec NSIS 3

- [ ] NSIS 3.x installé (https://nsis.sourceforge.io/)
- [ ] Tous les plugins utilisés sont la version Unicode/NSIS3
  - `FileFunc.nsh` : fourni avec NSIS 3 ✅
  - `x64.nsh` : fourni avec NSIS 3 ✅
  - `MUI2.nsh` : fourni avec NSIS 3 ✅
- [ ] `lang_extra.nsh` et `lang_extra_fr.nsh` encodés en UTF-8
- [ ] Le fichier `Contrib/Dutch_Belgium.nlf` / `.nsh` est placé dans
  `%NSIS%\Contrib\Language files\` ou référencé via un chemin relatif
