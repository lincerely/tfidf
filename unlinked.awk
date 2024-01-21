# csv input, check $1 contains $2 's filename
# if not, it is unlinked, print the line.
BEGIN {
	FS=", "
}
{
	fname = $2
	sub("/Users/lincoln/Documents/box/", "", fname)
	if (system("9 grep -s \"" fname "\" \"" $1 "\"") == 1) {
		print $0
	}
}
$3 < 0.25 {
	exit
}