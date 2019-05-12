# -*- perl -*-
use strict;
use warnings;
use tests::tests;
use tests::random;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(my-test-2) begin
(my-test-2) make "myfile1"
(my-test-2) create "myfile1"
(my-test-2) open "myfile1"
(my-test-2) close "myfile1"
(my-test-2) make "myfile2"
(my-test-2) create "myfile2"
(my-test-2) open "myfile2"
(my-test-2) close "myfile2"
(my-test-2) make "myfile3"
(my-test-2) create "myfile3"
(my-test-2) open "myfile3"
(my-test-2) close "myfile3"
(my-test-2) read "myfile2"
(my-test-2) open "myfile2"
(my-test-2) close "myfile2"
(my-test-2) Correctly hit cache on read
(my-test-2) read "myfile1"
(my-test-2) open "myfile1"
(my-test-2) close "myfile1"
(my-test-2) Correctly fetched from disk
(my-test-2) end
EOF
pass;
