# -*- perl -*-
use strict;
use warnings;
use tests::tests;
use tests::random;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(my-test-2) begin
(my-test-2) make 64
(my-test-2) close 64
(my-test-2) Correctly filled cold buffer to brimr
(my-test-2) Correctly got cache hit
(my-test-2) Correctly brought in new file to cache
(my-test-2) Correctly brought in new file to cache
(my-test-2) end
EOF
pass;
