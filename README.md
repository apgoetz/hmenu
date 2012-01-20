OVERVIEW
======== 

hmenu provides a simple wrapper around the commonly used dmenu command
to display previously selected entries ordered by usage. Its behavior
is similar to firefox's awesome bar. hmenu outputs the selected item
to stdout, like dmenu, but it then keeps track of the selected item,
and its frequency of use with respect to the past history of
selections in an SQLite table. If a perl script is passed in as a
parameter to hmenu, hmenu will try to call a the perl function with
the same name in that script before printing the selection to
stdout. For more detail, see PERL SCRIPT below.

USAGE
=====

hmenu uses environmental variables to set its specific parameters, and
passes any arguments it receives to dmenu. The environmental variables
that are important are:

**HMENU_DB:** name of file SQLite database of past history is stored
  in. If not set, hmenu behaves like dmenu.

**HMENU_CHILDMENU:** name of dmenu executable to run. If left blank,
  hmenu tries to execute the program 'dmenu' in the user's path.

**HMENU_PERL:** name of perl script to use to resolve commands


PERL SCRIPT
===========

If a perl script is defined as one of the parameters to hmenu, hmenu
will try to resolve functions with the same name as the first space
deliminated token passed selected from the list of choices. This
function, if it exists, will be executed, with the rest of the
selected string after the first token passed into the perl function.

For example, if the users selects the string 'foo bar bat' from hmenu;
hmenu will try to call the function 'foo' with the parameter 'bar
bat'.

BUILDING
========

Use the following command:

```bash

gcc -g -o hmenu hmenu.c -lsqlite3 `perl -MExtUtils::Embed -e ccopts -e ldopts`

```