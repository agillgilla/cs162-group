# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF', <<'EOF']);
(geminus) begin
(geminus) geminus.txt
(geminus) end
geminus: exit(0)
EOF
(geminus) begin
(geminus) geminus.txt
geminus: exit(-1)
EOF
pass;