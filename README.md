Transliteration
=================

**Important note**: Transliteration module was moved into Backdrop core in version 
1.3.0. Use of this module is no longer necessary and no further changes will be 
made here. If you would like to report a bug or feature request against 
Transliteration module, file an issue in the main Backdrop CMS core repository 
at https://github.com/backdrop/backdrop-issues.

Provides a central transliteration service to other Backdrop modules, and
sanitizes file names while uploading.

## Installation

-   Install this module using the official Backdrop CMS instructions at
    https://backdropcms.org/guide/modules.

-   If you are installing to an existing Backdrop site, you might want to fix
    existing file names after installation, which will update all file names
    containing non-ASCII characters. However, if you have manually entered links
    to those files in any contents, these links will break since the original
    files are renamed. Therefore it is a good idea to test the conversion
    first on a copy of your web site. You'll find the retroactive conversion at
    Configuration >> Media >> File system >> Transliteration.

## Configuration

This module doesn't require special permissions.

This module can be configured from the File system configuration page
(Configuration >> Media >> File system >> Settings).

*   Transliterate file names during upload: If you need more control over the
    resulting file names you might want to disable this feature here and install
    the FileField Paths module instead.

*   Lowercase transliterated file names: It is recommended to enable this option
    to prevent issues with case-insensitive file systems.

## 3rd Party integration

Third party developers seeking an easy way to transliterate text or file names
may use transliteration functions as follows:

```php
if (function_exists('transliteration_get')) {
  $transliterated = transliteration_get($text, $unknown, $source_langcode);
}
```

or, in case of file names:

```php
if (function_exists('transliteration_clean_filename')) {
  $transliterated = transliteration_clean_filename($filename, $source_langcode);
}
```

Note that the optional $source_langcode parameter specifies the language code
of the input. If the source language is not known at the time of transliter-
ation, it is recommended to set this argument to the site default language:

```php
$output = transliteration_get($text, '?', language_default('language'));
```

Otherwise the current display language will be used, which might produce
inconsistent results.

## Language specific replacements

This module supports language specific variations in addition to the basic
transliteration replacements. The following guide explains how to add them:

1.  First find the Unicode character code you want to replace. As an example,
    we'll be adding a custom transliteration for the cyrillic character 'г'
    (hexadecimal code 0x0433) using the ASCII character 'q' for Azerbaijani
    input.

2.  Transliteration stores its mappings in banks with 256 characters each. The
    first two digits of the character code (04) tell you in which file you'll
    find the corresponding mapping. In our case it is data/x04.php.

3.  If you open that file in an editor, you'll find the base replacement matrix
    consisting of 16 lines with 16 characters on each line, and zero or more
    additional language-specific variants. To add our custom replacement, we need
    to do two things: first, we need to create a new transliteration variant
    for Azerbaijani since it doesn't exist yet, and second, we need to map the
    last two digits of the hexadecimal character code (33) to the desired output
    string:

    ```php
    $variant['az'] = array(0x33 => 'q');
    ```

    (see http://people.w3.org/rishida/names/languages.html for a list of
    language codes).

    Any Azerbaijani input will now use the appropriate variant.

Also take a look at data/x00.php which already contains a bunch of language
specific replacements. If you think your overrides are useful for others please
file a patch at http://drupal.org/project/issues/transliteration.

## License

This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.

## Credits

Authors:
* Stefan M. Kudwien (smk-ka) - http://drupal.org/user/48898
* Daniel F. Kudwien (sun) - http://drupal.org/user/54136

Maintainers:
* Andrei Mateescu (amateescu) - http://drupal.org/user/729614

Backdrop maintainer:
* Jérôme Danthinne (jdanthinne) - https://github.com/jdanthinne/ 

UTF-8 normalization is based on UtfNormal.php from MediaWiki
(http://www.mediawiki.org) and transliteration uses data from Sean M. Burke's
Text::Unidecode CPAN module
(http://search.cpan.org/~sburke/Text-Unidecode-0.04/lib/Text/Unidecode.pm).

## Useful resources

Unicode Code Converter:
http://people.w3.org/rishida/tools/conversion/

UTF-8 encoding table and Unicode characters:
http://www.utf8-chartable.de/unicode-utf8-table.pl

Country codes:
http://www.loc.gov/standards/iso639-2/php/code_list.php
