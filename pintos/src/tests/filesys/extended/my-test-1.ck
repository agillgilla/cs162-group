# -*- perl -*-
use strict;
use warnings;
use tests::tests;
use tests::random;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(my-test-1) begin
(my-test-1) make "cache"
(my-test-1) create "cache"
(my-test-1) open "cache"
(my-test-1) close "cache"
(my-test-1) reset buffer
(my-test-1) open "cache"
(my-test-1) close "cache"
(my-test-1) open "cache"
(my-test-1) close "cache"
(my-test-1) New hit rate is higher than old hit rate
(my-test-1) end
EOF
pass;
