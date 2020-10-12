BEGIN{
	i=1
}

{
	x=1
	while(x<NF){
		print $x "\t"
		x++
	}
	i++
	print "\n"
}
