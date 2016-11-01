Instructions for generating sequential test numbers from the tests_tilde.cpp file

The file tests_tilde.cpp contains the master list of tests to be run by qavpid.
Additions and deletions may be made to this file at will, as long as each test line ends in '~'.
The file can have the tildes replaced by test numbers by running the reseq.vim script in the vim editor.
To do this, perform the following.

1.  Copy the reseq.vim file to your vim runtime directory (probably ~/.vim)
2.  Run "vim tests_tilde.cpp"  (All commands are case sensitive)
3.  In vim, run ":runtime reseq.vim"
4.  Run ":1,$call Resequence()"
5.  Run ":w! tests.cpp"
6.  Run ":q!"


You can now run make to rebuild qavpid with the new tests.cpp file.

