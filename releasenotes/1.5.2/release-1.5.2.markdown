1.5.2: 

Rofi 1.5.2 is another bug-fix relaese in the 1.5 serie. 

## Fix border drawing

Issue: #792, #783

There turned out to be a bug in how borders are drawn. 


## Improve Icon handling

Issue: #860

Several bugs around Icon handling have been fixed:

* Failing to load multiple (identical icons) on initial load.
* Preload user-set icon theme.
* Uuse a common threadpool in rofi for the icon fetching, instead of spawning a custom one.


## Documentation updates

Issue: #879, #867, #837, #831, #804

## Improving the ssh known hosts file parser

Issue: #820

## Additions

### Option to change the negate character

Issue: #877

The option to negate a query: `foo -bar` e.g. search for all items matching `foo` but not `bar` caused a lot of
confusion. It seems people often use rofi to also add arguments to applications (that start with a -).

To help with this, the negate character (`-`) can be changed, or disabled.


### Modify the DRUN display string

Issue: #858

An often requested option....



v1.5.2:
	- Fix assert and update test. (#875)
	- Add missing example Script (#869)
	- Add terminals to rofi-sensible-terminal (#808)
	- Lexer Fix several ambiguity and handling of empty input.
	- Lexer support environment variables.
	- Cleanup syntax for sorting. (#816)
