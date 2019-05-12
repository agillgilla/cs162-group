# -*- perl -*-
use strict;
use warnings;
use tests::tests;
use tests::random;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(cache-hit) begin
(cache-hit) make "cache"
(cache-hit) create "cache"
(cache-hit) open "cache"
(cache-hit) close "cache"
(cache-hit) reset buffer
(cache-hit) open "cache"
(cache-hit) close "cache"
(cache-hit) open "cache"
(cache-hit) close "cache"
(cache-hit) New hit rate is higher than old hit rate
(cache-hit) end
EOF
pass;
