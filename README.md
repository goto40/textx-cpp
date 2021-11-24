# textx-cpp

## Plan

Create an interpreter for textx grammars.

## Details

### grammar.h

 * Responsability: container for rules and interface to trigger parsing.

### arpeggio.h

 * Responsability: basic pasrer functionaity, inspirated by [Arpeggio](https://github.com/textX/Arpeggio).
 * Code config, arpeggio.h: `#define ARPEGGIO_USE_BOOST_FOR_REGEX` activates the boost version of regex; else the std-lib version is used. Since the boost is *much* faster, this define is introduced (+CMakeLists.txt adaptations).

## Links

 * Textx, Arpeggio, etc: [https://github.com/textX](https://github.com/textX)
 * Motivation for basic Arpeggio re-impl: [https://blog.bruce-hill.com/](https://blog.bruce-hill.com/packrat-parsing-from-scratch)

## Open Points

### Unclear
 * is it correct that either "eolterm", or a repeat modifier, but not both are allowed...? test in textx... (for repeat_operator and assignment)
### Open
 * "TODO determine common type here" attribute_info, Rule.h
 * "eolterm" not used/interpreted (for repeat_operator and assignment)
 * TODO: use has_match_suppression
