PROGRAMS="bubsort \
cat \
cmp \
cp \
echo \
hex-dump \
insult \
ls \
mkdir \
pwd \
rm \
shell"
EXAMPLES="../../examples"
CMDLINE="pintos --filesys-size=100"
CMDLINE_END="-f -q run 'shell'"

for PROGRAM in $PROGRAMS; do
    CMDLINE+=" -p $EXAMPLES/$PROGRAM -a $PROGRAM"
done
CMDLINE+=" -- $CMDLINE_END"
echo $CMDLINE
eval $CMDLINE
