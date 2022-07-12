#subruledef register
{
	a => 0x0
	b => 0x1
	m => 0x2
	f => 0x3
}

#subruledef source
{
	{immediate: u8}    => 0b0 @ immediate
	ptr[{r: register}] => 0b1 @ r`8
} 

#ruledef 
{
	mov 	{r: register}, {src: source} => 0x0 @ r`3 @ src
	ld 	{r: register}, {src: source} => 0x1 @ r`3 @ src
	ld 	{r: register} 		     => 0x1 @ r`3 @ 0b1
	sto 	{r: register}, {src: source} => 0x2 @ r`3 @ src
	sto 	{r: register} 		     => 0x2 @ r`3 @ 0b1
	add 	{r: register}, {src: source} => 0x3 @ r`3 @ src
	adc 	{r: register}, {src: source} => 0x4 @ r`3 @ src
	sbb 	{r: register}, {src: source} => 0x5 @ r`3 @ src
	and 	{r: register}, {src: source} => 0x6 @ r`3 @ src
	or 	{r: register}, {src: source} => 0x7 @ r`3 @ src
	nor 	{r: register}, {src: source} => 0x8 @ r`3 @ src
	cmp 	{r: register}, {src: source} => 0x9 @ r`3 @ src
	push		       {src: source} => 0xA @ 0b00 @ src 
	push 	{r: register},		     => 0xA @ r`3 @ 0b1
	pop 	{r: register}		     => 0xB @ r
	jnz		       {src: source} => 0xC @ 0b000 @ src
	jnz  	{r: register}		     => 0xC @ r`3 @ 0b1
	jmp 		       {src: source} => 0xD @ 0b000 @ src
	jmp				     => 0xD @ 0x1		
	hlt 				     => 0xE @ 0x0
	out 	{r: register}, {mode}	     => 0xF @ r`3 @ mode`1
}



