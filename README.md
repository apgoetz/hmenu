OVERVIEW
======== 

hmenu provides a simple wrapper around the commonly used dmenu command
to display previously selected entries ordered by usage. Its behavior
is similar to firefox's awesome bar. hmenu outputs the selected item
to stdout, like dmenu, but it then keeps track of the selected item,
and its frequency of use with respect to the past history of
selections in an SQLite table. 

USAGE
=====

hmenu uses environmental variables to set its specific parameters, and
passes any arguments it receives to dmenu. The environmental variables
that are important are:

**HMENU_DB:** name of file SQLite database of past history is stored
  in. If not set, hmenu behaves like dmenu.

**HMENU_CHILDMENU:** name of dmenu executable to run. If left blank,
  hmenu tries to execute the program 'dmenu' in the user's path.



BUILDING
========

Use the following command:

```bash

gcc -g -o hmenu hmenu.c -lsqlite3

```