# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(seek-bad-fd) begin
(seek-bad-fd) seek.txt
seek-bad-fd: exit(-1)
EOF
pass;
