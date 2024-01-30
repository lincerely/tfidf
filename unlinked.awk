# csv input, print line if $1 not contains $2 's filename
# variable: dir
BEGIN {
	FS=", "
}
{
	fname = $2
	sub(dir, "", fname)
	if (system("9 grep -s \"" fname "\" \"" $1 "\"") == 1) {
		print $0
	}
}
$3 < 0.25 {
	exit
}